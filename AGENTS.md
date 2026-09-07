# QuantumVerse Agent Instructions

## Project Overview

**QuantumVerse** is a production-ready, 4D spacetime cognition laboratory that enables users to navigate, visualize, and discover new physics in a four-dimensional Lorentzian manifold. It combines **general relativity**, **quantum gravity engines**, **AI-driven discovery**, and **immersive 4D visualization** into a single interactive platform.

Key capabilities:
- True 4D navigation with 6 rotation planes (SO(4))
- Real-time Einstein field equations with validated GR predictions
- Quantum gravity engines: CDT, Spin Foam (LQG), Group Field Theory, Causal Sets
- AI-driven discovery: symbolic regression, anomaly detection, autonomous hypothesis generation
- Multi-messenger pipeline: LIGO (GW), IceCube (neutrinos), TESS (exoplanets), Fermi GBM (GRBs)
- Differentiable physics with gradient-based optimization
- Holographic duality research (AdS/CFT)
- VR support via OpenXR
- Qt 6.11 QML UI (`quantumverse_qml`)

## Repository Structure

```
src/
├── spacetime/         # 4D events, metrics, curvature invariants
├── physics/           # Geodesics, singularities, Hawking radiation, GR raytracing
├── rendering/         # OpenGL 4.5 curvature and celestial rendering
├── ui4d/              # 4D UI coordinator, QML viewport, Planck microscope
├── math/              # Vector4D, Matrix4x4, AutoDiff, SO(4) rotations
├── discovery/         # Discovery instruments, TheoryManager, TheoryDiscoveryAgent,
│                      # SymbolicMath, HolographicDualityLab, FindingsModel
├── quantumgravity/    # CDT, Spin Foam, GFT, Causal Sets
├── ml/                # FeatureVector, AnomalyDetector, AnomalyMonitor,
│                      # TrainingDataCollector, GeodesicNeuralODE, MetricGNN,
│                      # CurvatureNormalizingFlow, DifferentiableSimulator
├── scenario/          # Scenario, ScenarioManager
├── data/              # Multi-messenger adapters (LIGO, IceCube, Fermi, Swift, TESS)
├── config/            # Configuration loaders
├── net/               # Networking utilities
├── audio/             # Spatial audio
├── vr/                # VR multi-user server/client
└── utils/             # Shared utilities
```

## Development Workflow

1. Write failing test first
2. Implement minimal code to pass
3. Run sanitizers when available: `cmake -DQUANTUMVERSE_USE_ASAN=ON`
4. Run verification and differential tests
5. Commit with clear message referencing tests

## Build & Test

### Prerequisites
- Windows 10/11, MSVC 2022, CMake 3.25+
- Qt 6.11.1 (msvc2022_64)
- Optional: ONNX Runtime 1.27.0, CUDA 12.x, Python 3.10+ (ML training only)

