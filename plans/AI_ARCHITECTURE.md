# QuantumVerse AI & ML Architecture

## Overview

This document is a foundational knowledge base for LLM prompts assisting with QuantumVerse development. It describes the role, architecture, data flows, core objectives, and technical constraints of Artificial Intelligence / Machine Learning integration within the QuantumVerse Simulator — a production-ready 4D spacetime cognition laboratory combining general relativity, quantum gravity engines, AI-driven discovery, and immersive 4D visualization.

**Purpose:** Enable an AI to accurately ingest project context to assist with code generation, bug fixing, feature integration, and decision-making without requiring re-explanation of conventions.

**Source of truth:** This document summarizes actual code in `src/ml/`, `src/discovery/`, `python/`, and `AGENTS.md`/rules. When in doubt, prefer the header files in `src/ml/` and `src/discovery/` over this summary.

---

## 1. Project Context (Non-AI Baseline)

Before understanding AI integration, an LLM must know the host system:

| Dimension | Detail |
|-----------|--------|
| Language | C++17 exclusively (`std::optional`, `std::variant`, `if constexpr`, smart pointers) |
| Headers | `#pragma once` in every header; Doxygen on all public symbols |
| Memory | `std::unique_ptr` / `std::shared_ptr` only; no raw `new`/`delete`; RAII everywhere |
| Namespace | `quantumverse` (top-level); sub-namespaces `ml`, `discovery`, `physics`, `spacetime`, `rendering`, `ui4d`, `math` |
| Build | CMake 3.25+, MSVC 2022, Qt 6.11.1 (local), Qt 6.8.3 (CI) |
| Assertions | `assert()` is compiled out under `/DNDEBUG`. Tests must use `QV_CHECK`/`QV_CHECK_NEAR` from `tests/test_assert.h` |
| Tests | `ctest -C Release --output-on-failure`; core scientific tests must remain green; canaries excluded (`PerformanceGateTest`, `QMLPerformanceBaseline`, `ViewportStateTest`, `ViewportContentTest`, `VisualRegressionTest`, `AnimationTimingTest`, `GLStrictAuditTest`) |
| Module layout | `src/<module>/` where module ∈ {spacetime, physics, rendering, ui4d, math, discovery, quantumgravity, data, ml, scenario, config, net, audio, vr, utils} |
| Headers-first | Write `.h` interface before `.cpp` implementation |

Key physics contract (relevant to AI validation):
- Lorentz signature `(−,+,+,+)`; `MetricTensor` for Christoffel/Riemann/Ricci/Kretschmann (do not re-invent tensor calculus)
- `MetricTensor::evaluate()` at r=0 returns zero-initialized flat-space array (NaN-free sentinel, NOT physical); `curvatureScalars()` returns `valid=false` at the pole
- Schwarzschild radius `rs = 2GM/c²` (fixed 2026-10-19; was incorrectly `c⁴`)
- N-body orbits in physical XY plane (angular momentum +Z); annulus disk in world XZ (angular momentum +Y) — two incompatible plane conventions; do not unify without a deferred refactor

---

## 2. Role of AI in QuantumVerse

AI/ML in QuantumVerse serves three distinct, non-overlapping roles:

### 2.1 Anomaly Detection (runtime watchdog)
Detect when the simulation drifts into unphysical regimes (NaN/Inf curvature, metric signature flips, energy non-conservation, naked singularities) using lightweight models running every N frames.

### 2.2 Autonomous Theory Discovery
Propose and validate modified-gravity theories (f(R), etc.) against observational data (Pantheon+ SNe, GW170817, BAO, CMB shift, BBN) using RL + Bayesian model comparison + Pareto optimization.

### 2.3 Natural-Language Explanation (LLM Co-Pilot)
Bridge the C++ simulator to a local Ollama LLM (llama3) so that raw anomaly/veto data is translated into physicist-grade plain-English explanations for the Discovery Console UI.

AI does NOT replace the numeric GR integrator (`GeodesicIntegrator`, adaptive RK4) or the metric evaluator. It wraps around them as an oversight and discovery layer.

---

