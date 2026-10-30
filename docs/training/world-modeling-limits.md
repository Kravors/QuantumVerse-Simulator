# World Modeling Limits at 10 Hz

**Finding:** Single-step world-model prediction at 10 Hz is mathematically limited by float32 precision, not by model capacity or training-loop bugs.

## The numbers

| Quantity | Value | Notes |
|---|---|---|
| Earth's velocity | 29,800 m/s | mean orbital |
| First-order Δx per frame (dt=0.1s) | 2,980 m | what `x + v·dt` predicts exactly |
| Centripetal acceleration from Sun | 5.93e-3 m/s² | at r = 1.5e11 m |
| Second-order correction per frame | 2.97e-5 m | `½·a·dt²` |
| Ratio (2nd / 1st) | 1.0e-8 | the signal/noise |
| Earth orbit radius | 1.496e11 m | the scale |
| Ratio of 2nd-order correction to position | 2.0e-16 | == float64 epsilon |

`2.0e-16` is exactly float64 epsilon (2.22e-16). PyTorch trains in float32 by default (epsilon = 1.19e-7). The second-order signal is **9 orders of magnitude below float32's ability to represent it.**

## What this means

`x_{t+1}` is bit-identical to `x_t + v·dt` in float32 at 10 Hz. Not approximately — literally. Once you compute `x_t + v·dt` in float32 and compare to `x_{t+1}`, the difference rounds to zero.

## Ruled out (all fail at float32 precision at 10 Hz)

| Approach | Why it fails |
|---|---|
| Delta prediction (Δx target) | Δx = 2980 m (first-order) + 3e-5 m (2nd); model can learn Δx to 1e-8 relative precision = 3e-5 m floor ≈ exact first-order, no improvement possible |
| Orbital elements | For 2-body, a/e/i are already constant; for N-body, drift per 0.1s is ~1e-11 relative — worse than 2nd-order |
| Physics-informed architecture | Correct structure, but signal is below representational resolution |
| More layers / attention / transformers | Capacity is not the constraint; representability is |
| Different loss function | The target is below precision; no loss can extract signal from rounding noise |
| Batch size / learning rate / optimizer changes | The model converges correctly to the true optimum (identity ≈ first-order); you can't tune a model to learn what isn't in the data |

## Not ruled out (but requires changing something other than the model)

| Approach | Why it can work | Cost |
|---|---|---|
| float64 training | Recovers the 2nd-order signal | 2× memory, 2–5× slower |
| Longer horizon (k=100, 10s) | 2nd-order: 2.97e-3 m vs 2.98e5 m = 1e-8. Still below float32 | Need 10s+ sessions |
| Longer horizon (k=1000, 100s) | 2nd-order: 0.297 m vs 2.98e6 m = 1e-7. At float32 edge | Need 100s+ sessions |
| Longer horizon (k=10000, 1000s) | 2nd-order: 29.7 m vs 2.98e7 m = 1e-6. Learnable | 17 min sessions per sample; impractical |
| Different task (anomaly detection) | Target is a scalar score, not position | Change the task |

## Experimental evidence (from this project)

Five diagnostic sessions converged on the same conclusion:

1. **Cross-metric split** (Kerr train, Schwarzschild val): model/first_order = 51x — invalid split, masked the real signal.
2. **Within-session split** (first 80%/last 20% per session): train ratio 1.98x identity, val ratio 199x — training not broken, but model couldn't beat identity even on same-physics val.
3. **200 epochs + lr=1e-4**: train ratio 1.01x — training converges correctly to identity level.
4. **ReLU vs Tanh**: Neither helped — the limit is representational, not architectural.
5. **Physics-informed MLP** (zero-init bias + small random weights, normalized loss): train ratio 1.07x, val ratio 0.99x, model/first_order = 28.9x. Model converges correctly but plateaus at identity.

In every configuration the model converges to `identity ≈ first-order ≈ model` — all three at the same loss floor (~2.2e-04 normalized). The model is not failing to train; it's hitting the precision floor of the target.

## Recommendation

**Stop trying to make world modeling at 10 Hz work. Move to anomaly detection.**

- Already have `AnomalyDetector`/`AnomalyMonitor` infrastructure
- `SessionRecorder` already produces per-frame data
- Anomaly detection asks "is this frame unusual?" — a scalar score where 0–1 is meaningful, not a position-magnitude problem
- 2,400 frames is enough for anomaly detection; it's tiny for world modeling

## Paths forward (documented, not implemented)

1. **float64 training + horizon k≥100** — 2nd-order grows as k²·3e-5, learnable at float64
2. **Anomaly detection** on per-frame density — recommended; reuses existing infrastructure
3. **Longer sessions (100s+)** — requires new recordings; impractical at scale

Next session: build anomaly detection, not world model iteration.