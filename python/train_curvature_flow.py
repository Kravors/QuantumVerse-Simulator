#!/usr/bin/env python3
"""
python/train_curvature_flow.py — train a RealNVP normalizing flow on 6 curvature dims.

Trains on normal v2 sessions (varied probe positions → real variance), evaluates
on the equator anomaly session (off-manifold curvature).

6 input dims: [bhMass, bhSpin, probeKretschmann, probeRicciScalar,
               probeWeylSquared, probeRedshift]
Curvature dims are log10-transformed (span orders of magnitude).

Success criterion: >50% of anomaly frames have log p below normal 5th percentile.

Run:  python python/train_curvature_flow.py
"""

import struct, os, math, sys
from pathlib import Path

import torch
import torch.nn as nn
import numpy as np


# ---------------------------------------------------------------------------
# Binary reader for SessionRecorder frames.bin (schema v2)
# ---------------------------------------------------------------------------
def read_frames(path):
    frames = []
    with open(path, 'rb') as f:
        header = f.read(64)
        magic = struct.unpack('<I', header[:4])[0]
        if magic != 0x51564652:
            raise ValueError('bad magic in %s' % path)
        schema = struct.unpack('<I', header[4:8])[0]
        if schema != 2:
            raise ValueError('schema v1 not supported')
        while True:
            len_bytes = f.read(4)
            if not len_bytes or len(len_bytes) < 4:
                break
            frame_len = struct.unpack('<I', len_bytes)[0]
            buf = f.read(frame_len)
            if not buf or len(buf) < frame_len:
                break
            p = 0
            struct.unpack_from('<d', buf, p); p += 8  # time
            bodyCount = struct.unpack_from('<I', buf, p)[0]; p += 4
            for i in range(bodyCount):
                nameLen = struct.unpack_from('<I', buf, p)[0]; p += 4
                p += nameLen
            p += bodyCount * 48  # positions + velocities
            p += 64  # cameras
            bhMass = struct.unpack_from('<d', buf, p)[0]; p += 8
            bhSpin = struct.unpack_from('<d', buf, p)[0]; p += 8
            probeKret = struct.unpack_from('<d', buf, p)[0]; p += 8
            probeRicci = struct.unpack_from('<d', buf, p)[0]; p += 8
            probeWeyl = struct.unpack_from('<d', buf, p)[0]; p += 8
            probeRedshift = struct.unpack_from('<d', buf, p)[0]; p += 8
            te, ke, pe = struct.unpack_from('<ddd', buf, p); p += 24
            frames.append({
                'bhMass': bhMass,
                'bhSpin': bhSpin,
                'probeKretschmann': probeKret,
                'probeRicciScalar': probeRicci,
                'probeWeylSquared': probeWeyl,
                'probeRedshift': probeRedshift,
            })
    return frames


def load_curvature_data(session_dirs):
    """Load 6 curvature dims from each session. Returns list of (name, tensor[N,6])."""
    sessions_dir = Path('data/sessions')
    out = []
    for name in session_dirs:
        sdir = sessions_dir / name
        if not (sdir / 'frames.bin').exists():
            print('WARNING: %s not found' % sdir)
            continue
        frames = read_frames(str(sdir / 'frames.bin'))
        data = np.array([[
            fr['bhMass'], fr['bhSpin'],
            fr['probeKretschmann'], fr['probeRicciScalar'],
            fr['probeWeylSquared'], fr['probeRedshift'],
        ] for fr in frames], dtype=np.float64)
        out.append((name, torch.tensor(data, dtype=torch.float64)))
        print('  %s: %d frames' % (name, len(frames)))
    return out


# Dim names
DIM_NAMES = ['bhMass', 'bhSpin', 'probeKretschmann', 'probeRicciScalar',
             'probeWeylSquared', 'probeRedshift']

# Dims to log-transform (span orders of magnitude, always positive in normal)
LOG_DIMS = {'probeKretschmann', 'probeWeylSquared', 'probeRedshift'}