## 3. System Architecture

### 3.1 High-Level Layering

```
┌─────────────────────────────────────────────────────────┐
│  QML UI (quantumverse_qml)                              │
│  └─ DiscoveryPanel, AnomalyMonitor Q_PROPERTY bindings │
├─────────────────────────────────────────────────────────┤
│  AI / Discovery Layer (src/discovery/)                  │
│  ├─ LLMCoPilot  (Ollama HTTP bridge, llama3)            │
│  ├─ TheoryDiscoveryAgent (RL + Bayesian + Pareto)       │
│  ├─ RLDiscoveryAgent (PolicyNetwork + ONNX PPO)         │
│  ├─ SymbolicMath / SymbolicRegressionEngine (SymPy)     │
│  ├─ DiscoveryEngine + DiscoveryInstrument base class    │
│  └─ 50+ domain instruments (GW, BH, cosmology, DM…)     │
├─────────────────────────────────────────────────────────┤
│  ML Inference Layer (src/ml/)                           │
│  ├─ AnomalyDetector   (autoencoder 20-10-2-10-20, JSON) │
│  ├─ AnomalyMonitor    (QTimer sampling, QML-exposed)    │
│  ├─ CurvatureNormalizingFlow (ONNX RealNVP/MAF, 15-dim) │
│  ├─ FeatureVector     (20-dim telemetry extraction)     │
│  ├─ GeodesicNeuralODE (ONNX neural ODE surrogate)       │
│  ├─ MetricGNN         (graph NN for metric learning)    │
│  ├─ DifferentiableSimulator (gradient-based optimization)│
│  ├─ TrainingDataCollector (labeled sample capture→JSON) │
│  └─ BVHAcceleratedMetricQuery, SurrogateIntegration     │
├─────────────────────────────────────────────────────────┤
│  Physics Core (src/spacetime/, src/physics/)            │
│  ├─ MetricTensor, Event4D, CurvatureCalculator          │
│  ├─ GeodesicIntegrator (adaptive RK4)                   │
│  └─ KerrMetric, SchwarzschildMetric (static analytic)   │
├─────────────────────────────────────────────────────────┤
│  External Runtimes (optional)                           │
│  ├─ ONNX Runtime C++ (curvature flow, geodesic ODE, RL) │
│  ├─ Python + SymPy (symbolic math via subprocess)       │
│  └─ Ollama HTTP API localhost:11434 (LLM Co-Pilot)      │
└─────────────────────────────────────────────────────────┘
```

### 3.2 Optional Dependency Strategy

ONNX Runtime is **optional**. Every ONNX-backed class provides a stub fallback:
- `CurvatureNormalizingFlow()` default constructor → stub returns constant score `0.0` (normal)
- `GeodesicNeuralODE::loadONNXModel()` → returns false if ONNX unavailable
- `RLDiscoveryAgent` → `HAVE_ONNX` macro guards `ort_*` members

Never `#include <onnxruntime/...>` unconditionally. Guard with `QUANTUMVERSE_USE_ONNX` / `HAVE_ONNX` and provide a no-op path.

Python + SymPy is invoked via subprocess (`SymbolicMath::executePythonScript`), not linked. Always validate the Python executable path (reject `WindowsApps` aliases — see `SymbolicMath::resolveExecutablePath` as reference).

Ollama is an external HTTP service at `http://localhost:11434/api/generate`. `LLMCoPilot` uses `QNetworkAccessManager`; hardcoded model `llama3`, `stream: false`.

---

## 4. Data Flows

### 4.1 Anomaly Detection Flow (runtime)

```
Simulation frame (UI4D::stepSimulationCPU)
        │
        ▼
FeatureVector::extract(telemetry, bhMass, bodies, findings)  [20-dim]
        │
        ▼
AnomalyMonitor::onSamplingTimeout()  (QTimer, configurable interval)
        │
        ├─► AnomalyDetector::score(features)  [reconstruction MSE]
        │     │
        │     └─► if score > threshold * thresholdMultiplier → anomalyDetected(QJsonObject)
        │
        └─► (optionally) CurvatureNormalizingFlow::predictRaw(curvatureFeatures)  [15-dim, -log p(x)]
              │
              └─► score > threshold → anomaly flag
        │
        ▼
QML UI: AnomalyMonitor properties (enabled, lastScore, thresholdMultiplier, sampleIntervalMs)
        │
        ▼
LLMCoPilot::analyzeAnomaly(anomalyData)  → Ollama HTTP POST → analysisReady(explanation)
```

