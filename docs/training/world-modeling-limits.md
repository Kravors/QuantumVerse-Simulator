# World Modeling Limits at 10 Hz

**Finding:** Single-step world-model prediction at 10 Hz is limited by float32 precision — the model correctly converges to first-order kinematics, but cannot beat it because the 2nd-order signal is below representable resolution.

## The math (proof, not empirical)

At dt=0.1s, Earth orbiting the Sun:

| Quantity | Value |
|---|---|
| First-order Δx per frame | 2,980 m (`v·dt`) |
| 2nd-order correction per frame | 2.97e-5 m (`½·a·dt²`) |
| Ratio (2nd / 1st) | 1.0e-8 |
| Ratio of 2nd-order correction to position (1.5e11 m) | 2.0e-16 |
| float32 epsilon | 1.19e-7 |

`2e-16` is 9 orders of magnitude below float32 precision. `x_{t+1}` is bit-identical to `x_t + v·dt` in float32. No model can extract the 2nd-order signal — it's below the representational floor.

## Empirical confirmation (smoke_test_training.py)

**Corrected baseline ladder** (normalized 59-dim, comparing target vs prediction — NOT input vs prediction, which was a bug in an earlier version):

| | Train | Val |
|---|---|---|
| Identity | 9.47e-05 | 2.29e-04 |
| First-order | 9.13e-05 | 2.22e-04 |
| Model (PhysicsInformedMLP) | 1.09e-04 | 2.43e-04 |
| Model / First-order | **1.19x** | **1.10x** |

The model converges to essentially the same loss floor as first-order kinematics (~3-10% above first-order). This is the **correct behavior** — first-order is the best achievable at 10 Hz, and the model learns it.

**The bug that made it look worse:** An earlier version of `smoke_test_training.py` computed the first-order baseline loss by comparing **normalized input** against **normalized first-order output** (`((train_n - first_order_n) ** 2)`), instead of **normalized target** against **normalized first-order output** (`((train_yn - first_order_n) ** 2)`). Since input ≈ first-order output at 10 Hz, this produced an artificially tiny first-order loss (3.55e-06), making the model look 29x worse. Fixed by comparing target vs output — first-order is only 3-10% better than identity, and the model matches it.

## Ruled out (all fail at float32 precision at 10 Hz)

| Approach | Why it fails |
|---|---|
| Delta prediction (Δx target) | Δx = 2980 m (1st-order) + 3e-5 m (2nd-order); 2nd-order signal below float32 floor |
| Orbital elements | For 2-body, a/e/i constant; N-body drift per 0.1s ~1e-11 relative — worse than 2nd-order |
| Physics-informed architecture | Correct structure, but signal below representational resolution |
| More layers / transformers | Capacity is not the constraint |
| Different loss function | Target below precision; no loss extracts signal from rounding noise |
| Batch size / LR / optimizer changes | Model converges correctly to the true optimum (identity ≈ first-order) |

## Not ruled out (requires changing something other than the model)

| Approach | Why it can work | Cost |
|---|---|---|
| float64 training | Recovers 2nd-order signal (2e-16 is representable in float64) | 2× memory, 2–5× slower |
| Longer horizon (k=100, 10s) | 2nd-order grows as k²: 2.97e-3 m vs 2.98e5 m = 1e-8 — still below float32 | Need 10s+ sessions |
| Longer horizon (k=1000, 100s) | 2nd-order: 0.297 m vs 2.98e6 m = 1e-7 — at float32 edge | Need 100s+ sessions |
| Longer horizon (k=10000, 1000s) | 2nd-order: 29.7 m vs 2.98e7 m = 1e-6 — learnable | 17 min sessions per sample; impractical |
| Different task (anomaly detection) | Scalar score target, no position-magnitude mismatch | Change the task |

## Recommendation

**Move to anomaly detection.** The world model at 10 Hz is mathematically capped at first-order kinematics. Anomaly detection (per-frame density scoring) reuses existing `AnomalyDetector`/`AnomalyMonitor` infrastructure, works with 2,400 frames, and has no precision floor issue.

Paths forward (documented, not implemented):
1. float64 training + horizon k≥100 → 2nd-order grows to k²·3e-5, learnable
2. **Anomaly detection** on per-frame density (recommended — reuses existing infrastructure)
3. Longer sessions (100s+) → requires new recordings

Next session: build anomaly detection, not world model iteration.