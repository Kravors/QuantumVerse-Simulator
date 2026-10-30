# Session Recording Manifest

Recorded sessions for training data pipeline validation. Binary frame data
(`frames.bin`) is gitignored; this manifest records the provenance of each
session so it can be re-recorded identically.

## Sessions

| Session | Dir | Metric | Frames | Duration | Command | Notes |
|---------|-----|--------|--------|----------|---------|-------|
| S1 | `a49d14f6151e318-session` | kerr | 600 | ~60 s sim | `--frames 60 --record 600 --metric kerr --time-scale 60.0` | Camera orbit (az 45°→90°), 10 M☉, spin 0.9 |
| S2 | `148d6cc66551e318-session` | kerr | 1200 | ~120 s sim | `--frames 120 --record 1200 --metric kerr --time-scale 60.0` | Camera orbit, 10 M☉, spin 0.9 |
| S3 | `40a8bf16851e318-session` | schwarzschild | 1200 | ~120 s sim | `--frames 120 --record 1200 --metric schwarzschild --time-scale 60.0` | Camera orbit, 10 M☉ |

## Recording parameters

- Sample rate: 10 Hz (frameInterval=6 at 60 FPS physics)
- Gate: `m_accumulatedTime -= interval` (remainder-preserving, machine-independent)
- All sessions recorded headless with Qt software rendering
- Camera orbit: cameraOrbitTimer sweeps azimuth by 0.05 rad/frame during recording

## Verification

- All body velocities physical (Earth ~29,789 m/s, Mercury ~47,885 m/s)
- Frame counts exact: 600/1200/1200 (proves `-=` gate)
- Camera azimuth varies per session (proves cameraOrbitTimer works under software rendering)

## Training smoke test results (python/smoke_test_training.py)

Masked dims: 24 active of 59 (31 degenerate: camera az/el/dist + mostly-static body dims)

| Baseline | Val Loss |
|----------|----------|
| Identity | 8.83e-05 |
| First-order (x + v·dt) | 3.46e-06 |
| **Model (MLP)** | **1.78e-04** |

- model / first-order ratio = **51.62** (model is 50x WORSE than kinematics)
- model / identity ratio = **2.02** (model is worse than trivial predictor)
- Conclusion: at 10 Hz, first-order extrapolation is a ~25x stronger baseline than identity, and the model cannot beat it. Do not record more sessions until the representation changes (delta prediction, longer horizon, or orbital elements).

## Re-recording

```
build\Release\quantumverse_qml.exe --frames <N> --record <M> --metric <m> --time-scale 60.0
```

where `<N>` = headless frames to render, `<M>` = max recording frames, `<m>` = kerr|schwarzschild.
