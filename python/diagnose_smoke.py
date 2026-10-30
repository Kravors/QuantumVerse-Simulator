#!/usr/bin/env python3
"""
python/diagnose_smoke.py — diagnostic for smoke_test_training.py results.
Prints per-dimension std, absolute identity losses, and re-runs training
excluding degenerate dims.
"""

import struct, os
from pathlib import Path

import torch
import torch.nn as nn


def read_frames(path):
    frames = []
    with open(path, 'rb') as f:
        header = f.read(64)
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
            body_names = []
            for i in range(bodyCount):
                nameLen = struct.unpack_from('<I', buf, p)[0]; p += 4
                name = buf[p:p + nameLen].decode('utf-8'); p += nameLen
                body_names.append(name)
            positions = []
            for i in range(bodyCount):
                pos = struct.unpack_from('<ddd', buf, p); p += 24
                positions.append(pos)
            velocities = []
            for i in range(bodyCount):
                vel = struct.unpack_from('<ddd', buf, p); p += 24
                velocities.append(vel)
            body_data = list(zip(body_names, positions, velocities))
            cam = struct.unpack_from('<dddd', buf, p); p += 32
            p += 32
            bhMass = struct.unpack_from('<d', buf, p)[0]; p += 8
            frames.append({'time': time_val, 'bodies': body_data,
                           'camera': cam[:3], 'bhMass': bhMass})
    return frames


def session_to_tensor(frames):
    vecs = []
    for fr in frames:
        bodies = sorted(fr['bodies'], key=lambda b: b[0])
        v = [fr['time'], fr['bhMass']] + list(fr['camera'])
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
    return out


def make_model(dim):
    return nn.Sequential(
        nn.Linear(dim, 128), nn.ReLU(),
        nn.Linear(128, 128), nn.ReLU(),
        nn.Linear(128, dim),
    ).double()


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


def main():
    sessions = load_sessions('data/sessions')
    train_sessions = sessions[:-1]
    val_session = sessions[-1]

    train_x = torch.cat([t[:-1] for _, t in train_sessions], dim=0)
    train_y = torch.cat([t[1:] for _, t in train_sessions], dim=0)
    val_x = val_session[1][:-1]
    val_y = val_session[1][1:]

    mu = train_x.mean(dim=0)
    sd = train_x.std(dim=0)
    D = train_x.shape[1]
    static = sd < 1e-6

    print('=== Per-dimension statistics ===')
    for i in range(D):
        flag = 'DEGENERATE' if static[i].item() else ''
        print(f'  dim {i:2d}: mean={mu[i]: .6e}  std={sd[i]: .6e}  {flag}')

    print()
    print('Degenerate dims:', [i for i in range(D) if static[i].item()])

    # --- Absolute prediction ---
    print()
    print('=== Absolute: model(x_t) -> x_{t+1} ===')

    mu_safe = torch.where(static, torch.zeros_like(mu), mu)
    sd_safe = torch.where(static, torch.ones_like(sd), sd)
    train_n = (train_x - mu_safe) / sd_safe
    train_n = torch.where(static, torch.zeros_like(train_n), train_n)
    train_yn = (train_y - mu_safe) / sd_safe
    train_yn = torch.where(static, torch.zeros_like(train_yn), train_yn)
    val_n = (val_x - mu_safe) / sd_safe
    val_n = torch.where(static, torch.zeros_like(val_n), val_n)
    val_yn = (val_y - mu_safe) / sd_safe
    val_yn = torch.where(static, torch.zeros_like(val_yn), val_yn)

    identity_train = ((train_n - train_yn) ** 2).mean().item()
    identity_val = ((val_n - val_yn) ** 2).mean().item()
    print(f'identity train={identity_train:.6e}  val={identity_val:.6e}')

    model = make_model(D)
    opt = torch.optim.Adam(model.parameters(), lr=1e-3)
    for epoch in range(20):
        tr = train_epoch(model, opt, train_n, train_yn)
        tr_l = eval_loss(model, train_n, train_yn)
        va_l = eval_loss(model, val_n, val_yn)
    print(f'ABSOLUTE final: train={tr_l:.6e}  val={va_l:.6e}')
    print(f'ABSOLUTE ratios: train/identity={tr_l/identity_train:.4f}  '
          f'val/identity={va_l/identity_val:.4f}')

    # --- Retrain without degenerate dims ---
    keep = [i for i in range(D) if not static[i].item()]
    print()
    print(f'Retraining with {len(keep)} non-degenerate dims (dropped '
          f'{D - len(keep)})')

    train_n2 = train_n[:, keep]
    train_yn2 = train_yn[:, keep]
    val_n2 = val_n[:, keep]
    val_yn2 = val_yn[:, keep]

    model2 = make_model(len(keep))
    opt2 = torch.optim.Adam(model2.parameters(), lr=1e-3)
    for epoch in range(20):
        tr = train_epoch(model2, opt2, train_n2, train_yn2)
        tr_l = eval_loss(model2, train_n2, train_yn2)
        va_l = eval_loss(model2, val_n2, val_yn2)
    print(f'NO-DEGEN final: train={tr_l:.6e}  val={va_l:.6e}')
    print(f'NO-DEGEN ratios: train/identity={tr_l/identity_train:.4f}  '
          f'val/identity={va_l/identity_val:.4f}')


if __name__ == '__main__':
    main()