### Build
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_BUILD_TESTS=ON
cmake --build build --parallel
```

### Run Tests
```bash
ctest -C Release --output-on-failure
```

### Run Headless
```bash
build\Release\quantumverse_qml.exe --headless --frames 3 --metric schwarzschild
```

### CI Exclusions
Some rendering and performance tests are non-blocking canaries in CI:
- `PerformanceGateTest`
- `QMLPerformanceBaseline`
- `ViewportStateTest`
- `ViewportContentTest`
- `VisualRegressionTest`
- `AnimationTimingTest`
- `GLStrictAuditTest`

Core scientific tests run in primary workflows and must remain green.

## Testing & Verification

### Test Categories
- Unit tests: `tests/test_*.cpp`
- Integration tests: `tests/test_integration*.cpp`
- Validation tests: GR benchmarks (Mercury precession, light deflection, redshift, frame dragging, Nordtvedt)
- Discovery tests: `tests/discovery/test_*.cpp` with 6+ TDD checks per instrument
- ML tests: anomaly detector, monitor, training data collector

### Physics Invariants Monitored
1. MetricTensor symmetry: `g[μ][ν] = g[ν][μ]`
2. Lorentzian signature: `(-,+,+,+)`
3. Negative determinant: `det(g) < 0`
4. Energy conservation along geodesics
5. Angular momentum conservation in central potentials
6. Timelike separation for physical trajectories

### Adding New Verification Tests
1. Add verification functions to `src/verification/` or the relevant module test file
2. Register test in `CMakeLists.txt`
3. Run `ctest -R <Name>Test --output-on-failure`

## Discovery Instruments

The project includes discovery instruments for multi-messenger astrophysics and theory exploration:

| Domain | Instruments |
|--------|-------------|
| **GW Detection** | `MergingBinaryInspiralAnalyzer`, `BosonStarCollisionPredictor`, `GWRingdownScanner` |
| **BH Physics** | `GWEchoHunter`, `ECORingdownAnalyzer`, `KerrNoHairViolationAnalyzer`, `HawkingRadiationDetector`, `GravitationalWaveMemoryAnalyzer` |
| **Multi-Messenger** | `NeutrinoBurstAnalyzer`, `KilonovaAfterglowScanner`, `FastRadioBurstAnalyzer` |
| **Cosmology** | `CMBLensingScanner`, `CosmicShearScanner`, `PTAScanner`, `RecombinationConstantVariationImager` |
| **Dark Matter** | `UltralightDMWaveInterferometer`, `DarkMatterAnnihilationAnalyzer`, `PBHMicrolensingScanner` |
| **Theory** | `TheoryDiscoveryAgent`, `HolographicDualityLab`, `SymbolicMath` |

Each instrument follows the `DiscoveryInstrument` base class pattern with:
- `analyze()` method taking `MetricTensor&`, `Event4D&`, `vector<Event4D>&`
- `getParameterRanges()` for QML dashboard integration
- TDD-verified tests in `tests/discovery/`
- Registration in `src/discovery/DiscoveryEngine.cpp` or `src/main_qml.cpp`

### Recent Analyzer Improvements
`MergingBinaryInspiralAnalyzer` includes:
- Median + 3σ outlier rejection on the frequency track
- Two-stage coarse-to-fine grid search centered on the best coarse estimate
- Improved SNR recovery from ~3 → ~770 for marginal signals
- Edge-case tests covering boundary chirp masses, off-grid recovery, short tracks, noise rejection, and seed consistency

### Adding New Instruments
1. Create `src/discovery/<Name>.h/.cpp` following existing patterns
2. Create `tests/discovery/test_<name>.cpp` with 6+ TDD checks
3. Add to test target in `CMakeLists.txt`
4. Register in discovery engine or QML entry point
5. Build and run: `ctest -R <Name>Test --output-on-failure`

## ML / Differentiable Physics

The `src/ml/` module provides C++ runtime inference and differentiable physics:
- `FeatureVector` / `AnomalyDetector` / `AnomalyMonitor`: real-time anomaly scoring
- `TrainingDataCollector`: telemetry logging for retraining
- `GeodesicNeuralODE`: Neural ODE surrogate for geodesic prediction
- `MetricGNN`: graph neural network for metric learning
- `CurvatureNormalizingFlow`: density estimation on curvature space
- `DifferentiableSimulator`: gradient-based spacetime optimization

Python training scripts live in `python/` and exported ONNX models are loaded at runtime.

## CI/CD

Primary workflows:
- PR Diagnostics
- Cross-Platform Sanitization Pipeline
- CI
- Windows Validation
- Advanced Error Discovery Pipeline

Nightly workflows (`Nightly Baseline Update`, `Nightly Deep Scan`) are allowed to fail without blocking mainline development. If they fail, check for environment-specific issues such as missing secrets or hardware differences before investigating code changes.

## Known Issues

- Windows COM exception `0x8001010D` may appear in headless/CI output; it is benign and caught by the VEH.
- Some rendering/performance tests are non-blocking canaries because they depend on GPU/driver state in CI.
- The inspiral analyzer’s `fitTaylorF2()` uses a fixed frequency model; extremely low-frequency or very short tracks may yield low-confidence detections.
- ONNX Runtime and CUDA acceleration are optional; runtime inference falls back to CPU if unavailable.

## Configuration Options

| Option | Default | Description |
|--------|---------|-------------|
| `QUANTUMVERSE_USE_ASAN` | OFF | AddressSanitizer |
| `QUANTUMVERSE_USE_TSAN` | OFF | ThreadSanitizer |
| `QUANTUMVERSE_USE_UBSAN` | OFF | UndefinedBehaviorSanitizer |
| `QUANTUMVERSE_USE_FUZZER` | OFF | libFuzzer targets |
| `QUANTUMVERSE_USE_BOOST_MULTIPRECISION` | OFF | Formal verification tests |
| `QUANTUMVERSE_ENFORCE_TIDY_ERRORS` | ON | Treat clang-tidy warnings as errors |
| `QUANTUMVERSE_USE_COVERAGE` | OFF | Enable code coverage instrumentation |

## Module Layout Convention

New code belongs under `src/<module>/` where `<module>` is one of:
`spacetime`, `physics`, `rendering`, `ui4d`, `discovery`, `quantumgravity`, `data`, `ml`, `math`, `scenario`, `audio`, `vr`, `net`, `utils`, `config`.

Tests belong in `tests/` and examples in `examples/`.