FeatureVector layout (20 dims, index → meaning):
```
0  totalKineticEnergy
1  totalPotentialEnergy
2  totalEnergy
3  angularMomentumZ
4  earthOrbitalSpeed
5  earthDistance
6  gravitationalWaveStrain
7  gravitationalWaveFreq
8  G (gravitational constant)
9  c (speed of light)
10 blackHoleMassKg
11 bodyCount
12 meanBodySpeed
13 maxBodySpeed
14 minBodySpeed
15 stdBodySpeed
16 meanInstrumentConfidence
17 maxInstrumentConfidence
18 anomalyCount
19 instrumentCount
```

CurvatureNormalizingFlow features (15 dims): mean/std/min/max of Kretschmann K, percentiles (5/25/50/75/95), skewness, kurtosis, log-mean, log-std, peakiness (max/mean), RMS.

### 4.2 Theory Discovery Flow

```
TheoryDiscoveryAgent::discoverBestTheory(max_steps)
        │
        ├─► evaluateTheory(params)  [per step]
        │     │
        │     ├─► instantiateTheory(params) → TheoryPlugin
        │     ├─► validateMetricInvariants(metric)  [Lorentzian, Kretschmann ≥ 0, no near-singularity]
        │     ├─► computeTheoreticalPenalty(metric)  [ghost, superluminal, energy conditions]
        │     ├─► computeObservationalChi2(metric, params)  [Pantheon+, GW170817, BAO, CMB, BBN]
        │     └─► DiscoveryResult {total_reward, chi2, penalties, validity flags}
        │
        ├─► Active learning (TheorySurrogate) guides selectNextActiveLearningPoint()
        │     Acquisition modes: RANDOM, UNCERTAINTY, EI, EHVI, UCB
        │
        ├─► Pareto archive (multi-objective): objectives = [chi2, theoreticalPenalty, simplicityPenalty, nearSingularityPenalty]
        │
        ├─► optimizeWithGradient()  [central FD gradient, maxIterations=50, lr=0.01, tol=1e-4]
        │
        ├─► Bayesian model comparison (BIC, Bayes factor vs GR baseline)
        │
        └─► Best parameters (denormalized physical values)
```

Reward components (TheoryDiscoveryAgent extends RLDiscoveryAgent):
- observational chi-squared (Pantheon+, GW170817, SLACS)
- theoretical penalties (ghost instabilities, superluminal propagation, Lorentzian signature violations, singularities)
- simplicity bonus (Occam penalty for extra parameters)

### 4.3 LLM Co-Pilot Flow

```
Discovery Engine / RL agent → PhysicsValidator veto OR anomaly logged
        │
        ▼
LLMCoPilot::analyzeAnomaly(anomalyData: QString)
        │
        ▼
Construct system prompt: "You are a brilliant astrophysicist analyzing a 4D spacetime simulation..."
        + anomalyData
        + "Explain what this means for General Relativity in 2-3 sentences..."
        │
        ▼
QNetworkAccessManager POST http://localhost:11434/api/generate
        JSON: {model: "llama3", prompt, stream: false}
        │
        ▼
Parse QJsonDocument → object()["response"].toString()
        │
        ▼
Emit analysisReady(explanation: QString) → QML UI
```

---

## 5. Core AI Objectives

