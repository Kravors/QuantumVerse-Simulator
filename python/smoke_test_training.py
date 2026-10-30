#!/usr/bin/env python3
"""
python/smoke_test_training.py — smallest possible training pipeline smoke test.

Loads all data/sessions/*/frames.bin, trains a tiny MLP to predict the next
frame from the current frame, and compares against trivial baselines.

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


def normalize(train_x):
    """Per-dimension z-score. Static dims (std~0) map to 0."""
    mu = train_x.mean(dim=0)
    sd = train_x.std(dim=0)
    static = sd < 1e-6
    sd_safe = torch.where(static, torch.ones_like(sd), sd)
    # For static dims, subtract the mean so (val-mean)=0 -> result 0
    mu_safe = torch.where(static, mu, mu)
    return (train_x - mu_safe) / sd_safe, mu_safe, sd_safe, static


def main():
    sessions_dir = 'data/sessions'
    sessions = load_sessions(sessions_dir)
    if len(sessions) < 2:
        print('Need at least 2 sessions. Found %d.' % len(sessions))
        sys.exit(1)

    train_sessions = sessions[:-1]
    val_session = sessions[-1]

    train_x = torch.cat([t[:-1] for _, t in train_sessions], dim=0)
    train_y = torch.cat([t[1:] for _, t in train_sessions], dim=0)
    val_x = val_session[1][:-1]
    val_y = val_session[1][1:]

    print()
    print('Train: %s, Val: %s' % (tuple(train_x.shape), tuple(val_x.shape)))

    # Normalize
    train_n, mu_safe, sd_safe, static = normalize(train_x)
    train_n = torch.where(static, torch.zeros_like(train_n), train_n)
    train_yn = (train_y - mu_safe) / sd_safe
    train_yn = torch.where(static, torch.zeros_like(train_yn), train_yn)
    val_n = (val_x - mu_safe) / sd_safe
    val_n = torch.where(static, torch.zeros_like(val_n), val_n)
    val_yn = (val_y - mu_safe) / sd_safe
    val_yn = torch.where(static, torch.zeros_like(val_yn), val_yn)

    print('Static dims (no variance):', [i for i, s in enumerate(static) if s.item()])

    # --- Absolute prediction ---
    print()
    print('=== Absolute: model(x_t) -> x_{t+1} ===')
    identity_loss = ((train_n - train_yn) ** 2).mean().item()
    const_loss = (train_yn ** 2).mean().item()
    print('Baselines: identity=%.6f  constant=%.6f' % (identity_loss, const_loss))

    model = make_model(train_x.shape[1]).double()
    opt = torch.optim.Adam(model.parameters(), lr=1e-3)
    for epoch in range(20):
        tr = train_epoch(model, opt, train_n, train_yn)
        tr_l = eval_loss(model, train_n, train_yn)
        va_l = eval_loss(model, val_n, val_yn)
        print('epoch %2d: train=%.6f  val=%.6f' % (epoch, tr_l, va_l))

    print('train/identity = %.4f   val/identity = %.4f' % (tr_l / identity_loss, va_l / identity_loss))

    # --- Delta prediction ---
    print()
    print('=== Delta: model(x_t) -> x_{t+1} - x_t ===')
    train_yd = train_y - train_x
    val_yd = val_y - val_x
    mu_d = train_yd.mean(dim=0)
    sd_d = train_yd.std(dim=0)
    static_d = sd_d < 1e-6
    sd_d_safe = torch.where(static_d, torch.ones_like(sd_d), sd_d)
    mu_d_safe = torch.where(static_d, torch.zeros_like(mu_d), mu_d)
    train_yn_d = (train_yd - mu_d_safe) / sd_d_safe
    train_yn_d = torch.where(static_d, torch.zeros_like(train_yn_d), train_yn_d)
    val_yn_d = (val_yd - mu_d_safe) / sd_d_safe
    val_yn_d = torch.where(static_d, torch.zeros_like(val_yn_d), val_yn_d)
    identity_d_loss = (train_yn_d ** 2).mean().item()
    const_loss_d = (train_yn_d ** 2).mean().item()
    print('Baselines: identity-delta=%.6e  constant=%.6e' % (identity_d_loss, const_loss_d))

    model_d = make_model(train_x.shape[1]).double()
    opt_d = torch.optim.Adam(model_d.parameters(), lr=1e-3)
    for epoch in range(20):
        tr = train_epoch(model_d, opt_d, train_n, train_yn_d)
        tr_l = eval_loss(model_d, train_n, train_yn_d)
        va_l = eval_loss(model_d, val_n, val_yn_d)
        print('epoch %2d: train=%.6f  val=%.6f' % (epoch, tr_l, va_l))

    print('train/identity = %.4f   val/identity = %.4f' % (tr_l / identity_d_loss, va_l / identity_d_loss))


if __name__ == '__main__':
    main()
