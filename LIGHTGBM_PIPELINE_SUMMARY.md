# LightGBM Discovery Pipeline - Implementation Summary

## Overview

This document describes the production-grade LightGBM pipeline for QuantumVerse discovery instruments, implemented based on the configuration provided in the project completion report.

## Files Created

### Python Training Pipeline
- **`python/train_lightgbm_discovery.py`** - Main training script with:
  - Anomaly detection model (binary classification)
  - Spacetime classifier (multiclass classification)
  - Optuna hyperparameter tuning support
  - ONNX export capability

### C++ Integration
- **`src/ml/LightGBMDiscovery.h`** - Header file with:
  - `LightGBMModelMetadata` struct
  - `LightGBMDiscovery` class for inference
  - Python subprocess fallback for model loading

- **`src/ml/LightGBMDiscovery.cpp`** - Implementation with:
  - Model loading and validation
  - Anomaly prediction
  - Classification prediction
  - Feature importance tracking

### Requirements
- **`python/requirements_lightgbm.txt`** - Dependencies:
  - lightgbm>=4.0.0
  - numpy>=1.21.0
  - pandas>=1.3.0
  - scikit-learn>=1.0.0
  - optuna>=2.10.0
  - h5py>=3.0.0

## Model Configuration

### Anomaly Detection Model
```python
params = {
    'objective': 'binary',
    'boosting_type': 'gbdt',
    'num_leaves': 127,
    'learning_rate': 0.05,
    'min_data_in_leaf': 50,
    'feature_fraction': 0.8,
    'bagging_fraction': 0.8,
    'bagging_freq': 5,
    'reg_alpha': 0.1,
    'reg_lambda': 0.1,
}
```

### Training Parameters
- **n_estimators**: 1000 (with early stopping at 50 rounds)
- **learning_rate**: 0.01-0.1 (default: 0.05)
- **num_leaves**: 31-255 (default: 127)
- **min_data_in_leaf**: 20-100 (default: 50)
- **feature_fraction**: 0.8
- **bagging_fraction**: 0.8

## Input Features

The models accept 15-dimensional feature vectors representing curvature statistics:

| Feature | Description |
|---------|-------------|
| kretschmann_mean | Mean Kretschmann scalar |
| kretschmann_std | Standard deviation |
| kretschmann_min | Minimum value |
| kretschmann_max | Maximum value |
| kretschmann_p5 | 5th percentile |
| kretschmann_p25 | 25th percentile |
| kretschmann_p50 | Median (50th percentile) |
| kretschmann_p75 | 75th percentile |
| kretschmann_p95 | 95th percentile |
| skewness | Distribution skewness |
| kurtosis | Distribution kurtosis |
| log_mean | Log-transformed mean |
| log_std | Log-transformed std |
| peakiness | max/mean ratio |
| rms | Root mean square |

## Usage

### Training
```bash
pip install -r python/requirements_lightgbm.txt
python python/train_lightgbm_discovery.py --data dataset.h5 --output-dir ./models/lightgbm
```

### Inference (C++)
```cpp
#include "ml/LightGBMDiscovery.h"

auto discovery = quantumverse::LightGBMDiscovery::createAnomalyDetector();
discovery->loadModel("models/lightgbm/lightgbm_anomaly_detector.txt");

std::vector<double> features = {/* 15 curvature features */};
double anomaly_score = discovery->predictAnomaly(features);
```

## Integration with DiscoveryEngine

The `LightGBMDiscovery` class can be integrated into the existing `DiscoveryEngine`:

```cpp
class DiscoveryEngine {
private:
    std::unique_ptr<LightGBMDiscovery> lightgbm_;
    
public:
    void loadLightGBMModel(const std::string& path) {
        lightgbm_ = LightGBMDiscovery::createAnomalyDetector();
        lightgbm_->loadModel(path);
    }
    
    double detectAnomalyWithLightGBM(const std::vector<double>& features) {
        if (lightgbm_ && lightgbm_->isLoaded()) {
            return lightgbm_->predictAnomaly(features);
        }
        return 0.0;  // Fallback
    }
};
```

## Performance Targets

| Metric | Target |
|--------|--------|
| Inference time | <1ms (CPU), <0.1ms (GPU) |
| Model size | <100MB |
| Accuracy | >95% on validation set |
| AUC-ROC | >0.95 |

## Deployment

1. Train models using `train_lightgbm_discovery.py`
2. Export to ONNX (optional, for C++ deployment)
3. Copy model files to `models/lightgbm/`
4. Build C++ project with LightGBM library linked
5. Run `quantumverse_imgui.exe`

## References

- LightGBM Documentation: https://lightgbm.readthedocs.io/
- Optuna: https://optuna.org/
- ONNX Runtime: https://onnx.ai/