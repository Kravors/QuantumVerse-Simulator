#!/usr/bin/env python3
"""
python/anomaly_baseline.py — anomaly detection baseline via Mahalanobis + log-diff.

Loads v2 schema sessions (64 dims: 59 kinematic/meta + bhSpin + 4 probe
curvature scalars), computes per-session anomaly signal on two dim subsets:

  masked  — dims that vary across normal data (std >= 1e-3): z-score
  degen   — dims constant in normal (std < 1e-3): log10-difference for
            curvature dims, presence-check for zero-valued dims

For each anomaly session reports:
  1. Aggregate Mahalanobis ratio (masked dims)
  2. Max per-dim z-score (masked dims)
  3. Max log10-difference (degen curvature dims)

Decision gate: any of the three > threshold → green light for
CurvatureNormalizingFlow.

Run:  python python/anomaly_baseline.py
"""

import struct, os, math, sys
from pathlib import Path

import torch
import numpy as np


# ---------------------------------------------------------------------------
# Binary reader for SessionRecorder frames.bin (schema v2)
# Layout per frame:
#   uint32 frame_len
#   double time
#   uint32 bodyCount
#   per body: uint32 nameLen + name bytes, double pos[3], double vel[3]
#   double cameraPosition[4], double cameraLookAt[4]
#   double blackHoleMassKg
#   double blackHoleSpin
#   double probeKretschmann, probeRicciScalar, probeWeylSquared, probeRedshift
#   double totalEnergy, double kineticEnergy, double potentialEnergy
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
            raise ValueError('schema v1 not supported by this reader; use v1 reader')
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
            cam = struct.unpack_from('<dddd', buf, p); p += 32
            p += 32  # cameraLookAt
            bhMass = struct.unpack_from('<d', buf, p)[0]; p += 8
            bhSpin = struct.unpack_from('<d', buf, p)[0]; p += 8
            probeKret = struct.unpack_from('<d', buf, p)[0]; p += 8
            probeRicci = struct.unpack_from('<d', buf, p)[0]; p += 8
            probeWeyl = struct.unpack_from('<d', buf, p)[0]; p += 8
            probeRedshift = struct.unpack_from('<d', buf, p)[0]; p += 8
            te, ke, pe = struct.unpack_from('<ddd', buf, p); p += 24
            frames.append({
                'time': time_val,
                'bodies': list(zip(body_names, positions, velocities)),
                'camera': cam,
                'bhMass': bhMass,
                'bhSpin': bhSpin,
                'probeKretschmann': probeKret,
                'probeRicciScalar': probeRicci,
                'probeWeylSquared': probeWeyl,
                'probeRedshift': probeRedshift,
                'energy': (te, ke, pe),
            })
    return frames


def session_to_tensor(frames):
    """Flatten each frame to 64-dim vector:
       [t, bhMass, cam_x, cam_y, cam_z, bhSpin,
        probeKretschmann, probeRicciScalar, probeWeylSquared, probeRedshift,
        then per body: x,y,z,vx,vy,vz × 9 bodies]
       Total: 5 + 1 + 4 + 54 = 64 dims."""
    vecs = []
    for fr in frames:
        bodies = sorted(fr['bodies'], key=lambda b: b[0])
        v = ([fr['time'], fr['bhMass']] + list(fr['camera'][:3]) +
             [fr['bhSpin'],
              fr['probeKretschmann'], fr['probeRicciScalar'],
              fr['probeWeylSquared'], fr['probeRedshift']])
        for _, pos, vel in bodies:
            v += list(pos) + list(vel)
        vecs.append(v)
    return torch.tensor(vecs, dtype=torch.float64)


def load_sessions(sessions_dir, names):
    sessions_dir = Path(sessions_dir)
    out = []
    for name in names:
        sdir = sessions_dir / name
        if not (sdir / 'frames.bin').exists():
            print('WARNING: %s/frames.bin not found, skipping' % sdir)
            continue
        frames = read_frames(str(sdir / 'frames.bin'))
        t = session_to_tensor(frames)
        out.append((name, t))
        print('  %s: %d frames, dim=%d' % (name, t.shape[0], t.shape[1]))
    return out


# Tensor layout constants (matches session_to_tensor, schema v2)
HEADER_DIMS = 5     # t, bhMass, cam_x, cam_y, cam_z
SPIN_DIMS = 1       # bhSpin
PROBE_DIMS = 4      # probeKretschmann, probeRicciScalar, probeWeylSquared, probeRedshift
BODY_DIMS = 6       # per body: x,y,z,vx,vy,vz
DT = 0.1            # 10 Hz recording interval

# Dim index ranges
IDX_T = 0
IDX_BHMASS = 1
IDX_CAM_X = 2
IDX_CAM_Y = 3
IDX_CAM_Z = 4
IDX_BHSPIN = 5
IDX_PROBE_KRET = 6
IDX_PROBE_RICCI = 7
IDX_PROBE_WEYL = 8
IDX_PROBE_REDSHIFT = 9
IDX_BODIES_START = 10


