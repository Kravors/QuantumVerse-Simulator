# Task 2.5: Anomaly Detection with Normalizing Flows - Setup Guide

**Status**: Ready to start  
**Dependencies**: Task 2.3 (Differentiable Simulator) ✅ COMPLETE  
**Estimated Duration**: 2-3 weeks  
**Start Date**: 2026-05-04  

---

## Task Overview

**Objective**: Build an unsupervised anomaly detection system for curvature patterns using normalizing flows (MAF/RealNVP). The system will learn the distribution of "normal" General Relativity curvature invariants (Kretschmann, Weyl, Ricci) and flag exotic spacetimes (regular black holes, quantum-corrected metrics) as anomalies.

**Key Innovation**: Unlike supervised CNNs, normalizing flows provide **density estimation** - they can quantify how unlikely a curvature pattern is, even for anomalies never seen before.

---

## Architecture

### 1. CurvatureNormalizingFlow (Python)

**Purpose**: Train a normalizing flow model on GR curvature histograms.

**Components**:
- **Dataset generation**: Sample curvature invariants from diverse GR spacetimes
  - Schwarzschild (varying M)
  - Kerr (varying M, a)
  - Reissner-Nordström (varying M, Q)
  - Kerr-Newman (M, a, Q)
  - Weak-field limits (post-Newtonian)
  
- **Feature extraction**:
  - Compute curvature invariants at sample points
  - Build histograms or summary statistics (mean, std, skew, kurtosis, percentiles)
  - Normalize features (standard scaling)

- **Normalizing flow model**:
  - **Architecture**: MAF (Masked Autoregressive Flow) or RealNVP
  - **Base distribution**: Standard multivariate Gaussian
  - **Transform layers**: Affine coupling layers with neural networks
  - **Training objective**: Maximize log-likelihood log p(x)
  - **Loss**: Negative log-likelihood (NLL)

- **Anomaly scoring**:
  - Score = -log p(x) (higher = more anomalous)
  - Threshold selection via ROC curve on validation set

**Output**: Trained PyTorch model (`models/curvature_flow/curvature_flow.pth`)

### 2. RealtimeAnomalyDetector (C++)

**Purpose**: Fast inference in C++ for real-time anomaly detection during simulation.

**Components**:
- `RealtimeAnomalyDetector` class:
  - Load trained ONNX model
  - Extract curvature features from current spacetime region
  - Compute anomaly score in <1ms
  - Threshold comparison (configurable)
  - Callback/event system for anomaly alerts

**Integration**:
- Hook into `DifferentiableCurvature` or `CurvatureRenderer`
- Sample curvature at grid points in region of interest
- Compute features, run through flow, get score
- Emit signal if score > threshold

### 3. DiscoveryEngine Integration

**Purpose**: Replace/augment existing CNN-based anomaly detection with flow-based scoring.

**Current state**: `DiscoveryEngine` uses `AnomalyDetector` (CNN classifier)  
**New approach**: Use `RealtimeAnomalyDetector` with normalizing flow

**Changes**:
- Add `RealtimeAnomalyDetector` member to `DiscoveryEngine`
- In `detectAnomaly()`: compute curvature features → flow score → threshold
- Keep CNN as fallback or ensemble (optional)

---

## Implementation Steps

### Step 1: Python Environment Setup

**Dependencies** (add to `python/requirements_task2_5.txt`):
```
torch>=2.0
numpy>=1.24
scipy>=1.10
scikit-learn>=1.3
matplotlib>=3.6
seaborn>=0.12
h5py>=3.9
tensorboard>=2.13  # optional, for logging
pyyaml>=6.0
```

**Install**:
```bash
cd f:/syyyy
python -m pip install -r python/requirements_task2_5.txt
```

### Step 2: Dataset Generation Script

**File**: `python/generate_curvature_dataset.py`

**What it does**:
1. Generate diverse GR spacetimes (Schwarzschild, Kerr, RN, KN)
2. For each spacetime, sample points in coordinate space
3. Compute curvature invariants (R, K, Weyl², Ricci²) using `DifferentiableCurvature`
4. Compute histogram or summary statistics of curvature values
5. Store features + labels (spacetime type, parameters)