def log10_safe_transform(x):
    """log10(x + eps) for positive dims; identity for others."""
    out = x.clone()
    for i, name in enumerate(DIM_NAMES):
        if name in LOG_DIMS:
            out[:, i] = torch.log10(torch.abs(out[:, i]) + 1e-300)
    return out


# ---------------------------------------------------------------------------
# RealNVP coupling layer
# ---------------------------------------------------------------------------
class CouplingLayer(nn.Module):
    def __init__(self, dim, hidden_dim=64, mask_parity=0):
        super().__init__()
        self.dim = dim
        self.mask = torch.zeros(dim)
        self.mask[mask_parity::2] = 1.0  # alternate dims
        self.scale_net = nn.Sequential(
            nn.Linear(dim, hidden_dim), nn.Tanh(),
            nn.Linear(hidden_dim, hidden_dim), nn.Tanh(),
            nn.Linear(hidden_dim, dim),
        )
        self.translate_net = nn.Sequential(
            nn.Linear(dim, hidden_dim), nn.Tanh(),
            nn.Linear(hidden_dim, hidden_dim), nn.Tanh(),
            nn.Linear(hidden_dim, dim),
        )

    def forward(self, x, reverse=False):
        masked = x * self.mask.to(x.device)
        s = self.scale_net(masked) * (1 - self.mask.to(x.device))
        t = self.translate_net(masked) * (1 - self.mask.to(x.device))
        if reverse:
            x = (x - t) * torch.exp(-s)
            log_det = -s.sum(dim=1)
        else:
            x = masked + (1 - self.mask.to(x.device)) * (x * torch.exp(s) + t)
            log_det = s.sum(dim=1)
        return x, log_det


class RealNVP(nn.Module):
    def __init__(self, dim, n_layers=6, hidden_dim=64):
        super().__init__()
        self.dim = dim
        self.layers = nn.ModuleList([
            CouplingLayer(dim, hidden_dim, i % 2) for i in range(n_layers)
        ])

    def forward(self, x):
        log_det_total = torch.zeros(x.shape[0], device=x.device)
        for layer in self.layers:
            x, ld = layer(x, reverse=False)
            log_det_total = log_det_total + ld
        return x, log_det_total

    def inverse(self, z):
        x = z
        for layer in reversed(self.layers):
            x, _ = layer(x, reverse=True)
        return x

    def log_prob(self, x):
        z, log_det = self.forward(x)
        # Standard normal prior
        log_prior = -0.5 * (z ** 2).sum(dim=1) - 0.5 * self.dim * np.log(2 * np.pi)
        return log_prior + log_det


def train_epoch(model, opt, data, batch=256):
    n = data.shape[0]
    perm = torch.randperm(n)
    total_loss = 0.0
    count = 0
    for i in range(0, n, batch):
        idx = perm[i:i + batch]
        x = data[idx]
        log_p = model.log_prob(x)
        loss = -log_p.mean()
        opt.zero_grad()
        loss.backward()
        opt.step()
        total_loss += loss.item() * idx.shape[0]
        count += idx.shape[0]
    return total_loss / count