| Objective | Mechanism | Success Criterion |
|-----------|-----------|-------------------|
| Detect unphysical spacetime states at runtime | AnomalyDetector (autoencoder MSE) + CurvatureNormalizingFlow (-log p(x)) | Score exceeds threshold → anomaly signal; false positives minimized by training on "healthy" GR spacetimes |
| Propose viable modified-gravity theories | TheoryDiscoveryAgent (RL + Bayesian + Pareto) | Pareto front contains theories with chi2 near GR and low theoretical penalty |
| Validate field equations symbolically | SymbolicMath (SymPy subprocess) + SymbolicRegressionEngine | No ghost instabilities, no superluminal propagation, energy conditions satisfied |
| Explain anomalies in plain English | LLMCoPilot → Ollama llama3 | 2-3 sentence physicist-grade explanation emitted via analysisReady() |
| Surrogate geodesic prediction | GeodesicNeuralODE (ONNX) | PredictEvent4D trajectory within tolerance of RK4 integrator, faster inference |
| Learn metric structure from data | MetricGNN | Graph NN reproduces metric tensor components from sampled points |
| Differentiable spacetime optimization | DifferentiableSimulator | Gradient-based optimization converges on target metric parameters |

### 5.1 Validation Discipline (mandatory before claiming any AI "discovery")

Per `rules.md` section 4 and `AGENTS.md`:
1. Build and test the full suite (`cmake -B build -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_BUILD_TESTS=ON && cmake --build build --parallel && ctest -C Release --output-on-failure`)
2. Validate physics against analytical targets:
   - Mercury perihelion: 43.0 arcsec/century (±0.1)
   - Eddington light deflection: 1.75 arcsec (±0.01)
   - Gravitational redshift: Δλ/λ = GM/(c²r) (±1e-6)
   - Gravity Probe B frame dragging: 39 mas/year (±1 mas/year)
   - Nordtvedt parameter: \|ω−1\| < 0.003
3. Reject any result that fails these historical tests.

No "new physics" claim may be made without passing the GR validation suite. Discovery logs must record parameter values, observed vs expected curvature, and whether the anomaly survives cross-check. Rule out numerical artifacts or coordinate singularities before accepting an anomaly.

---

## 6. Technical Constraints

### 6.1 Build & Optional Dependencies

| Feature | Dependency | Guard / Fallback |
|---------|-----------|------------------|
| CurvatureNormalizingFlow | ONNX Runtime C++ | Stub (score=0.0) if unavailable |
| GeodesicNeuralODE | ONNX Runtime C++ | loadONNXModel returns false |
| RLDiscoveryAgent ONNX policy | ONNX Runtime C++ | `HAVE_ONNX` macro; opaque `ort_*` pointers |
| SymbolicMath | Python + SymPy (subprocess) | `isAvailable()` check; `executePythonScript` |
| LLMCoPilot | Ollama HTTP (localhost:11434) | Network error → analysisReady with error string |
| QML integration | Qt 6.11 `Q_OBJECT`, `Q_INVOKABLE`, `Q_PROPERTY` | `DiscoveryPanelManager::Q_OBJECT` guarded by `#ifdef QUANTUMVERSE_USE_QML` |

### 6.2 Threading & Concurrency

- `CurvatureNormalizingFlow::predict()` is `const` and thread-safe
- `AnomalyDetector::score()` is `const`; model loaded once, read-only during inference
- `GeodesicNeuralODE::predictBundle()` accepts `utils::ThreadPool&` for parallel geodesic integration
- `TrainingDataCollector` uses `std::mutex` for thread-safe sample access
- `TheoryDiscoveryAgent` evaluates are `const` methods (best_result_ is `mutable`)

### 6.3 Model Formats

- **AnomalyDetector:** JSON file with `feature_dim`, `threshold`, `mean`, `std`, `encoder` layers, `decoder` layers. Architecture fixed at 20→10→2→10→20 (ReLU hidden, linear output). Load via `load(path)` or `loadFromString(json_str)`.
- **CurvatureNormalizingFlow:** ONNX file (`.onnx`), RealNVP or MAF. 15-dimensional input. Normalization mean/std stored in Metadata.
- **GeodesicNeuralODE:** ONNX file. Inputs: initial event, initial velocity, metric params, tau. Output: `std::vector<Event4D>` trajectory.
- **RL PolicyNetwork:** hand-rolled C++ network (tanh activations), evolutionary mutation; optional ONNX PPO actor export.
- **Training data:** JSON (`data/ml/training_data.json`), `TrainingSample` {features, timestamp, scenarioName, isAnomalous, labelSource, description}.

