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
       Only body positions change; t, bhMass, camera, and velocities stay."""
    y = x.clone()
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
    first_order_train_loss = ((train_n - first_order_train_n) ** 2).mean().item()
    first_order_val_loss = ((val_n - first_order_val_n) ** 2).mean().item()

    print()
    print('=== Baseline ladder (validation, masked + normalized dims) ===')
    print('  identity   : %.6e' % identity_val_loss)
    print('  first-order: %.6e' % first_order_val_loss)
    print()
    print('=== Baseline ladder (training, masked + normalized dims) ===')
    print('  identity   : %.6e' % identity_train_loss)
    print('  first-order: %.6e' % first_order_train_loss)

    # ---- Model: absolute prediction on masked dims ----
    print()
    print('=== Absolute: model(x_t) -> x_{t+1} ===')
    model = make_model(train_x.shape[1]).double()
    opt = torch.optim.Adam(model.parameters(), lr=1e-3)
    for epoch in range(20):
        tr = train_epoch(model, opt, train_n, train_yn)
        tr_l = eval_loss(model, train_n, train_yn)
        va_l = eval_loss(model, val_n, val_yn)
        if epoch % 5 == 0 or epoch == 19:
            print('epoch %2d: train=%.6e  val=%.6e' % (epoch, tr_l, va_l))

    model_val_loss = eval_loss(model, val_n, val_yn)
    model_train_loss = eval_loss(model, train_n, train_yn)
    print()
    print('=== Diagnostic: model vs identity (fit quality) ===')
    print('  train: model=%.6e  identity=%.6e  ratio=%.2fx' % (
        model_train_loss, identity_train_loss,
        model_train_loss / identity_train_loss))
    print('  val:   model=%.6e  identity=%.6e  ratio=%.2fx' % (
        model_val_loss, identity_val_loss,
        model_val_loss / identity_val_loss))
    print()
    print('=== RESULT ===')
    print('  model_val_loss      = %.6e' % model_val_loss)
    print('  first_order_val_loss= %.6e' % first_order_val_loss)
    print('  ratio = %.4f' % (model_val_loss / first_order_val_loss))
    print('  model vs identity   = %.4f' % (model_val_loss / identity_val_loss))


if __name__ == '__main__':
    main()
