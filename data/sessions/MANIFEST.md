# Session Recording Manifest

Recorded sessions for training data pipeline validation. Binary frame data
(`frames.bin`) is gitignored; this manifest records the provenance of each
session so it can be re-recorded identically.

## Sessions

| Session | Dir | Metric | Frames | Duration | Command | Notes |
|---------|-----|--------|--------|----------|---------|-------|
| S1 | `6479d88f564ee318-session` | kerr | 1200 | ~120 s sim | `--frames 120 --record 1200 --metric kerr --time-scale 60.0` | Static camera, 10 M☉, spin 0.9 |
| S2 | `805e91764c4ee318-session` | kerr | 1200 | ~120 s sim | `--frames 120 --record 1200 --metric kerr --time-scale 60.0` | Static camera, 10 M☉, spin 0.9 |
| S3 | `c23ecd11d4ee318-session` | schwarzschild | 600 | ~60 s sim | `--frames 60 --record 600 --metric schwarzschild --time-scale 60.0` | Static camera, 10 M☉ |

## Recording parameters

- Sample rate: 10 Hz (frameInterval=6 at 60 FPS physics)
- Gate: `m_accumulatedTime -= interval` (remainder-preserving, machine-independent)
- All sessions recorded headless with Qt software rendering
- Camera is static (az=45°, el=30°, distance=91.856) — GL orbit hook did not fire in software mode

## Verification

- S1 body velocities: Earth 29,789 m/s, Mercury 47,885 m/s, Sun 0 m/s (matches N-body solver)
- Frame counts exact: 600/1200/1200 (proves `-=` gate, not `= 0`)
- Training smoke test: val/identity = 0.80 (absolute), 0.75 (delta)

## Re-recording

```
build\Release\quantumverse_qml.exe --frames <N> --record <M> --metric <m> --time-scale 60.0
```

where `<N>` = headless frames to render, `<M>` = max recording frames, `<m>` = kerr|schwarzschild.