### 6.4 QML Integration Constraints

- `Q_INVOKABLE` methods are callable from QML; `Q_PROPERTY` with NOTIFY signals enables bindings
- `DiscoveryPanelManager::Q_OBJECT` is inside `#ifdef QUANTUMVERSE_USE_QML` — builds without it produce a benign AutoMoc note
- Qt 6.11 QML parser quirks: functions not hoisted (declare before use); avoid `parent: ApplicationWindow.overlay` inside `Popup`; wrap `RowLayout`s in `Item` containers inside `ColumnLayout` inside `Popup`
- `LLMCoPilot` is `Q_INVOKABLE void analyzeAnomaly(const QString&)` with signal `analysisReady(const QString&)`

### 6.5 Performance Constraints

- `TheoryDiscoveryAgent::evaluateTheory()` ~350ms/call, ~335ms in `CurvatureCalculator::computeAll()` (Christoffel → Riemann → Ricci → Kretschmann). Pareto archive updates are microseconds; `luminosityDistanceMpc` is ~4%.
- `multi_objective_mode_` triggers `updateParetoArchive()` inside the optimization loop — super-linear cost with iterations. Gradient optimizer test reduced iteration count; theory discovery suite ~43s; adjoint test ~384s (CI timeout 900s).
- `DLLookupTable` caches luminosity distance (z → d_L) keyed on (H0, omegaM, omegaLambda); invalidated when cosmology changes.
- Probe cache (`QmlGlViewport::m_probeCache`) keys on `(metric pointer, position)` — safe only because `m_probeMetric` is a static analytic metric with no mutators. Do NOT wire a live N-body metric into `m_probeMetric` without adding a `m_metricEpoch` to the cache key.

### 6.6 Correctness Constraints (AI-relevant)

1. **Metric pointer stability:** `QmlGlViewport::m_probeCache` assumes `m_probeMetric` pointer is stable while state is static. If a time-dependent N-body metric is ever wired in, cache returns stale curvature scalars.
2. **r=0 contract:** `evaluate()` returns zero array at origin; `curvatureScalars().valid=false`. Do not reinterpret zero array as Minkowski.
3. **Schwarzschild rs:** `rs = 2GM/c²`, not `c⁴`. Horizon check `r ≤ rs → invalid`; `r_eps = 0.01·rs` floor clamp.
4. **Mass clamp:** `setBlackHoleMass()` clamps to `[0.1, 10.0] M☉`. Sweep runner (`runSweep`/`applySweepStep` in `qmlglviewport.cpp`) is the suspect layer for out-of-range sweeps, not the setter.
5. **Two "up" axes:** N-body orbits XY (+Z angular momentum); annulus disk XZ (+Y angular momentum). Do not mix.
6. **QV_CHECK over assert:** All test assertions must use `QV_CHECK`/`QV_CHECK_NEAR` so they survive Release builds.

### 6.7 File Locations (key)

```
src/ml/FeatureVector.h                  # 20-dim feature extraction
src/ml/AnomalyDetector.h                # Autoencoder (20-10-2-10-20), JSON model
src/ml/AnomalyMonitor.h                 # QTimer sampling, QML-exposed
src/ml/CurvatureNormalizingFlow.h       # ONNX normalizing flow, 15-dim curvature
src/ml/GeodesicNeuralODE.h              # ONNX neural ODE surrogate
src/ml/MetricGNN.h                      # Graph NN for metric learning
src/ml/DifferentiableSimulator.h        # Gradient-based spacetime optimization
src/ml/TrainingDataCollector.h          # Labeled sample capture → JSON
src/ml/BVHAcceleratedMetricQuery.h      # Spatial metric query acceleration
src/ml/SurrogateIntegration.h           # Surrogate model integration

src/discovery/DiscoveryInstrument.h     # Base class, InstrumentFinding, AlertSeverity
src/discovery/DiscoveryEngine.h         # Instrument registry
src/discovery/RLDiscoveryAgent.h        # RL agent, PolicyNetwork, ONNX PPO
src/discovery/TheoryDiscoveryAgent.h    # Theory discovery, Bayesian, Pareto, BMA
src/discovery/SymbolicMath.h            # SymPy interface, field equations
src/discovery/SymbolicMath.h            # SymbolicRegressionEngine (genetic programming)
src/discovery/LLMCoPilot.h              # Ollama HTTP bridge (llama3)
src/discovery/TheorySurrogate.h         # Surrogate for active learning
src/discovery/TheoryParameterSpace.h    # Theory parameter space

python/train.py                         # Base training
python/train_rl.py                      # RL training
python/train_curvature_flow.py          # Curvature flow training
python/export_onnx.py                   # ONNX export
python/export_flow_onnx.py              # Flow ONNX export
python/discovery_env.py                 # Gymnasium RL env (QuantumVerseEnv)
python/symbolic_regression.py           # Symbolic regression training
```

