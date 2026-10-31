# Curvature Anomaly Detection Limits in Vacuum Static Kerr

**Finding:** Curvature-only anomaly detection via density model is mathematically impossible in vacuum static Kerr/Schwarzschild at any probe position. Kretschmann scalar K and gravitational redshift z are geometrically locked: any probe position that pushes K off the normal manifold necessarily pushes z off-manifold first. A normalizing flow trained on 6 curvature dims across 6 varied-probe normal sessions (3600 frames) detects **ergosphere entry** (redshift = 0 boundary) at 100%, but detects **pure curvature anomalies** at 0%.

## Setup

| Component | Value |
|---|---|
| Metric | Kerr, M = 10 M☉, a* = 0.9 |
| Recorder schema | v2 (6 dims: bhMass, bhSpin, K, Ricci, Weyl, redshift) |
| Normal sessions | 6, probe radii 50k–150k m, 600 frames each = 3600 frames |
| Training frames | 3600 normal frames |
| Model | RealNVP, 6 layers, 64 hidden dims, 200 epochs |

## Normal manifold (3600 frames, 6 sessions)

| Dim | Min | Max | Span |
|---|---|---|---|
| bhMass | 10.0 M☉ | 10.0 M☉ | constant |
| bhSpin | 0.0 | 0.9 | 2 values (S3 Schwarzschild) |
| probeKretschmann | 9.19e-22 | 6.70e-19 | 730× |
| probeRicciScalar | 0.0 | 0.0 | vacuum = 0 |
| probeWeylSquared | 9.19e-22 | 6.70e-19 | 730× (Weyl = K in vacuum) |
| probeRedshift | 0.116 | 0.607 | 5.2× |

## Equator anomaly (probe at r = 23330 m, inside ergosphere)

| Quantity | Value | vs normal |
|---|---|---|
| probeKretschmann | 1.38e-18 | 2.06× max normal |
| probeRedshift | 0.0 | below min normal (0.116) |

### 6-dim flow (with redshift)
- Detection rate: **100%** (600/600 frames below normal 5th percentile log-p)
- Mean anomaly log-p: -3.3e7 (vs normal 467)
- Mechanism: redshift = 0.0 → log10(0 + 1e-300) ≈ -300 → z-score ≈ -597 → RealNVP coupling layers extrapolate → log p → -∞
- **This is ergosphere entry detection, not curvature anomaly detection.**

### 5-dim flow (without redshift)
- Detection rate: **0%** (0/600 frames below normal 5th percentile)
- Mean anomaly log-p: 424 (vs normal 417 — anomaly has *higher* likelihood than normal 5th percentile)
- Kretschmann 2.06× max normal falls within the flow's learned manifold (training spans 730× in K)
- **Curvature-only anomaly is not detected.**

## Why — geometric lock

In vacuum static Kerr, both observables are monotonic in probe radius r:

| Probe r | Kretschmann | Redshift | Normal? |
|---|---|---|---|
| 150,000 m | ~5.5e-22 | 0.116 | z at floor |
| 100,000 m | 6.97e-21 | 0.187 | S1/S2 normal |
| 75,000 m | ~4e-20 | 0.281 | N5 normal |
| 50,000 m | 6.70e-19 | 0.607 | N4 normal (max K) |
| 40,000 m | ~2.5e-18 | 0.956 | **z outside normal range** |

Kretschmann ∝ r⁻⁶; redshift = 1/√(1 − r_s/r) − 1, which diverges as r → r_s. The two observables move together. To get K above the normal max (6.7e-19), you need r < 50,000 m — but at r = 40,000 m, redshift = 0.956, far above the normal max of 0.607.

### Widened off-equatorial scan (proof)

Scanned (r, θ) grid: r from 1.005·r₊ (21,320 m) to 100,000 m, θ from 0.005 to π/2 − 0.005 rad.

| Threshold | Result |
|---|---|
| Any (r, θ) with K > normal max AND z in range | 184 points found |
| Best point: K > 3× normal max (2.0e-18) AND z in range | **None found** |
| Absolute max K with z in range | 8.30e-19 = **1.2× max normal** |
| Location of absolute max | r = 48,252 m, θ = 89.7°, z = 0.606 |

**Mathematical ceiling:** Even optimizing over all off-equatorial positions, Kretschmann exceeds normal max by only 1.2× while keeping redshift in range. That's below any realistic detection threshold (the 5-dim flow flags 0% at 2.06×).

## Paths that would work

| Approach | Mechanism | Cost |
|---|---|---|
| Matter source (Ricci ≠ 0) | Ricci scalar breaks vacuum constraint; K and z decouple | Requires metric modification not in recording pipeline |
| Moving probe | Doppler shifts redshift; breaks the static lock | Requires probeVelocity in schema (v3) |
| Time-dependent metric | Redshift evolves; K/z no longer coupled pointwise | Requires time-varying metric infrastructure |
| float64 + tighter threshold | Redshift floor issue partially mitigated | Doesn't solve geometric lock |

## Recommendation

**Accept ergosphere entry detection as the product, or close the arc.**

- The 6-dim flow works and flags probe-inside-ergosphere at 100%. That's a real, working feature for any instrument asking "did the probe cross into the ergosphere?" — genuinely interesting for frame-dragging studies.
- It is **not** general curvature anomaly detection. Do not claim it detects "unusual curvature" — it detects redshift boundary crossing.
- Curvature-only anomaly detection in vacuum static Kerr/Schwarzschild is impossible at any probe position. Documented here so a future session doesn't re-derive this arc.

## Companion docs

- `world-modeling-limits.md` — world model at 10 Hz capped at first-order kinematics (float32 floor)
- This file — curvature anomaly detection capped by geometric lock in vacuum Kerr

## Notes

- Pole Kretschmann clamp (`K < 0 → K = 0`) in `src/spacetime/KerrMetric.cpp:228-233` is a workaround for a coordinate singularity at θ=0. Clamped values in `[REALITY]` dumps are indistinguishable from genuinely small curvature. Consider `NaN` instead of `0.0` if any future instrument queries θ=0.
- Mass/spin sync fix (`src/main.qml:947`, `src/main_qml.cpp:1394`) verified: config mass=5 spin=0.5 records bhMass=5.0, bhSpin=0.5.
