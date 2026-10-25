# QuantumVerse Phase 3.5 — Curvature Normalizing Flow for Anomaly Detection

**Date**: 2026-10-24  
**Task**: 3.5 Normalising Flow for Anomaly Detection  
**Status**: DESIGN  
**Gate**: CP3.3 — Anomaly detector >95% TP rate

---

## 1. Problem statement

`DiscoveryEngine` has no working anomaly detector today. The normalizing flow
(`CurvatureNormalizingFlow`) exists as a stub (returns 0.0 if model unavailable).
Task 3.5 trains a flow on curvature statistics, exports it to ONNX, and wires
C++ inference so the detector can flag >3σ anomalies in real time.

The detector has three mechanisms, not one:

1. **Flow** — normalizing flow on per-point curvature invariants; scores continuous anomalies (strong-field spikes, naked singularities, signature flips, Ricci excursions)
2. **probeValid gate** — boolean pre-filter; catches NaN/Inf injection (not flow-compatible)
3. **Differential detector** — cross-sample statistic `|ΔK/ΔM|`; catches mass transitions (per-sample flow can't)

This design doc commits to acceptance criteria *before* implementation, so the
model isn't reverse-engineered from whatever it happens to do.

---

## 2. Q1: Feature representation

**Decision: per-point invariants + spatial aggregation, with residual features for Schwarzschild.**

Features per probe point:

| Feature | Transform | Rationale |
|---------|-----------|-----------|
| `log|K| - log|K_expected|` | residual vs analytic GR | **Primary feature.** Captures deviation from Schwarzschild GR at fixed (M, r). Catches modified-gravity metrics at normal parameters. |
| `log|R|` | sign-preserved log of Ricci scalar | Vacuum metrics (Schwarzschild/Kerr) have R=0; deviations indicate non-vacuum physics |
| `log|W|²` | log of Weyl norm squared | Captures tidal structure; for vacuum metrics W² ≈ K |
| `g_tt` | raw | Signature flip shows up here (g_tt > 0 = invalid) |
| `probeValid` | boolean | Mask invalid points (r ≤ r_s); don't model them |

`K_expected = 48·M²/r⁶` (analytic Schwarzschild Kretschmann, returned by
`compute_curvature_features()` alongside raw K). For Kerr/RN training data,
`K_expected` is not analytically available; use raw `log|K|` for those samples.

The flow models the joint `p(log|K| - log|K_expected|, log|R|, log|W|², g_tt | probeValid)`
for Schwarzschild samples, and `p(log|K|, log|R|, log|W|², g_tt | probeValid)`
for Kerr/RN samples. Anomaly score per point = `-log p`. Spatial aggregation =
**95th percentile of per-point scores** across all probe points in a sample
(chosen over max because max is sensitive to single-point numerical noise; 95th
percentile is robust while still catching localized anomalies).

**Why residuals:** Raw `log|K|` spans orders of magnitude with mass (M ∈ [1, 50]).
A modified-gravity metric at M=10 with K smaller than GR prediction might fall
within the healthy distribution for M=50. Residuals remove this mass dependence,
so the flow detects **curvature anomalies at normal parameters**, not just
**parameter excursions**. If the first training run shows held-out AUC > 0.75
on `signature_flip` + `ricci_excursion` with raw features, residuals can be
dropped — but they're the safer default.

**Not chosen**: local neighborhood patch (fixed radius is an unpruned design
choice; adds a hyperparameter we'd need to justify). **Not chosen**: slice as
image (64² is high-dim, slow inference, hard to interpret; defer to Phase 4 if
per-point proves insufficient).

---

## 3. Q2: Healthy distribution spec

Must be *specified and reproducible*. Every parameter is a choice that affects
the model; this section pins them.

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| Metric families | Schwarzschild, Kerr (a ∈ [0, 0.99]M), RN (Q ∈ [0, 0.99]M) | Covers rotating/charged cases within physical bounds |
| Mass range | M ∈ [1, 50] M☉ | Matches Neural ODE training distribution [5, 50] |
| Probe radius | r ∈ [1.5·r_s, 100·r_s], uniform in log-r | Outside horizon; log-r samples strong field densely |
| Points per sample | 1000 | Sufficient for spatial aggregation stats |
| Sample count | 10⁴ healthy samples → 10⁷ points | Large enough to estimate tails |
| Train seed | 42 (fixed) | Reproducible |
| Test seed | 12345 (fixed, different from train) | Held-out evaluation |
| Validation split | 10% of train | Early stopping / hyperparameter tuning |

**Why M ∈ [1, 50]:** matches the Neural ODE training distribution per
`EXECUTION_TRACKING.md`, so the flow and neural ODE share a consistent domain.

**Why r starts at 1.5·r_s:** inside the horizon (r ≤ r_s) metrics are signature-invalid
and masked by `probeValid`. Starting at 1.5·r_s avoids the degenerate near-horizon
region while still capturing strong-field curvature.

**Data source:** Every feature value is computed by the simulator, not a formula.
For each sample, construct the metric (Schwarzschild/Kerr/RN at the specified
M, a, Q), place 1000 probe points at the specified radii, and call
`CurvatureCalculator::computeAll(metric, probe_event)` to get K, R, W², g_tt at
each point. This guarantees the flow trains on *simulator* output, not analytical
approximations — eliminating train/test distribution shift at inference time
(see Flag A, §Q3 anomaly generation follows the same rule).

---

## 4. Q3: Anomaly taxonomy — split into flow categories, probeValid gate, differential detector

Not every anomaly is a flow category. The flow models continuous features
`p(log|K|, log|R|, log\|W\|², g_tt | probeValid)` — anything that breaks that
model (NaN, or cross-sample discontinuity) is a separate mechanism.

### 4.1 Flow categories (train the flow on these)

Four categories, ≥500 samples each → ≥2000 anomaly samples total:

| Category | Generator (simulator-based) | Label | Rationale |
|----------|----------------------------|-------|-----------|
| Near-horizon K-spike | M=10 Schwarzschild, probe at r ∈ [1.05, 1.2]·r_s | `near_horizon_spike` | Strong-field but physically valid — tests model resolution |
| Naked singularity | Kerr with a > 1.05M; RN with Q > 1.05M | `naked_singularity` | Signature-invalid metric — tests validity checking |
| Signature flip | g_tt > 0 at r > 2·r_s (build metric, then flip sign) | `signature_flip` | Structural violation — `g_tt < 0` in exterior |
| Near-singularity Ricci excursion | Dilaton metric at M=50, probe at r ∈ [1.01, 1.05]·r_s | `ricci_excursion` | Dilaton field raises Ricci scalar R at large r (R ≠ 0 in dilaton vacuum, unlike GR vacuum R=0). Tests detection of modified-gravity curvature at normal mass. |

All anomaly generators follow Flag A: build the metric through the simulator,
then evaluate `CurvatureCalculator::computeAll()` at probe points. The flow
never sees synthetic feature values.

### 4.2 probeValid gate (not a flow category)

| Mechanism | Generator | Label | Rationale |
|-----------|-----------|-------|-----------|
| NaN / Inf injection | Replace K with NaN at random 5% of points | `nan_injection` | Implementation-bug simulation — `probeValid` catches it |

NaN propagates through the flow as NaN; `-log p` is undefined. Instead of a
flow category, NaN detection is a **pre-flow gate**: `if isnan(any feature) →
emit {category: "invalid", score: ∞}, skip flow`. 100% recall on NaN injection,
<0.1% FPR on healthy data.

### 4.3 Differential detector (not a flow category)

| Mechanism | Generator | Label | Rationale |
|-----------|-----------|-------|-----------|
| Mass transition | M sweep 1→50 in 0.1 steps, flag step discontinuities | `mass_transition` | Phase-boundary / regime-change detection |

A per-sample flow can't detect mass transition — each sample individually looks
healthy. Detection requires a temporal/differential statistic:
`|K(M_{n+1}) - K(M_n)| / |ΔM|`, flagged where this exceeds a threshold.
Separate code path, separate acceptance criterion.

### 4.4 Sample counts

- Flow categories: ≥500 samples per category (≥2000 total anomaly)
- Healthy: 10⁴ samples → 10⁷ points
- probeValid gate: measured on the same healthy + NaN samples
- Differential detector: measured on the M sweep sequence

If resources allow, scale flow categories to 2000 per category.

---

## 4.5 Q4: Acceptance criteria

All of the following must pass for CP3.3:

| Criterion | Value | Role | Rationale |
|-----------|-------|------|-----------|
| Flow per-category AUC (each of 4) | > 0.88 | **Hard gate** — fail if any category below | Per-category is the *informative* test; it tells you whether one anomaly family is quietly failing while others carry the pooled score |
| Flow per-category AUC (each of 4) | > 0.92 | Target — report, not gate | Combined AUC is the weighted average; if every category clears 0.88, combined will typically be ~0.90–0.95 |
| Flow held-out category AUC (each of 2) | > 0.75 | **Generalization gate** | Train on near-horizon spike + naked singularity; hold out signature flip + Ricci excursion |
| Flow combined AUC (4 categories pooled) | Report only | Headline metric | Not gated — per-category gate is stricter and more informative |
| probeValid gate recall on NaN injection | 100% | Hard gate | Every NaN sample flagged invalid |
| probeValid gate FPR on healthy data | < 0.1% | Hard gate | Nearly zero false alarms on healthy |
| Differential detector AUC on mass-transition | > 0.90 | Hard gate | Separate mechanism, separate criterion |

**Named held-out split:** Train the flow on healthy data only. Hold out `signature_flip` + `ricci_excursion` anomalies — never seen during training or threshold calibration.

**Why this split:** The signature flip (`g_tt > 0` at `r > 2·r_s`) is a *structural* violation, not a magnitude excursion. If the flow catches it without training on it, it learned that `g_tt < 0` in the exterior is a structural constraint — that's the real test of learning curvature structure.

**The held-out test is the killer test.** If every per-category AUC clears 0.88 but held-out is 0.60, the model memorized generators, not curvature. Gate fails.

Also required: anomaly score calibration. At the **99th percentile of healthy
validation scores** (score threshold chosen on validation set), false positive
rate on healthy data must be < 1%. Document the threshold choice.

---

## 4.6 Training protocol — flow trains on healthy only

Standard normalizing-flow anomaly detection protocol:

| Phase | Data | Purpose |
|-------|------|---------|
| **Train** | 8000 healthy samples | Flow learns `p_healthy(x)` — anomaly data *not used* |
| **Val** | 1000 healthy + 1000 per anomaly (4 categories) | Calibrate score threshold for FPR < 1% at 3σ operating point |
| **Test** | 1000 healthy + 1000 per anomaly (4 categories) | Measure AUC per category; held-out 2 categories generalize |

The 4 anomaly categories are split for held-out testing:
- **In-threshold-calibration:** `near_horizon_spike` + `naked_singularity` (used to tune threshold multiplier, never trained on)
- **Held-out:** `signature_flip` + `ricci_excursion` (never seen in train or val — pure generalization test)

Without a val split, threshold calibration and FPR measurement would be done on training data — silently optimistic and hiding overfitting.

---

## 5. Q5: Baseline comparison protocol

A normalizing flow is complex. Before accepting it, compare against:

1. **Gaussian (Mahalanobis distance)** on the same 4 features
2. **Isolation Forest** (scikit-learn) on the same 4 features
3. **Per-feature z-score** (independent, simplest possible)

**Day-1 rule:** Fit and measure all three baselines *on day 1 of data generation*,
before training the flow. Record per-category baseline AUC in
`docs/FLOW_ANOMALY_BASELINE.md`.

**+0.03 margin scoping:** The flow replaces baseline only where
`flow_per_category_AUC > baseline_per_category_AUC + 0.03` **and**
`baseline_per_category_AUC < 0.92`. Categories where *any* baseline already
reaches >= 0.92 AUC are considered trivially separated — they are tested only
against the held-out generalization floor (> 0.75), not the +0.03 margin.

Example: `signature_flip` is `g_tt > 0` at `r > 2·r_s`; raw `g_tt` is a feature
with tight healthy distribution (`g_tt ∈ [-1, -0.95]` in the exterior). z-score
baseline will trivially separate it (AUC ~0.995). No model can beat 0.995 by
0.03 — exempting signature_flip from the margin rule prevents the release from
"failing" on a well-behaved model.

**Commit rule:** Flow ships per-category only where:
- baseline AUC < 0.92: `flow_AUC > baseline_AUC + 0.03`
- baseline AUC >= 0.92: flow passes held-out gate (> 0.75) — margin rule N/A

Document all baseline numbers before `train_flow.py` runs, so the margin
criterion is never a moving target.

---

## 6. Implementation phases

| Phase | Duration | Deliverable |
|-------|----------|-------------|
| 6.1 Data generation | 2 days | `python/gen_flow_data.py` — simulator-based: 8000/1000/1000 healthy (train/val/test) + ≥2000 anomaly samples (4 flow categories), probeValid gate data, differential detector sweep, HDF5, fixed seeds |
| 6.2 Baseline measurement | 1 day | Fit Gaussian / IsolationForest / z-score on train; evaluate per-category AUC on test; commit `docs/FLOW_ANOMALY_BASELINE.md` |
| 6.3 Flow training | 3 days | `python/train_flow.py` — PyTorch RealNVP, train on healthy only, val split for threshold calibration |
| 6.4 ONNX export | 1 day | `models/curvature_flow/flow.onnx` + `normalization.json` |
| 6.5 C++ inference | 2 days | `CurvatureNormalizingFlow::score()` — replaces stub; loads ONNX via existing ONNX Runtime integration |
| 6.6 Integration test | 2 days | `test_curvature_flow.cpp` — real AUC assertions: per-category >0.88, held-out >0.75; probeValid gate 100% recall/<0.1% FPR; differential detector >0.90; bit-exact Python↔C++ score match |

**Total: ~11 days** for the flow, assuming the design doc surfaces no surprises.
If it does (e.g., "healthy distribution needs moving metrics"), that's good —
the doc caught it before code was written.

### 6.1 Data generation details

Reuse the existing simulator (GeodesicIntegrator + SchwarzschildMetric/KerrMetric)
to compute curvature invariants at probe points. For each sample:

1. Construct the metric (Schwarzschild/Kerr/RN at specified M, a, Q)
2. Place 1000 probe points at radii r ∈ [1.5·r_s, 100·r_s], uniform in log-r
3. Call `CurvatureCalculator::computeAll(metric, probe_event)` at each point → K, R, W², g_tt
4. Record `probeValid` boolean per point

Output HDF5 with:
- `healthy/train/features` (8000 × 4), `healthy/train/labels` (all 0), `healthy/train/probeValid`
- `healthy/val/features` (1000 × 4), `healthy/val/labels` (all 0), `healthy/val/probeValid`
- `healthy/test/features` (1000 × 4), `healthy/test/labels` (all 0), `healthy/test/probeValid`
- `flow_anomaly/<category>/features`, `flow_anomaly/<category>/labels` (4 categories, ≥500 each)
- `probe_valid/nan_injection/features`, `probe_valid/nan_injection/labels`
- `differential/mass_transition/features` (M sweep sequence)
- `train_seed=42`, `test_seed=12345`, `params` (mass range, radius range, etc.)

**Pybind blocker (must be resolved before 6.1):** `python/gen_flow_data.py` cannot call
C++ directly. Add `compute_curvature_features(metric_type, M, a, Q, probe_pos)` to
`src/physics/geodesic_pybind.cpp` (~20 lines, reuses existing binding mechanism):
construct `SchwarzschildMetric`/`KerrMetric`/`RNMetric`, call
`CurvatureCalculator(metric).computeAll(Event4D)`, return `{K, R, W2, g_tt, valid}`.
Python then calls it per probe point. Do **not** re-implement curvature formulas in
Python — that recreates the formula-vs-simulator gap Q2 was written to close.

**Critical:** All features come from the simulator, never synthetic formulas.

### 6.2 Baseline measurement

Fit Gaussian (Mahalanobis), IsolationForest (scikit-learn), and per-feature z-score
on the training features. Evaluate per-category AUC on the test set for all 4
anomaly categories. Commit the numbers to `docs/FLOW_ANOMALY_BASELINE.md`
*before* training the flow. The flow's per-category target becomes
`baseline_AUC + 0.03` only where baseline AUC < 0.92; otherwise the held-out
gate (> 0.75) applies (§4.6).

### 6.3 Flow training

PyTorch RealNVP with 4 coupling layers, hidden dim 64, Adam optimizer,
learning rate 1e-3, early stopping on validation loss. Train only on healthy
data (density estimation). Anomaly score = `-log p(x)` per point.

### 6.4 ONNX export

Reuse `export_flow_onnx.py` pattern from the Neural ODE pipeline. Export the
flow's `log_prob` computation to ONNX. Save `normalization.json` with feature
mean/std used during training (C++ must apply identical normalization).

### 6.5 C++ inference

`CurvatureNormalizingFlow::loadONNXModel()` — loads `models/curvature_flow/flow.onnx`
using existing ONNX Runtime C++ integration (same pattern as
`GeodesicNeuralODE::loadONNXModel`). `score(features)` → per-point anomaly score.
Replace the stub that returns 0.0.

### 6.6 Integration test

`test_curvature_flow.cpp`:

**Bit-exact pipeline test (orthogonal to AUC):**
- Load a known feature vector from a fixture (`test_fixture.json`)
- Python ONNX runtime returns `reference_score`
- C++ `CurvatureNormalizingFlow::score(features)` returns `cpp_score`
- `QV_CHECK_NEAR(cpp_score, reference_score, 1e-5)` — validates the round-trip:
  Python preprocessing → ONNX → C++ postprocessing all agree on known input

**AUC test (model quality on top of trusted pipeline):**
- Load ONNX model, run inference on healthy + anomaly points
- Assert flow per-category AUC > 0.88 for all 4 flow categories
- Assert flow held-out category AUC > 0.75 (signature_flip + ricci_excursion unseen)
- Assert probeValid gate: 100% recall on NaN injection, <0.1% FPR on healthy
- Assert differential detector AUC > 0.90 on mass-transition samples

---

## 7. Risks and mitigations

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Healthy distribution too narrow (model flags valid strong-field as anomaly) | Medium | High | Widen M/r ranges; verify per-category AUC on near-horizon spike stays high |
| Model memorizes generator patterns | Medium | High | Held-out category test catches this; if it fails, add more generator diversity |
| ONNX export of flow ops unsupported | Low | Medium | Test export early in 6.3; fall back to TorchScript if needed |
| C++ inference normalization mismatch with Python | Medium | Medium | Share `normalization.json`; assert identical scores on same input in test |
| Flow per-category AUC plateaus below 0.88 on any category | Medium | High | Add `log K · log\|W\|²` cross-term first (highest yield, +0.02–0.04 typical). Then add `Christoffel_norm = sum |Γ^λ_μν|²` as 6th feature. Budget 2 extra days for one feature-engineering iteration. If both fail, revisit per-point vs neighborhood representation (§Q2) — the design doc decision is reviewable when evidence contradicts it. |

---

## 8. Success sign-off criteria

CP3.3 is green when all seven hold:

1. **Bit-exact pipeline test passes** — `QV_CHECK_NEAR(cpp_score, reference_score, 1e-5)` on fixture input
2. **Flow per-category AUC > 0.88** (each of 4 flow categories)
3. **Flow held-out category AUC > 0.75** (signature_flip + ricci_excursion unseen)
4. **probeValid gate**: 100% recall on NaN injection, <0.1% FPR on healthy
5. **Differential detector**: AUC > 0.90 on mass-transition samples
6. **Baseline comparison documented** — per-category (+0.03 margin where baseline < 0.92; exempt where baseline >= 0.92)
7. **ONNX model** at `models/curvature_flow/flow.onnx` + `normalization.json`; `CurvatureNormalizingFlow::score()` replaces the stub and produces non-zero scores

After CP3.3, proceed to 3.6 (neural surrogate integration with RK4 fallback).
