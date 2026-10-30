#!/usr/bin/env python3
"""
python/smoke_test_training.py — smallest possible training pipeline smoke test.

Loads all data/sessions/*/frames.bin, trains a tiny MLP to predict the next
frame from the current frame, and compares against trivial baselines:

  identity:  x_{t+1} = x_t                      (no change)
  first-order: x_{t+1} = x_t + v_t * dt         (kinematic extrapolation)
  model:     MLP(x_t) -> x_{t+1}                (learned predictor)

The meaningful number is model_val_loss / first_order_val_loss:
  < 0.5  -> model learned real physics, scale up
  0.5-0.9 -> model learned something modest
  ≈ 1.0  -> model learned only trivial kinematics, need representation change
  > 1.0  -> model is broken, fix training first

Run:  python python/smoke_test_training.py
"""

import struct, os, math, sys
from pathlib import Path

import torch
import torch.nn as nn


# ---------------------------------------------------------------------------
# Binary reader for SessionRecorder frames.bin (schema v1)
# Layout per frame:
#   uint32 frame_len
#   double time
#   uint32 bodyCount
#   per body: uint32 nameLen + name bytes, double pos[3], double vel[3]
#   double cameraPosition[4], double cameraLookAt[4], double blackHoleMassKg
#   double totalEnergy, double kineticEnergy, double potentialEnergy
# ---------------------------------------------------------------------------
def read_frames(path):
    frames = []
    with open(path, 'rb') as f:
        header = f.read(64)
        magic = struct.unpack('<I', header[:4])[0]
        if magic != 0x51564652:
            raise ValueError('bad magic in %s' % path)
        while True:
            len_bytes = f.read(4)
            if not len_bytes or len(len_bytes) < 4:
                break
            frame_len = struct.unpack('<I', len_bytes)[0]
            buf = f.read(frame_len)
            if not buf or len(buf) < frame_len:
                break
            p = 0
            time_val = struct.unpack_from('<d', buf, p)[0]; p += 8
            bodyCount = struct.unpack_from('<I', buf, p)[0]; p += 4
            # Read all body names first
            body_names = []
            for i in range(bodyCount):
                nameLen = struct.unpack_from('<I', buf, p)[0]; p += 4
                name = buf[p:p + nameLen].decode('utf-8'); p += nameLen
                body_names.append(name)
            # Read all positions
            positions = []
            for i in range(bodyCount):
                pos = struct.unpack_from('<ddd', buf, p); p += 24
                positions.append(pos)
            # Read all velocities
            velocities = []
            for i in range(bodyCount):
                vel = struct.unpack_from('<ddd', buf, p); p += 24
                velocities.append(vel)
            body_data = list(zip(body_names, positions, velocities))
            cam = struct.unpack_from('<dddd', buf, p); p += 32
            p += 32  # cameraLookAt
            bhMass = struct.unpack_from('<d', buf, p)[0]; p += 8
            te, ke, pe = struct.unpack_from('<ddd', buf, p); p += 24
            frames.append({
                'time': time_val,
                'bodies': body_data,
                'camera': cam,
                'bhMass': bhMass,
                'energy': (te, ke, pe),
            })
    return frames


def session_to_tensor(frames):
    """Flatten each frame to a fixed vector:
       [t, bhMass, cam_x, cam_y, cam_z, then per body: x,y,z,vx,vy,vz]
       Bodies are sorted by name for deterministic ordering."""
    vecs = []
    for fr in frames:
        bodies = sorted(fr['bodies'], key=lambda b: b[0])
        v = [fr['time'], fr['bhMass']] + list(fr['camera'][:3])
        for _, pos, vel in bodies:
            v += list(pos) + list(vel)
        vecs.append(v)
    return torch.tensor(vecs, dtype=torch.float64)


def load_sessions(sessions_dir):
    sessions_dir = Path(sessions_dir)
    session_dirs = sorted([d for d in sessions_dir.iterdir()
                           if d.is_dir() and (d / 'frames.bin').exists()])
    out = []
    for sdir in session_dirs:
        frames = read_frames(str(sdir / 'frames.bin'))
        t = session_to_tensor(frames)
        out.append((sdir.name, t))
        print('  %s: %d frames, dim=%d' % (sdir.name, t.shape[0], t.shape[1]))
    return out