def ledoit_wolf_shrinkage(X):
    n, d = X.shape
    Xc = X - X.mean(axis=0, keepdims=True)
    sample_cov = (Xc.T @ Xc) / (n - 1)
    mu = np.trace(sample_cov) / d
    target = mu * np.eye(d)
    diff = sample_cov - target
    numerator = np.sum(diff ** 2)
    denom_base = np.cov(Xc, rowvar=False, bias=True)
    denominator = np.sum((sample_cov - denom_base) ** 2) + 1e-12
    alpha = np.clip(numerator / denominator, 0.0, 1.0)
    shrunk = alpha * target + (1.0 - alpha) * sample_cov
    return shrunk


def mahalanobis_distances(X, mu, cov_inv):
    d = X - mu
    return np.sqrt(np.einsum('ni,ij,nj->n', d, cov_inv, d))


def log10_safe(x):
    """log10 with guard for zero/negative: returns -300 for non-positive."""
    return np.where(x > 0, np.log10(x), -300.0)


def main():
    sessions_dir = 'data/sessions'

    # Normal sessions: S1-v2, S2-v2 (Kerr), S3-v2 (Schwarzschild)
    normal_names = [
        '5c896a226f77e318-session',   # S1-v2 normal Kerr
        '08ea8ad7a77e318-session',   # S2-v2 normal Kerr
        'b09e244b8777e318-session',  # S3-v2 normal Schwarzschild
    ]
    # Anomaly sessions: probe at equator near horizon (clamped to 3M)
    anomaly_sessions = [
        ('probe-equator', 'fc78a0a9ef77e318-session'),   # K = 1.38e-18 (positive)
        ('probe-pole-clamped', 'a8ed7de3c677e318-session'),  # K = -1.89e-19 (clamped pole)
    ]

    print('Loading normal sessions...')
    normal = load_sessions(sessions_dir, normal_names)
    print('Loading anomaly sessions...')
    anomalies = load_sessions(sessions_dir, [n for _, n in anomaly_sessions])

    if len(normal) < 2:
        print('Need at least 2 normal sessions. Found %d.' % len(normal))
        sys.exit(1)
    if len(anomalies) < 1:
        print('Need at least 1 anomaly session. Found %d.' % len(anomalies))
        sys.exit(1)

    # Concatenate normal data
    normal_x = torch.cat([t for _, t in normal], dim=0).double()
    n_normal = normal_x.shape[0]
    print()
    print('Normal: %d frames total, dim=%d' % (n_normal, normal_x.shape[1]))

    # Determine degenerate vs non-degenerate dims from normal data
    mu_normal = normal_x.mean(dim=0)
    sd_normal = normal_x.std(dim=0)
    keep_mask = sd_normal >= 1e-3  # non-degenerate
    masked_idx = keep_mask.nonzero().flatten().tolist()
    degen_idx = (~keep_mask).nonzero().flatten().tolist()
    print('Non-degenerate (masked): %d dims' % len(masked_idx))
    print('Degenerate: %d dims' % len(degen_idx))
    print('  Degenerate dim indices:', degen_idx)

    # Dim names for reporting (64 dims, schema v2)
    dim_names = []
    dim_names.append('t')
    dim_names.append('bhMass')
    dim_names.append('cam_x')
    dim_names.append('cam_y')
    dim_names.append('cam_z')
    dim_names.append('bhSpin')
    dim_names.append('probeKretschmann')
    dim_names.append('probeRicciScalar')
    dim_names.append('probeWeylSquared')
    dim_names.append('probeRedshift')
    n_bodies = 9
    for b in range(n_bodies):
        dim_names.append('body%d_x' % b)
        dim_names.append('body%d_y' % b)
        dim_names.append('body%d_z' % b)
        dim_names.append('body%d_vx' % b)
        dim_names.append('body%d_vy' % b)
        dim_names.append('body%d_vz' % b)

    # Convert to numpy
    normal_np = normal_x.numpy()
    mu_np = mu_normal.numpy()
    sd_np = sd_normal.numpy()

    # ---- Masked (non-degenerate) dims: Mahalanobis + z-score ----
    normal_masked = normal_np[:, masked_idx]
    mu_masked = mu_np[masked_idx]
    cov_masked = ledoit_wolf_shrinkage(normal_masked)
    cov_masked += 1e-8 * np.eye(len(masked_idx))
    cov_inv_masked = np.linalg.inv(cov_masked)

    # ---- Degenerate dims: per-dim log10-difference ----
    # For each degenerate dim: signal = log10(|anomaly_mean|) - log10(|normal_mean|)
    # If normal_mean == 0 and anomaly_mean != 0: signal = large (presence)
    # If both == 0: signal = 0 (no difference)
    mu_degen = mu_np[degen_idx]

    results = {}
    for anomaly_name, anomaly_path in anomaly_sessions:
        anomaly_t = None
        for name, t in anomalies:
            if name == anomaly_path:
                anomaly_t = t
                break
        if anomaly_t is None:
            print('WARNING: anomaly session %s not found' % anomaly_path)
            continue

        anomaly_np = anomaly_t.numpy()
        anomaly_mean = anomaly_np.mean(axis=0)

        print()
        print('=== %s anomaly ===' % anomaly_name.upper())

        # ---- Masked: Mahalanobis ratio ----
        a_masked = anomaly_np[:, masked_idx]
        m_masked = mahalanobis_distances(a_masked, mu_masked, cov_inv_masked)
        normal_masked_mahal = mahalanobis_distances(normal_masked, mu_masked, cov_inv_masked)
        ratio_masked = float(m_masked.mean() / normal_masked_mahal.mean())
        print('  1. masked Mahalanobis ratio: %.2fx  (anomaly=%.3f, normal=%.3f)' % (
            ratio_masked, m_masked.mean(), normal_masked_mahal.mean()))

        # ---- Masked: max per-dim z-score ----
        sd_masked = sd_np[masked_idx]
        sd_floored = np.maximum(sd_masked, 1e-6)
        z_scores = np.abs((anomaly_mean[masked_idx] - mu_masked) / sd_floored)
        max_z_idx = int(np.argmax(z_scores))
        max_z = float(z_scores[max_z_idx])
        max_z_dim = dim_names[masked_idx[max_z_idx]]
        print('  2. max per-dim z: %.1f at %s' % (max_z, max_z_dim))

        # ---- Degenerate: max log10-difference ----
        mu_degen_vals = mu_degen
        anomaly_degen_vals = anomaly_mean[degen_idx]
        log_normal = log10_safe(np.abs(mu_degen_vals))
        log_anomaly = log10_safe(np.abs(anomaly_degen_vals))
        log_diff = log_anomaly - log_normal
        max_log_diff_idx = int(np.argmax(np.abs(log_diff)))
        max_log_diff = float(log_diff[max_log_diff_idx])
        max_log_diff_dim = dim_names[degen_idx[max_log_diff_idx]]
        print('  3. max log10-diff: %.2f at %s (normal=%.3e, anomaly=%.3e)' % (
            max_log_diff, max_log_diff_dim,
            mu_degen_vals[max_log_diff_idx], anomaly_degen_vals[max_log_diff_idx]))

        # Also report probe-specific dims
        probe_dim_names = ['probeKretschmann', 'probeRicciScalar',
                           'probeWeylSquared', 'probeRedshift']
        probe_dim_idx = [IDX_PROBE_KRET, IDX_PROBE_RICCI, IDX_PROBE_WEYL,
                         IDX_PROBE_REDSHIFT]
        for pname, pidx in zip(probe_dim_names, probe_dim_idx):
            nv = mu_np[pidx]
            av = anomaly_mean[pidx]
            ld = log10_safe(abs(av)) - log10_safe(abs(nv))
            print('     %s: normal=%.3e anomaly=%.3e log10-diff=%.2f' % (
                pname, nv, av, ld))

        results[anomaly_name] = {
            'ratio_masked': ratio_masked,
            'max_z': max_z,
            'max_z_dim': max_z_dim,
            'max_log_diff': max_log_diff,
            'max_log_diff_dim': max_log_diff_dim,
        }

    # ---- Summary ----
    print()
    print('=== SUMMARY ===')
    for anomaly_name in ['probe-equator', 'probe-pole-clamped']:
        r = results.get(anomaly_name, {})
        print('%s:' % anomaly_name.upper())
        print('  1. masked Mahalanobis ratio: %.2fx' % r.get('ratio_masked', float('nan')))
        print('  2. max per-dim z: %.1f at %s' % (
            r.get('max_z', float('nan')), r.get('max_z_dim', '?')))
        print('  3. max log10-diff: %.2f at %s' % (
            r.get('max_log_diff', float('nan')), r.get('max_log_diff_dim', '?')))

    # ---- Decision gate ----
    print()
    print('=== Decision gate ===')
    green = False
    for anomaly_name in ['probe-equator', 'probe-pole-clamped']:
        r = results.get(anomaly_name, {})
        rm = r.get('ratio_masked', 0)
        mz = r.get('max_z', 0)
        mld = abs(r.get('max_log_diff', 0))
        # Gate: any of the three exceeds threshold
        if rm > 3.0 or mz > 50 or mld > 2.0:
            print('%s: GREEN (masked_ratio=%.2f, max_z=%.1f, max_log10_diff=%.2f)' % (
                anomaly_name.upper(), rm, mz, mld))
            green = True
        else:
            print('%s: below gate (masked_ratio=%.2f, max_z=%.1f, max_log10_diff=%.2f)' % (
                anomaly_name.upper(), rm, mz, mld))
    if green:
        print('RESULT: Green light for CurvatureNormalizingFlow')
    else:
        print('RESULT: No anomaly detected above gate')

    return 0 if green else 1


if __name__ == '__main__':
    sys.exit(main())