**Output**: `data/curvature_dataset.h5` (or multiple files)

**Dataset size**: ~10⁴-10⁵ samples (each sample = one spacetime configuration)

**Feature vector design** (options):
- **Option A (histogram)**: Bin curvature values into 20-50 bins, normalize to PDF
- **Option B (summary stats)**: Mean, std, min, max, percentiles (5%, 25%, 50%, 75%, 95%), skew, kurtosis
- **Option C (raw samples)**: Sample N curvature values, flatten (N=100-1000)

Recommend starting with **Option B** (summary stats, ~15 dimensions) for simplicity.

### Step 3: Normalizing Flow Model

**File**: `python/train_curvature_flow.py`

**Architecture choices**:
1. **MAF (Masked Autoregressive Flow)**:
   - Autoregressive transforms: each dimension conditioned on previous
   - Uses MADE (Masked Autoencoder for Distribution Estimation)
   - Pros: Exact likelihood, flexible
   - Cons: Slow sampling (sequential)

2. **RealNVP (Non-Volume Preserving)**:
   - Coupling layers: split dimensions, transform half conditioned on other half
   - Pros: Fast sampling (parallel), exact likelihood
   - Cons: Less expressive than MAF

**Recommendation**: Start with **RealNVP** (simpler, faster), try MAF if needed.

**PyTorch implementation**:
- Use `pyro.distributions.transforms` (Pyro library) or custom
- Or use `nflows` library (dedicated normalizing flows)
- Or implement from scratch (educational)

**Training**:
- Optimizer: Adam (lr=1e-3)
- Batch size: 64-256
- Epochs: 100-500 (early stopping on validation loss)
- Validation split: 20%
- Logging: TensorBoard or simple CSV

**Output**: `models/curvature_flow/curvature_flow.pth` (state dict)

### Step 4: ONNX Export

**File**: `python/export_flow_onnx.py`

**Steps**:
1. Load trained model
2. Create dummy input (batch, feature_dim)
3. `torch.onnx.export()` with opset 18
4. Validate with `onnx` package

**Output**: `models/curvature_flow/curvature_flow.onnx`

### Step 5: C++ Implementation

**Files**:
- `src/ml/CurvatureNormalizingFlow.h` (interface)
- `src/ml/CurvatureNormalizingFlow.cpp` (ONNX Runtime + stub)

**Pattern**: Same as `GeodesicNeuralODE` (PIMPL, RAII, move semantics)

**Methods**:
- `loadONNXModel(const std::string& path)`
- `predict(const std::vector<double>& features) -> double anomaly_score`
- `isLoaded() const -> bool`
- `createDefault() -> std::unique_ptr<CurvatureNormalizingFlow>`

**Stub fallback**: Return constant score (e.g., 0.0 = normal) or simple heuristic (e.g., max curvature > threshold).

### Step 6: Unit Tests

**File**: `tests/test_curvature_flow.cpp`

**Tests**:
- Constructor, destructor
- Model loading (ONNX available vs stub)
- Prediction on known "normal" (Schwarzschild) → low score
- Prediction on known "anomalous" (Hayward) → high score
- Feature vector validation (correct size)
- Threshold logic

### Step 7: DiscoveryEngine Integration

**File**: `src/discovery/DiscoveryEngine.h` (modify)

**Changes**:
- Add `std::unique_ptr<CurvatureNormalizingFlow> anomaly_flow_;`
- Add `loadAnomalyFlow(const std::string& model_path)`
- Modify `detectAnomaly()` to use flow score instead of/in addition to CNN
- Add configuration: anomaly threshold, feature extraction method

**Feature extraction**:
- Sample curvature at grid points in simulation region
- Compute summary statistics (mean, std, percentiles, etc.)
- Normalize using training set statistics (store mean/std in model metadata)

### Step 8: Validation & Benchmarking

**Validation**:
- Test on held-out GR spacetimes (should have low anomaly scores)
- Test on exotic spacetimes (Hayward, Bardeen, LQG) (should have high scores)
- Compute ROC AUC (target >0.95)