---

## 7. Integration Guidelines for Future AI-Generated Code

When generating or modifying AI-related code:

1. **Header-first:** Define the `.h` interface (Doxygen `@brief`, `@param`, `@return`, `@note`) before the `.cpp`.
2. **Namespace:** `quantumverse` for top-level; `quantumverse::ml` for ML; `quantumverse::discovery` for discovery.
3. **Smart pointers only:** `std::unique_ptr` for PIMPL and ownership; `std::shared_ptr` for shared dependencies (e.g., `AnomalyMonitor::setUI4D(std::shared_ptr<UI4D>)`).
4. **Optional ONNX:** Guard all ONNX includes/usage; provide stub fallback returning safe defaults (score 0.0, loaded false).
5. **No raw `new`/`delete`:** `QNetworkAccessManager* m_network = new QNetworkAccessManager(this);` in `LLMCoPilot` is acceptable because Qt parent ownership handles deletion — but prefer smart pointers for non-Qt objects.
6. **QML exposure:** Use `Q_INVOKABLE` for callable methods, `Q_PROPERTY` with NOTIFY for bindable state, `Q_OBJECT` macro.
7. **Tests:** Every new feature needs at least one test in `tests/`. Use `QV_CHECK`/`QV_CHECK_NEAR`, not `assert()`. Register in `CMakeLists.txt`.
8. **Physics validation:** Any AI-produced metric or theory must pass the GR validation suite (Mercury 43.0″, Eddington 1.75″, redshift, GP-B 39 mas, Nordtvedt \|ω−1\|<0.003).
9. **Do not re-invent tensor calculus:** Use `MetricTensor` / `CurvatureCalculator` for Christoffel/Riemann/Ricci/Kretschmann.
10. **Respect the two plane conventions:** N-body XY vs annulus disk XZ — do not unify without a deferred refactor.

---

## 8. Known Issues & Open Defects (AI-relevant)

| Issue | Location | Impact on AI |
|-------|----------|--------------|
| Probe cache assumes static metric | `qmlglviewport.h:973`, `qmlglviewport.cpp:2358` | Stale curvature if live N-body metric wired into `m_probeMetric`; add `m_metricEpoch` to cache key |
| Quantum geometry FOV mismatch | `qmlglviewport.cpp:1770` | CDT geometry frustum too narrow; use `M_PI/4.0` FOV (already fixed) |
| Mass sweep rebuilds metric per sample | `qmlglviewport.cpp` runSweep | ~15 swaps per batch; cache invalidated each time; grid Z deformation stale |
| `UI4D::updateSolarSystemPositions()` is dead code | `ui4d/UI4D.cpp:597` | Never called; do not route new AI code through it |
| Test-suite speed bounded by curvature compute | `TheoryDiscoveryAgent::evaluateTheory()` | ~350ms/call; consider metric memoization or SPSA for >20 iterations |
| `optimizeWithGradient()` multi-objective cost | `TheoryDiscoveryAgent.cpp` | Pareto update inside loop is super-linear; reduced iterations in tests |

---

*End of document. This file is intended to be ingested by LLMs as project context. Update it when AI/ML architecture changes.*