# Tensor layout constants (matches session_to_tensor)
HEADER_DIMS = 5    # t, bhMass, cam_x, cam_y, cam_z
BODY_DIMS = 6      # per body: x,y,z,vx,vy,vz
DT = 0.1           # 10 Hz recording interval


def make_model(dim):
    return nn.Sequential(
        nn.Linear(dim, 128), nn.ReLU(),
        nn.Linear(128, 128), nn.ReLU(),
        nn.Linear(128, dim),
    )


class PhysicsInformedMLP(nn.Module):
    """MLP that predicts raw acceleration only; position/velocity updated by
    exact kinematics in RAW space. Input is normalized internally for the MLP,
    but the kinematic update pos += vel*dt, vel += accel*dt is in raw space.
    If net outputs zero, model == first-order baseline exactly."""

    def __init__(self, state_dim=59, dt=0.1, mu=None, sd=None):
        super().__init__()
        self.dt = dt
        self.n_bodies = (state_dim - HEADER_DIMS) // BODY_DIMS
        if mu is not None:
            self.register_buffer('mu', mu.double())
            self.register_buffer('sd', sd.double())
        else:
            self.register_buffer('mu', torch.zeros(state_dim))
            self.register_buffer('sd', torch.ones(state_dim))
        self.net = nn.Sequential(
            nn.Linear(state_dim, 128), nn.Tanh(),
            nn.Linear(128, 128), nn.Tanh(),
            nn.Linear(128, self.n_bodies * 3),  # raw acceleration per body
        )
        # Zero-init bias, small random weights for last layer so model starts
        # near first-order but gradients can flow to all layers
        nn.init.zeros_(self.net[-1].bias)
        nn.init.uniform_(self.net[-1].weight, -1e-4, 1e-4)

    def forward(self, x_raw):
        # Normalize input for MLP stability (all dims O(1))
        x_norm = (x_raw - self.mu) / self.sd

        # Per-body raw position and velocity
        n_bodies = self.n_bodies
        pos_parts, vel_parts = [], []
        for b in range(n_bodies):
            off = HEADER_DIMS + b * BODY_DIMS
            pos_parts.append(x_raw[:, off:off + 3])
            vel_parts.append(x_raw[:, off + 3:off + 6])
        pos_raw = torch.stack(pos_parts, dim=1)   # [B, N, 3]
        vel_raw = torch.stack(vel_parts, dim=1)   # [B, N, 3]

        # Predict raw acceleration (last layer linear, outputs raw m/s^2)
        accel_raw = self.net(x_norm).view(-1, n_bodies, 3)  # [B, N, 3]

        # Raw-space kinematics (exact, no network involvement for positions)
        out_raw = x_raw.clone()
        out_raw[:, 0] = x_raw[:, 0] + self.dt                       # t += dt
        for b in range(n_bodies):
            off = HEADER_DIMS + b * BODY_DIMS
            out_raw[:, off:off + 3] = pos_raw[:, b, :] + vel_raw[:, b, :] * self.dt  # pos += vel*dt
            out_raw[:, off + 3:off + 6] = vel_raw[:, b, :] + accel_raw[:, b, :] * self.dt  # vel += accel*dt

        # Normalize output so all dims are O(1) for the loss
        out_norm = (out_raw - self.mu) / self.sd
        return out_norm


def train_epoch(model, opt, train_n, train_yn, batch=64):
    perm = torch.randperm(train_n.shape[0])
    total_loss = 0.0
    count = 0
    for i in range(0, len(perm), batch):
        idx = perm[i:i + batch]
        x = train_n[idx]
        y = train_yn[idx]
        loss = ((model(x) - y) ** 2).mean()
        opt.zero_grad(); loss.backward(); opt.step()
        total_loss += loss.item() * idx.shape[0]
        count += idx.shape[0]
    return total_loss / count


def eval_loss(model, x, y):
    with torch.no_grad():
        return ((model(x) - y) ** 2).mean().item()


def first_order(x):
    """Kinematic extrapolation: x_{t+1} = x_t + v_t * dt.
       t advances by dt, positions advance by vel*dt, velocities stay,
       bhMass/camera/energy stay unchanged."""
    y = x.clone()
    y[:, 0] = x[:, 0] + DT   # t += dt
    n_bodies = (x.shape[1] - HEADER_DIMS) // BODY_DIMS
    for b in range(n_bodies):
        off = HEADER_DIMS + b * BODY_DIMS
        y[:, off + 0] += x[:, off + 3] * DT   # px += vx * dt
        y[:, off + 1] += x[:, off + 4] * DT   # py += vy * dt
        y[:, off + 2] += x[:, off + 5] * DT   # pz += vz * dt
    return y