**Benchmark**:
- Inference time per sample (target <1ms)
- Compare to CNN inference time (should be faster or similar)

---

## File Structure

```
Task 2.5 Files (new):
python/
  generate_curvature_dataset.py    (NEW - dataset generation)
  train_curvature_flow.py          (NEW - flow training)
  export_flow_onnx.py              (NEW - ONNX export)
  requirements_task2_5.txt         (NEW - dependencies)

src/ml/
  CurvatureNormalizingFlow.h       (NEW - C++ interface)
  CurvatureNormalizingFlow.cpp     (NEW - ONNX + stub)

tests/
  test_curvature_flow.cpp          (NEW - unit tests)

data/
  curvature_dataset.h5             (NEW - training data, ~100 MB)

models/curvature_flow/
  curvature_flow.pth               (NEW - best checkpoint)
  curvature_flow.onnx              (NEW - exported model, ~1-10 MB)
  training_log.csv                 (NEW - training metrics)

docs/
  TASK2_5_STATUS.md                (NEW - live status)
  TASK2_5_IMPLEMENTATION_SUMMARY.md (NEW - implementation details)
```

---

## Success Criteria

- [ ] Dataset: ≥10⁴ curvature samples from diverse GR spacetimes
- [ ] Model: Normalizing flow trained to NLL < -50 (or comparable to baseline)
- [ ] ONNX export: Successful, validates with onnxruntime
- [ ] C++ interface: `CurvatureNormalizingFlow` class with stub fallback
- [ ] Unit tests: All passing (stub mode + ONNX if available)
- [ ] Integration: `DiscoveryEngine` uses flow for anomaly scoring
- [ ] Validation: ROC AUC > 0.95 on held-out test set (GR vs exotic)
- [ ] Performance: Inference <1ms per sample on CPU

---

## Dependencies & Prerequisites

**Must have**:
- Task 2.3 complete (DifferentiableCurvature available)
- PyTorch installed (with CUDA if available)
- ONNX Runtime Python package (`onnxruntime`)

**Nice to have**:
- `nflows` library for normalizing flows (`pip install nflows`)
- Pyro (`pip install pyro-ppl`) for transforms
- TensorBoard for training visualization

---

## Risks & Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| Feature extraction too slow | Real-time detection impossible | Pre-compute features, use summary stats (not raw samples) |
| Flow model doesn't converge | Poor anomaly detection | Try MAF if RealNVP fails; increase capacity (more layers, hidden units) |
| Curvature computation expensive | Dataset generation slow | Use DifferentiableCurvature (already optimized); parallelize |
| ONNX export fails (custom ops) | C++ deployment blocked | Use standard PyTorch ops; avoid exotic layers |
| High false positive rate | Spurious anomalies | Tune threshold on validation; use ensemble with CNN |

---

## Next Actions (Immediate)

1. **Create Python environment**: `python/requirements_task2_5.txt`
2. **Design feature extraction**: Decide on histogram vs summary stats
3. **Write dataset generation script**: `python/generate_curvature_dataset.py`
4. **Generate small pilot dataset** (1K samples) for prototyping
5. **Choose flow architecture**: RealNVP vs MAF (start with RealNVP)
6. **Write training script**: `python/train_curvature_flow.py`
7. **Prototype in Python**: Train on pilot data, evaluate
8. **Scale to full dataset** (10K-100K samples)
9. **Export to ONNX**: `python/export_flow_onnx.py`
10. **Create C++ wrapper**: `src/ml/CurvatureNormalizingFlow.{h,cpp}`
11. **Write unit tests**: `tests/test_curvature_flow.cpp`
12. **Integrate with DiscoveryEngine**: Modify `detectAnomaly()`
13. **Validate & benchmark**: ROC AUC, inference speed
14. **Document**: API reference, usage tutorial

---

## Timeline

- **Week 1**: Dataset generation + flow training prototype (Python only)
- **Week 2**: Full training, ONNX export, C++ wrapper, unit tests
- **Week 3**: DiscoveryEngine integration, validation, benchmarking, documentation

---

*Ready to begin implementation.*