def main():
    # ---- Normal sessions: S1, S2, S3, N4, N5, N6 ----
    normal_names = [
        '5c896a226f77e318-session',   # S1-v2 normal Kerr, probe at r=100000
        '08ea8ad7a77e318-session',   # S2-v2 normal Kerr, probe at r=100000
        'b09e244b8777e318-session',  # S3-v2 normal Schwarzschild, probe at r=100000
        'a45edb6b5078e318-session',  # N4 normal Kerr, probe at r=50000
        'e0bd7bb5778e318-session',   # N5 normal Kerr, probe at r=75000
        '6c7782fa5778e318-session',  # N6 normal Kerr, probe at r=150000
    ]
    # Anomaly session: equator near horizon
    anomaly_name = 'fc78a0a9ef77e318-session'  # probe at r=23330, kret=1.38e-18

    print('Loading normal sessions...')
    normal = load_curvature_data(normal_names)
    print('Loading anomaly session...')
    anomaly = load_curvature_data([anomaly_name])

    if len(normal) < 3:
        print('Need at least 3 normal sessions. Found %d.' % len(normal))
        sys.exit(1)
    if len(anomaly) < 1:
        print('Need anomaly session. Found %d.' % len(anomaly))
        sys.exit(1)

    # Concatenate normals
    normal_data = torch.cat([t for _, t in normal], dim=0).double()
    n_normal = normal_data.shape[0]
    anomaly_data = anomaly[0][1].double()
    n_anomaly = anomaly_data.shape[0]
    print()
    print('Normal: %d frames, %d dims' % (n_normal, normal_data.shape[1]))
    print('Anomaly: %d frames' % n_anomaly)

    # ---- Log10 transform curvature dims ----
    normal_tr = log10_safe_transform(normal_data)
    anomaly_tr = log10_safe_transform(anomaly_data)

    # ---- Normalize: per-dim mean/std from normals ----
    mu = normal_tr.mean(dim=0)
    sd = normal_tr.std(dim=0).clamp(min=1e-6)
    normal_norm = (normal_tr - mu) / sd
    anomaly_norm = (anomaly_tr - mu) / sd

    print()
    print('Normal per-dim mean/std (log-space):')
    for i, name in enumerate(DIM_NAMES):
        print('  %s: mean=%+.4f  std=%.4f' % (name, mu[i].item(), sd[i].item()))

    # ---- Train RealNVP ----
    model = RealNVP(dim=6, n_layers=6, hidden_dim=64).double()
    opt = torch.optim.Adam(model.parameters(), lr=1e-3)
    print()
    print('Training RealNVP (%d layers, 64 hidden)...' % len(model.layers))
    for epoch in range(200):
        loss = train_epoch(model, opt, normal_norm, batch=256)
        if epoch % 20 == 0 or epoch == 199:
            with torch.no_grad():
                train_lp = model.log_prob(normal_norm).mean().item()
                anomaly_lp = model.log_prob(anomaly_norm).mean().item()
            print('epoch %3d: loss=%.4f  train_logp=%.4f  anomaly_logp=%.4f' % (
                epoch, loss, train_lp, anomaly_lp))

    # ---- Evaluate ----
    with torch.no_grad():
        normal_logp = model.log_prob(normal_norm)
        anomaly_logp = model.log_prob(anomaly_norm)

    # Threshold: normal 5th percentile
    threshold = torch.quantile(normal_logp, 0.05).item()

    anomaly_below = (anomaly_logp < threshold).sum().item()
    anomaly_pct = 100.0 * anomaly_below / n_anomaly

    print()
    print('=== RESULTS ===')
    print('Normal log p: mean=%.4f  std=%.4f  5th pct=%.4f' % (
        normal_logp.mean().item(), normal_logp.std().item(), threshold))
    print('Anomaly log p: mean=%.4f  std=%.4f  min=%.4f  max=%.4f' % (
        anomaly_logp.mean().item(), anomaly_logp.std().item(),
        anomaly_logp.min().item(), anomaly_logp.max().item()))
    print()
    print('Anomaly frames below normal 5th percentile: %d/%d = %.1f%%' % (
        anomaly_below, n_anomaly, anomaly_pct))
    print()
    if anomaly_pct > 50:
        print('RESULT: PASS — flow detects off-manifold curvature (meaningful)')
    elif anomaly_pct > 10:
        print('RESULT: MARGINAL — flow sees anomaly but not confidently')
    else:
        print('RESULT: FAIL — flow does not detect anomaly')

    return 0 if anomaly_pct > 50 else 1


if __name__ == '__main__':
    sys.exit(main())