def mask_degenerate(train_x):
    """Drop dims with std < 1e-3 (constant across the dataset)."""
    mu = train_x.mean(dim=0)
    sd = train_x.std(dim=0)
    keep = sd >= 1e-3
    print('Degenerate dims dropped:',
          [i for i in range(train_x.shape[1]) if not keep[i].item()])
    return train_x[:, keep], mu[keep], sd[keep], keep


def main():
    sessions_dir = 'data/sessions'
    sessions = load_sessions(sessions_dir)
    if len(sessions) < 2:
        print('Need at least 2 sessions. Found %d.' % len(sessions))
        sys.exit(1)

    # ---- Within-session split (Option A): first 80% of each session -> train,
    # last 20% -> val. Train and val have the same physics (same metric, same
    # bodies, same initial conditions); only different time windows. ----
    train_x_parts, train_y_parts, val_x_parts, val_y_parts = [], [], [], []
    for sname, t in sessions:
        n = t.shape[0]
        split = int(0.8 * n)
        if split < 2:
            continue
        train_x_parts.append(t[:split - 1])
        train_y_parts.append(t[1:split])
        val_x_parts.append(t[split:-1])
        val_y_parts.append(t[split + 1:])
        print('  %s: train=%d val=%d' % (sname, split - 1, n - split - 1))

    train_x_full = torch.cat(train_x_parts, dim=0)
    train_y_full = torch.cat(train_y_parts, dim=0)
    val_x_full = torch.cat(val_x_parts, dim=0)
    val_y_full = torch.cat(val_y_parts, dim=0)

    print()
    print('Train: %s, Val: %s' % (tuple(train_x_full.shape), tuple(val_x_full.shape)))

    # ---- Mask degenerate dims (the 1e-3 threshold keeps only dims that vary) ----
    train_x, mu, sd, keep = mask_degenerate(train_x_full)
    train_y = train_y_full[:, keep]
    val_x = val_x_full[:, keep]
    val_y = val_y_full[:, keep]

    # Sanity: per-dim std for first 5 dims detects distribution shift
    print('  train std[:5]: %s' % train_x.std(dim=0)[:5].tolist())
    print('  val   std[:5]: %s' % val_x.std(dim=0)[:5].tolist())

    # Sanity: val must use the same mask as train
    print('  train_x: %s   val_x: %s   model expects dim=%d' % (
        tuple(train_x.shape), tuple(val_x.shape), train_x.shape[1]))

    # Normalize with masked stats
    train_n = (train_x - mu) / sd
    train_yn = (train_y - mu) / sd
    val_n = (val_x - mu) / sd
    val_yn = (val_y - mu) / sd

    print('Active dims:', int(keep.sum().item()), 'of', len(keep))

    # ---- Baseline ladder ----
    # Identity: predict current frame = next frame (already in normalized space)
    identity_train_loss = ((train_n - train_yn) ** 2).mean().item()
    identity_val_loss = ((val_n - val_yn) ** 2).mean().item()

    # First-order: x_{t+1} = x_t + v_t * dt (raw space, then mask + normalize)
    first_order_train_raw = first_order(train_x_full)[:, keep]
    first_order_val_raw = first_order(val_x_full)[:, keep]
    first_order_train_n = (first_order_train_raw - mu) / sd
    first_order_val_n = (first_order_val_raw - mu) / sd
    first_order_train_loss = ((train_yn - first_order_train_n) ** 2).mean().item()
    first_order_val_loss = ((val_yn - first_order_val_n) ** 2).mean().item()

    print()
    print('=== Baseline ladder (validation, masked + normalized dims) ===')
    print('  identity   : %.6e' % identity_val_loss)
    print('  first-order: %.6e' % first_order_val_loss)
    print()
    print('=== Baseline ladder (training, masked + normalized dims) ===')
    print('  identity   : %.6e' % identity_train_loss)
    print('  first-order: %.6e' % first_order_train_loss)

    # ---- Normalize full data (clamped std to avoid div-by-zero on degenerate dims) ----
    mu_full = train_x_full.mean(dim=0).double()
    sd_full = train_x_full.std(dim=0).clamp(min=1e-6).double()
    train_n_full = (train_x_full - mu_full) / sd_full
    train_yn_full = (train_y_full - mu_full) / sd_full
    val_n_full = (val_x_full - mu_full) / sd_full
    val_yn_full = (val_y_full - mu_full) / sd_full

    # ---- Baseline ladder on RAW full data ----
    identity_train_loss_raw = ((train_x_full - train_y_full) ** 2).mean().item()
    identity_val_loss_raw = ((val_x_full - val_y_full) ** 2).mean().item()

    first_order_train_raw = first_order(train_x_full)
    first_order_val_raw = first_order(val_x_full)
    first_order_train_loss_raw = ((first_order_train_raw - train_y_full) ** 2).mean().item()
    first_order_val_loss_raw = ((first_order_val_raw - val_y_full) ** 2).mean().item()

    # Also compute baselines in normalized space (matches model's loss space)
    identity_train_loss_norm = ((train_n_full - train_yn_full) ** 2).mean().item()
    identity_val_loss_norm = ((val_n_full - val_yn_full) ** 2).mean().item()
    first_order_train_n = (first_order(train_x_full) - mu_full) / sd_full
    first_order_val_n = (first_order(val_x_full) - mu_full) / sd_full
    first_order_train_loss_norm = ((train_yn_full - first_order_train_n) ** 2).mean().item()
    first_order_val_loss_norm = ((val_yn_full - first_order_val_n) ** 2).mean().item()

    print()
    print('=== Baseline ladder (validation, raw 59-dim) ===')
    print('  identity   : %.6e' % identity_val_loss_raw)
    print('  first-order: %.6e' % first_order_val_loss_raw)
    print()
    print('=== Baseline ladder (training, raw 59-dim) ===')
    print('  identity   : %.6e' % identity_train_loss_raw)
    print('  first-order: %.6e' % first_order_train_loss_raw)
    print()
    print('=== Baseline ladder (validation, normalized 59-dim) ===')
    print('  identity   : %.6e' % identity_val_loss_norm)
    print('  first-order: %.6e' % first_order_val_loss_norm)
    print()
    print('=== Baseline ladder (training, normalized 59-dim) ===')
    print('  identity   : %.6e' % identity_train_loss_norm)
    print('  first-order: %.6e' % first_order_train_loss_norm)

    # ---- Model: PhysicsInformedMLP on RAW input, normalized output ----
    # Net predicts raw acceleration; pos += vel*dt in raw space; output normalized.
    # Zero-init bias + small random weights -> starts near first-order,
    # gradients flow to all layers. Loss in normalized space (all dims O(1)).
    print()
    print('=== Physics-informed: MLP(x_t) -> accel, then x_{t+1} = kinematics (norm loss) ===')
    model = PhysicsInformedMLP(state_dim=59, dt=DT, mu=mu_full, sd=sd_full).double()
    opt = torch.optim.Adam(model.parameters(), lr=1e-3)
    for epoch in range(200):
        tr = train_epoch(model, opt, train_x_full, train_yn_full)
        tr_l = eval_loss(model, train_x_full, train_yn_full)
        va_l = eval_loss(model, val_x_full, val_yn_full)
        if epoch % 5 == 0 or epoch == 199:
            print('epoch %3d: train=%.6e  val=%.6e' % (epoch, tr_l, va_l))

    model_val_loss = eval_loss(model, val_x_full, val_yn_full)
    model_train_loss = eval_loss(model, train_x_full, train_yn_full)
    print()
    print('=== Diagnostic: model vs identity (fit quality) ===')
    print('  train: model=%.6e  identity=%.6e  ratio=%.2fx' % (
        model_train_loss, identity_train_loss_norm,
        model_train_loss / identity_train_loss_norm))
    print('  val:   model=%.6e  identity=%.6e  ratio=%.2fx' % (
        model_val_loss, identity_val_loss_norm,
        model_val_loss / identity_val_loss_norm))
    print()
    print('=== RESULT ===')
    print('  model_val_loss      = %.6e' % model_val_loss)
    print('  first_order_val_loss= %.6e' % first_order_val_loss_norm)
    print('  ratio = %.4f' % (model_val_loss / first_order_val_loss_norm))
    print('  model vs identity   = %.4f' % (model_val_loss / identity_val_loss_norm))


if __name__ == '__main__':
    main()
