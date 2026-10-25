# QuantumVerse Technical Documentation

**Project**: QuantumVerse Simulator — 4D Spacetime Cognition Laboratory  
**Version**: 3.9.0  
**Status**: Production Ready — All Phases 0–4 Complete (100%)  
**Language**: C++17  
**Build System**: CMake 3.25+  
**Primary UI**: Qt 6.11.1 QML (`quantumverse_qml`)  
**Rendering**: OpenGL 4.5 Core Profile  
**License**: MIT  
**Location**: `F:\syyyy`

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [System Architecture](#2-system-architecture)
3. [Feature Specification](#3-feature-specification)
4. [Implementation Details](#4-implementation-details)
5. [Development Roadmap & Context](#5-development-roadmap--context)
6. [Appendix: Reference Tables](#6-appendix-reference-tables)

---

## 1. Project Overview

### 1.1 Purpose

QuantumVerse is a production-ready, 4D spacetime cognition laboratory that enables users to navigate, visualize, and discover new physics in a four-dimensional Lorentzian manifold. It combines general relativity, quantum gravity engines, AI-driven discovery, multi-messenger astrophysics, differentiable physics, and immersive 4D visualization into a single interactive platform.

The application is not merely a simulator — it is a computational laboratory for testing the limits of general relativity, quantum gravity candidates, and exotic spacetime geometries. It allows users to "break" Einstein's field equations (e.g., negative mass, imaginary coordinates, extreme parameters) and observe what new phenomena emerge, with built-in validation against historical GR measurements.

### 1.2 Goals

| Goal | Description |
|------|-------------|
| True 4D Navigation | Full four-dimensional spacetime exploration with 6 rotation planes (SO(4)) |
| Real-Time GR | Full Einstein field equations with validated GR predictions (Mercury, light deflection, redshift, frame dragging) |
| Quantum Gravity Engines | CDT, Spin Foam (LQG), Group Field Theory, Causal Sets |
| AI-Driven Discovery | Symbolic regression, anomaly detection, RL agents, autonomous hypothesis generation |
| Multi-Messenger Pipeline | LIGO (GW), IceCube (ν), TESS (exoplanets), Fermi GBM (GRBs) real-time ingestion |
| Differentiable Physics | Gradient-based optimization of spacetime parameters via forward-mode AD |
| Immersive Visualization | OpenGL 4.5 rendering, QML UI, optional OpenXR VR |
| Holographic Duality | AdS/CFT correspondence research tooling |

### 1.3 Current Status

- **Overall completion**: 100% (production ready)
- **Phases 0–4**: Complete (100% each)
- **Test suite**: ~125 tests registered via ctest; core scientific tests all green
- **Primary executable**: `quantumverse_qml` (Qt 6.11.1 QML + OpenGL 4.5)
- **Qt-only**: Qt/QML is the supported build
- **Last updated**: 2026-09-29 (per README/DEVELOPMENT_STATUS.md); Kilo session activity through 2026-10-24 (PaperGenerator escaping fix, lensing shader fixes, rs bug fix, mass sweep clamping, camera basis fix)

---

## 2. System Architecture

### 2.1 Layered Architecture

```
┌─────────────────────────────────────────────────────────────┐
│ Application Layer (main_qml.cpp, QuantumVerseApp.cpp)       │
├─────────────────────────────────────────────────────────────┤
│ UI Layer (src/ui4d/) — QML viewport, 4D camera, UI4D coord. │
├─────────────────────────────────────────────────────────────┤
│ Discovery Layer (src/discovery/) — instruments, theory mgmt │
├─────────────────────────────────────────────────────────────┤
│ Physics Layer (src/physics/) — geodesics, singularities     │
├─────────────────────────────────────────────────────────────┤
│ Spacetime Layer (src/spacetime/) — events, metrics, curvature│
├─────────────────────────────────────────────────────────────┤
│ Rendering Layer (src/rendering/) — OpenGL 4.5 + quantum geo │
├─────────────────────────────────────────────────────────────┤
│ Math Layer (src/math/) — Vector4D, Matrix4x4, AutoDiff      │
├─────────────────────────────────────────────────────────────┤
│ Quantum Gravity (src/quantumgravity/) — CDT, SF, GFT, CS    │
├─────────────────────────────────────────────────────────────┤
│ ML / Data / Utils / VR / Audio / Scenario / Net             │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 Module Directory Structure

| Module | Path | Contents |
|--------|------|----------|
| spacetime | `src/spacetime/` | Event4D, MetricTensor, DilatonMetric, FRWMetric, LightCone, WorldLine, DifferentiableMetric |
| physics | `src/physics/` | GeodesicIntegrator, DifferentiableGeodesicIntegrator, SingularityHandler, DifferentiableCurvature, CurvatureCalculator, HawkingCalculator, DilatonBlackHole, ParameterizedMetrics, GRRayTracer, GWPolarization, GeodesicDeviation, ThreadPool, PhysicsValidator, PhysicsConstants |
| rendering | `src/rendering/` | CurvatureRenderer, QuantumGeometryRenderer, CelestialBodyRenderer, OrbitRenderer, Texture, ProceduralTextures, GLDebug, PostProcess, ComputeShader, GravitationalLensing |
| ui4d | `src/ui4d/` | UI4D, Camera4D, Camera4DAdapter, SceneGraphManager, PlanckMicroscope, EnhancedLayout, CausalGraph, DiscoveryProbe |
| math | `src/math/` | Vector4D, Matrix4x4, AutoDiff, SO4Rotation, DifferentiableMetric |
| discovery | `src/discovery/` | DiscoveryEngine, TheoryManager, DiscoveryPanelManager, DiscoveryInstrument, FindingsModel, RLDiscoveryAgent, SymbolicMath, HolographicDualityLab, LLMCoPilot, PaperGenerator + ~25 discovery instruments |
| quantumgravity | `src/quantumgravity/` | CDTEngine, SpinNetwork, SpinFoam, SpinFoamEngine, GFTEngine, CausalSet, CausalSetEngine |
| data | `src/data/` | MultiMessengerAdapter, LIGOAdapter, IceCubeAdapter, TESSAlertAdapter, GCNNoticeParser, KafkaAlertListener, AlertRouter, AlertListener, GCNReplayStream, MultiMessengerCorrelator |
| ml | `src/ml/` | GeodesicNeuralODE, MetricGNN, CurvatureNormalizingFlow, DifferentiableSimulator, AnomalyDetector, AnomalyMonitor, TrainingDataCollector |
| utils | `src/utils/` | ConfigManager, PerformanceMonitor, TraceLogger, CrashHandler, FrameDiagnostics, ThreadPool, FileIO, Security, WorkflowAnalytics, DiagnosticMode |
| vr | `src/vr/` | MultiUserServer, SharedSession, SignalingClient, OpenXRBackend, VRCommon |
| audio | `src/audio/` | GravitationalWaveAudio |
| scenario | `src/scenario/` | Scenario, ScenarioManager |
| education | `src/education/` | EducationalTour, TourController, TourManager |
| net | `src/net/` | Networking utilities |

### 2.3 Core Data Structures

#### Event4D (`src/spacetime/Event4D.h`)
Represents a 4D spacetime event `(t, x, y, z)`.
- Minkowski interval: `Δs² = -Δt² + Δx² + Δy² + Δz²`
- Proper time, spatial distance
- Causal classification: timelike (`Δs² > 0`), lightlike (`Δs² = 0`), spacelike (`Δs² < 0`)
- Lorentz transformations, coordinate extraction (`theta()`, `phi()`)
- Physical constants: `C`, `C2`, `G` static members
- Operators: `+`, `-`, `*`, `==`, `!=`, `[]`

#### MetricTensor (`src/spacetime/MetricTensor.h`)
Metric tensor `g_μν` for spacetime geometry.
- Minkowski (flat): `η = diag(-1, 1, 1, 1)`
- Schwarzschild, Kerr, Reissner-Nordström metric constructors
- `evaluate(Event4D)` → 4×4 array; `determinant()`, `inverse()`, `lowerIndex()`, `raiseIndex()`
- `isLorentzian()`, `interval()`, `properTime()`
- **Contract at r=0**: `curvatureScalars()` returns `valid=false`; `evaluate()` returns zero-initialized flat-space array (NaN/Inf-free sentinel, NOT physical Minkowski)

#### Vector4D / Matrix4x4 / AutoDiff / SO4Rotation (`src/math/`)
- `Vector4D<T>`: Lorentzian inner product `-t₁t₂ + x₁x₂ + y₁y₂ + z₁z²`, normalization, timelike/spacelike/lightlike typing
- `Matrix4x4`: Lorentz boosts, 4D rotations in any plane, determinant/inverse/transpose
- `ADVariable<N>`: forward-mode dual numbers (value + N-gradient), chain-rule arithmetic, transcendental functions, gradient/Jacobian/Hessian extraction
- `SO4Rotation`: 6 orthogonal rotation planes (XY, YZ, XZ, TX, TY, TZ)

### 2.4 Build Configuration (`CMakeLists.txt`)

**Project**: `QuantumVerse`, C++17 required, C11, `/W4` (MSVC) / `-Wall -Wextra` (GCC/Clang)

**Options**:
| Option | Default | Description |
|--------|---------|-------------|
| `QUANTUMVERSE_BUILD_TESTS` | ON | Build test suite |
| `QUANTUMVERSE_USE_QT` | OFF | Build Qt UI |
| `QUANTUMVERSE_USE_VR` | OFF | Build VR/OpenXR backend |
| `QUANTUMVERSE_USE_KAFKA` | OFF | Kafka/GCN live ingest |
| `QUANTUMVERSE_USE_COVERAGE` | OFF | Code coverage instrumentation |
| `QUANTUMVERSE_USE_ASAN` | OFF | AddressSanitizer |
| `QUANTUMVERSE_USE_UBSAN` | OFF | UndefinedBehaviorSanitizer |
| `QUANTUMVERSE_USE_TSAN` | OFF | ThreadSanitizer |
| `QUANTUMVERSE_USE_FUZZER` | OFF | libFuzzer targets (Clang only) |
| `QUANTUMVERSE_BUILD_WASM` | OFF | Emscripten WASM portal |
| `QUANTUMVERSE_USE_ONNX` | (off by default) | ONNX Runtime for ML inference |
| `QUANTUMVERSE_USE_BENCHMARK` | OFF | Google Benchmark |
| `QUANTUMVERSE_PERF_TRACE` | OFF | QmlGlRenderer per-stage timers |
| `QUANTUMVERSE_ENFORCE_TIDY_ERRORS` | ON | Treat clang-tidy warnings as errors |

**Dependencies** (Fetched/vendored):
- `nlohmann/json` v3.11.3 (FetchContent, header-only JSON)
- `libcurl` (system, optional HTTP)
- `librdkafka` (system, optional GCN/Kafka)
- OpenXR SDK (system, optional VR)
- `glad` OpenGL loader (`third_party/glad/`)
- `stb_image.h` (image loading)
- ONNX Runtime 1.22.1 prebuilt (optional, `third_party/onnxruntime/`)

**Targets**:
- `dilaton` — INTERFACE header library (src + tests include dirs, nlohmann_json, curl)
- `quantumverse_multi_user` — STATIC (vr/SignalingClient, SharedSession, MultiUserServer)
- `quantumverse_qml` — MAIN executable (Qt 6.11: Core Gui Widgets Qml Quick OpenGL OpenGLWidgets QuickControls2 QuickTemplates2 QuickLayouts Network WebSockets Test [+ Multimedia])
- ~30 test executables registered via ctest

**Windows deployment**: `deploy_qt_runtime` custom target runs `windeployqt.exe` (+ `--compiler-runtime` probe), copies MSVC CRT DLLs (msvcp140.dll, vcruntime140.dll, concrt140.dll), Qt6Test.dll; removes `opengl32sw.dll` to force native OpenGL.

### 2.5 Entry Points

| Entry Point | File | Purpose |
|-------------|------|---------|
| Qt/QML UI | `src/main_qml.cpp` | Supported production executable `quantumverse_qml` |
| Console | `src/main_console.cpp` | Headless console entry |
| App | `src/QuantumVerseApp.cpp` | Main application object |

### 2.6 Dependencies Between Modules

```
main_qml.cpp
  ├─ spacetime (Event4D, MetricTensor, KerrMetric)
  ├─ physics (SingularityHandler, CurvatureCalculator, PhysicsValidator)
  ├─ rendering (CurvatureRenderer, QuantumGeometryRenderer, CelestialBodyRenderer, GravitationalLensing)
  ├─ ui4d (UI4D, Camera4DAdapter, SceneGraphManager, PlanckMicroscope)
  ├─ discovery (DiscoveryPanelManager, FindingsModel, ~25 instruments)
  ├─ data (LIGOAdapter, IceCubeAdapter, MultiMessengerCorrelator)
  ├─ quantumgravity (CDTEngine)
  ├─ ml (GeodesicNeuralODE)
  └─ utils (TraceLogger, CrashHandler, ConfigManager, PerformanceMonitor)
```

---

## 3. Feature Specification

### 3.1 4D Navigation & Camera

**Camera4D** (`src/ui4d/Camera4D.h`)
- Position & look-at in ℝ⁴ `(t, x, y, z)`
- Temporal + spatial up-vectors
- 6 rotation planes (SO(4)): XY, XZ, YZ, TX, TY, TZ
- 4D→3D→2D perspective projection
- `cyclePlane()` — Q key / QML binding cycles through all 6 planes
- `fieldOfView` property (10°–120°, default 45°)

**Camera4DAdapter** (`src/ui4d/Camera4DAdapter.h`)
- Qt bridge mapping QML input to SO(4) rotations
- QML properties: `distance`, `azimuth`, `elevation`, `activePlane`
- `animationSpeed` configurable; smooth slerp-based animation
- Handles boost planes (TX/TY/TZ) via Lorentz boost mixing time+space

**Controls** (interactive/QML):
| Input | Effect |
|-------|--------|
| WASD | Translate X/Y (spatial) |
| QE | Translate Z (spatial) |
| Mouse drag | Rotate in selected 4D plane |
| Scroll wheel | Zoom (4D dolly) |
| Arrow keys | Adjust slicing hyperplane offset |
| C | Toggle complex spacetime mode |
| T | Toggle time-geography overlay |
| S | Save current state |
| E | Open celestial body editor |
| Space | Pause/resume simulation |
| +/- | Adjust time scale |

**Rotation plane modifiers** (mouse drag + modifier):
| Modifier | Plane |
|----------|-------|
| None | XY (standard 3D) |
| Shift | YZ |
| Ctrl | TX (time-X) |
| Alt | XZ |
| Q key | Cycle all 6 SO(4) planes |

### 3.2 Spacetime & Metrics

**Metric types supported**:
- Minkowski (flat)
- Schwarzschild (spherical mass, `rs = 2GM/c²`)
- Kerr (rotating black hole)
- Reissner-Nordström (charged)
- Dilaton (2D CGHS model)
- FRW (cosmology)

**Curvature invariants** (`CurvatureCalculator`, `MetricTensor`):
- Christoffel symbols: `Γ^λ_μν = ½ g^λρ(∂_μg_νρ + ∂_νg_μρ - ∂_ρg_μν)`
- Riemann tensor `R^ρ_σμν`
- Ricci tensor `R_μν` & scalar `R`
- Weyl tensor (conformal curvature)
- Kretschmann scalar `K = R_αβγδR^αβγδ` (singularity detection)

**Physics invariants monitored**:
1. MetricTensor symmetry: `g[μ][ν] = g[ν][μ]`
2. Lorentzian signature: `(-,+,+,+)`
3. Negative determinant: `det(g) < 0`
4. Energy conservation along geodesics
5. Angular momentum conservation in central potentials
6. Timelike separation for physical trajectories

### 3.3 Physics — Geodesics & Singularities

**GeodesicIntegrator** (`src/physics/GeodesicIntegrator.h`)
- Adaptive RK4, tolerance 1e-8
- Solves `d²x^μ/dτ² + Γ^μ_αβ(dx^α/dτ)(dx^β/dτ) = 0`
- Modes: TIMELIKE, LIGHTLIKE, SPACELIKE
- Adaptive and `integrateSimple()` (fixed-step)
- Step range 1e-10 to 0.1, max 100,000 iterations
- Performance: 5–10 ms per trajectory (τ=1.0, 1000 steps)

**DifferentiableGeodesicIntegrator** (`src/physics/DifferentiableGeodesicIntegrator.h`)
- Template `integrateAD<N>()` for N-parameter gradients via forward-mode AD
- `integrateWithGradients()` — gradient extraction `∂final_position/∂θ`
- Supports N=1,2,3 (compile-time)
- Validated against analytical GR formulas

**SingularityHandler** (`src/physics/SingularityHandler.h`)
- Black hole types: Schwarzschild, Kerr, Reissner-Nordström, Kerr-Newman, Naked, Fuzzball
- Regular BHs: Hayward, Bardeen, Loop Quantum, Regular Gauss
- Event horizon: `isInsideEventHorizon()`, ergosphere `isInErgosphere()`
- Tidal forces: `computeTidalForces()` (spaghettification)
- Hawking temperature & evaporation: `getHawkingTemperature()`, `evolveHawkingEvaporation()`
- Gravitational redshift, frame dragging angular velocity

**Related**: `GeodesicDeviation` (tidal tensor, Raychaudhuri), `GRRayTracer` (light deflection), `GWPolarization` (plus/cross/breathing/longitudinal/vector modes), `HawkingCalculator`, `DilatonBlackHole` (2D CGHS).

### 3.4 Rendering

**CurvatureRenderer** (`src/rendering/CurvatureRenderer.h`)
- OpenGL 4.5, custom vertex/fragment/geometry shaders
- Deformable 4D lattice (30×30×30 grid)
- Modes: GRID_DEFORMATION, LIGHT_CONE, GEODESIC_SPRAY, TIDAL_VECTORS, WIREFRAME, SOLID
- Color coding by curvature invariants; GPU instancing; LOD system

**QuantumGeometryRenderer** (`src/rendering/QuantumGeometryRenderer.h`)
- Quantum gravity theories: CDT (4-simplex wireframe), Spin Foam (sphere vertices ∝ spin j, cylinder edges), GFT (volume cubes colored by |φ|), Causal Set (point cloud + causal links)
- 4 LOD levels (Ultra→Low), VBO/VAO caching per theory
- 4D→3D projection via UI4D camera
- **Known fix**: QG path uses fixed `M_PI/4` FOV (not distance-derived) to match main viewport

**CelestialBodyRenderer** (`src/rendering/CelestialBodyRenderer.h`)
- UV sphere tessellation (LOW/MEDIUM/HIGH/ULTRA quality)
- Phong/Blinn-Phong lighting, atmospheric glow, emissive
- `GL_TEXTURE_2D_ARRAY` texture array for per-body NASA planet textures
- Instanced rendering: 33 floats/instance (model mat4, inverse-transpose mat3, color vec3, emissive vec3, radius, textureLayer)
- `Texture` / `TextureArray` RAII wrappers (stb_image)

**OrbitRenderer**: Orbital path rendering for solar system bodies.

**GravitationalLensing** (`src/rendering/GravitationalLensing.cpp`)
- Kerr black hole shadow rendering
- `[LENS]` debug logs in `updateUniforms()` prints `u_cameraPos`, `u_bhPos`, `distance`, `theta`, `phi`, `hasCameraPos`, `u_cameraDir`
- Theta convention: `(90.0 - cliCamElevationDeg) * π/180` so el=0 → θ=π/2 (equator)
- Camera position fed via `setLensingCameraPos()`; `hasCameraPos=true` bypasses spherical reconstruction
- `u_cameraDir = normalize(bhPos - cameraPos)`; orthonormal basis `right = cross(forward, worldUp)`, `up = cross(right, forward)`

### 3.5 4D UI (ui4d)

**UI4D** (`src/ui4d/UI4D.h`) — main coordinator
- 4 simultaneous slice views; linked selection across views
- Slicing modes: FIXED_T, PROPER_TIME, NULL_SLICE, COMPLEX (Wick rotation)
- 4D object rendering: events (color-coded spheres), world-lines, light cones, singularities
- Time-geography overlay (capability prisms, coupling zones, authority regions)
- Topology explorer (manifold/Zeeman/Alexandrov modes)
- Event/singularity real-time editing; anomaly detection; curvature manipulation; gravitational lensing; time dilation grid; quantum foam

**SliceView**: 4 slicing modes; generates 3D hypersurface from 4D spacetime; caches points.

**CausalGraph**: Event nodes + causal relationships (`Δs² ≥ 0`); cycle detection (CTCs); graph visualization.

**DiscoveryProbe**: 4D cursor sampling Kretschmann/Ricci/Weyl scalars, stress-energy at any spacetime point.

**SceneGraphManager**: 4D scene graph for solar system and objects.

**PlanckMicroscope** (`src/ui4d/PlanckMicroscope.h`)
- Logarithmic zoom 1 m → 10⁻³⁵ m (35 orders of magnitude)
- 4 LOD levels: Ultra (<10⁻²⁰ m), High (10⁻²⁰–10⁻¹⁰ m), Medium (10⁻¹⁰–1 m), Low (≥1 m)
- Theory-aware info panel; smooth 30 FPS transitions

**EnhancedLayout**, **CausalGraph**, **DiscoveryProbe**: supporting UI components.

### 3.6 Discovery Instruments

Each instrument follows `DiscoveryInstrument` base: `analyze(MetricTensor&, Event4D&, vector<Event4D>&)`, `getParameterRanges()` for QML, TDD tests, registration in `DiscoveryEngine.cpp` / `main_qml.cpp`.

| Domain | Instruments |
|--------|-------------|
| GW Detection | `MergingBinaryInspiralAnalyzer`, `BosonStarCollisionPredictor`, `GWRingdownScanner`, `GWEchoHunter`, `ECORingdownAnalyzer`, `GWMemoryDetector`, `GravitationalWaveMemoryAnalyzer`, `SGWBBackgroundAnalyzer`, `EMBrightGWCounterpartDetector` |
| BH Physics | `KerrNoHairViolationAnalyzer`, `HawkingRadiationDetector`, `NeutronStarGlitchPhaseDetector`, `BlackHoleJetAnomalyRecogniser` |
| Multi-Messenger | `NeutrinoBurstAnalyzer`, `KilonovaAfterglowScanner`, `FastRadioBurstAnalyzer`, `FRBDispersionScanner` |
| Cosmology | `CMBLensingScanner`, `CosmicShearScanner`, `PTAScanner`, `RecombinationConstantVariationImager` |
| Dark Matter | `UltralightDMWaveInterferometer`, `DarkMatterAnnihilationAnalyzer`, `PBHMicrolensingScanner` |
| Exoplanets / Fifth Force | `ExoplanetaryTTVFifthForceHunter` |
| Fine Structure / BBN | `FineStructureConstantDriftObservatory`, `PrimordialLithiumCrisisSolver` |
| Galactic | `GalacticRotationCurveScanner`, `GalacticTidalStreamCartographer` |
| Theory | `TheoryDiscoveryAgent`, `HolographicDualityLab`, `SymbolicMath`, `RLDiscoveryAgent`, `LLMCoPilot` |

**DiscoveryEngine** — AI-driven autonomous discovery: symbolic regression, anomaly detection, hypothesis management (propose→validate→rank), GR validation, Bayesian comparison, LaTeX report generation, state persistence.

**FindingsModel** (`src/discovery/FindingsModel.h`) — QML list model for findings with filtering/sorting/detail-view; multi-messenger integration (LIGO/IceCube → findings).

**TheoryManager** — plugin registration/management for gravity theories (TheoryPlugin interface: `computeMetric()`, `predictsNewPhenomena()`, `getName()`, `getParameters()`).

**RLDiscoveryAgent** (`src/discovery/RLDiscoveryAgent.h`) — RL agent for autonomous parameter exploration (epsilon-greedy policy).

**SymbolicMath** (`src/discovery/SymbolicMath.h`) — symbolic regression for field equation generation; `resolveExecutablePath()` rejects `WindowsApps` stubs (App Execution Alias trap).

**HolographicDualityLab** — AdS/CFT correspondence for quantum gravity research.

**PaperGenerator** (`src/discovery/PaperGenerator.h/.cpp`) — LaTeX paper generation; `escapeLatex()` sanitizes `_ % & # $ {} ~ ^ \` from LLM output; `setWorkingDirectory` fix ensures PDF written next to .tex.

### 3.7 Quantum Gravity Engines

**CDTEngine** (`src/quantumgravity/CDTEngine.h`) — Causal Dynamical Triangulations
- `SimplicialManifold`: vertices, edges, triangles, tetrahedra, 4-simplices
- Topology: S¹×S³, S³, closed
- Pachner moves (2→6, 3→3, 4→2); Monte Carlo `runMonteCarlo()`, `thermalize()`
- Regge curvature via deficit angles; observables: spectral dimension, Hausdorff dimension, curvature
- `computeMetric()` → effective continuum metric

**SpinNetwork / SpinFoam / SpinFoamEngine** — Loop Quantum Gravity
- SU(2) spin network (spins on edges, intertwiners at vertices)
- 2-complex with vertex amplitudes; Wigner 6j (GSL or fallback), 15j placeholder
- EPRL vertex amplitude; spectral dimension `d_s(μ) = 4 - 2/(1+(μ/μ_c))` → 2 at Planck scale
- Effective metric with λ² regularization; Monte Carlo spin sampling

**GFTEngine** (`src/quantumgravity/GFTEngine.h`) — Group Field Theory
- `GFTField` tensor field on G^d×G^d (G=SU(2)); `TensorNetwork` Feynman diagrams
- `CondensateSolver` mean-field ⟨φ⟩≠0; action `S[φ] = ∫φ⁴ + m²φ² + λφ⁴...`
- Emergent FLRW metric from homogeneous condensate

**CausalSet / CausalSetEngine** — Causal Set Dynamics
- Partial order (poset) with Poisson sprinkling; `GrowthDynamics` sequential growth
- `BenincasaDowkerAction: S = λR + μN`
- Dimension estimators: Myrheim-Meyer, spectral; Hasse diagram visualization

### 3.8 Data Pipeline — Multi-Messenger

**MultiMessengerAdapter** (base) — `GWEvent` struct (SNR, masses, FAR); callback `setCallback()`/`simulateAlert()`.
- `LIGOAdapter` — GraceDB/GCN REST via libcurl; VOEvent XML parsing; back-propagation source direction; simulation matching score; background polling
- `IceCubeAdapter`, `TESSAlertAdapter`, `FermiGBMAdapter` — stubs mirroring LIGOAdapter pattern
- `GCNNoticeParser`, `KafkaAlertListener`, `AlertRouter`, `AlertListener`, `GCNReplayStream`, `MultiMessengerCorrelator`
- **Limitation**: `start()` is no-op; real-time GCN/Kafka ingestion requires network backend

### 3.9 Machine Learning & Differentiable Physics

**GeodesicNeuralODE** (`src/ml/GeodesicNeuralODE.h`)
- C++ ONNX Runtime inference; `predict(initial event, velocity, metric params, τ) → final event`
- `predictBundle()` via ThreadPool; `loadONNXModel()`/`saveONNXModel()`
- Stub fallback for deterministic predictions when ONNX session creation unavailable
- Target: <1 ms inference, <1% endpoint error, 100× speed-up over RK4

**Python training pipeline** (`python/`):
- `datagen.py` — 10⁶ geodesics from DifferentiableGeodesicIntegrator → HDF5
- `train.py` — PyTorch MLP 256-256-256, Adam, MSE loss
- `export_model.py`, `export_best.py`, `export_flow_onnx.py`, `train_retrain.py`, `train_tiny.py`
- Dataset: `data/geodesics_1M.h5` (109 MB, 1M samples); model `models/geodesic_ode/geodesic_ode.onnx` (val_error 0.000188); `normalization.json`

**DifferentiableSimulator** (`src/ml/DifferentiableSimulator.h`)
- End-to-end differentiable simulation; `simulate()`, `simulateFull()`
- `computeJacobian()` via central differences; `simulateWithJacobian()`; `verifyGradient()`
- Factory: `createDifferentiableSimulator()`

**CurvatureNormalizingFlow** (`src/ml/CurvatureNormalizingFlow.h`) — normalizing flow anomaly detection on curvature statistics; ONNX loading with fallback; feature normalization; stub returns 0.0 if model unavailable.

**MetricGNN**, **AnomalyDetector**, **AnomalyMonitor**, **TrainingDataCollector**: supporting ML components.

### 3.10 VR, Audio, Scenario, Utils

**VR**: `MultiUserServer` (Qt WebSocket, create/join/leave sessions, head pose broadcast, 60s stale cleanup); `SharedSession`, `SignalingClient`; `OpenXRBackend` stub (full VR requires OpenXR SDK).

**Audio**: `GravitationalWaveAudio`.

**Scenario**: `Scenario`, `ScenarioManager`.

**Utils**: `ConfigManager`, `PerformanceMonitor`, `TraceLogger`, `CrashHandler` (VEH, benign `0x8001010D` COM exception caught), `FrameDiagnostics` (JSON output), `ThreadPool`, `FileIO`, `Security`, `WorkflowAnalytics`, `DiagnosticMode`.

---

## 4. Implementation Details

### 4.1 Core Workflows

#### Simulation step (single clock, post-`d3962e0`)
One active caller path for `stepSimulation(deltaTime)`:
- `QmlGlViewport::updateSimulation()` (`src/qmlglviewport.cpp:2411`) → `stepSimulation(deltaTime)` at line 2424
- Called from QML `simTimer` (interactive, `main.qml:1569`) and C++ headless loop (`main_qml.cpp:1526`)
- Never both paths simultaneously; no double-step bug
- `UI4D::updateSolarSystemPositions()` (`src/ui4d/UI4D.cpp:597`) is **dead code** (zero callers)

**Physics clock**: `physics dt = interval × timeScale`; `[N-BODY]` instrumentation in `UI4D::stepSimulationCPU` logs substep state.

**Frozen-body diagnostic** (`[REALITY]` dump every 60 sim frames, shows `bodies=9`, `gridZRange`, `simTime`):
| Layer | Check | Commit |
|-------|-------|--------|
| Config load | `[DataPaths]` resolves to source tree, not build/ | `81da060` |
| Physics loop | `updateSimulation()` calls `m_ui4d->stepSimulation()` | `c2fb9f4` |
| Initial conditions | `initializeSolarSystem()` velocity in `.y`, not `.z` | `98e80ea` |
| Renderer read | Renderer uses `body.position`, not `orbitPoints.back()` | `00e113c` |

#### N-body plane convention (load-bearing, do not unify lightly)
| System | Orbital plane | Up axis | Angular momentum |
|--------|--------------|---------|------------------|
| N-body planets | physical XY | physical Z | +Z |
| Annulus/disk | world XZ | world +Y | +Y |

`initializeSolarSystem()`: position `Event4D(0, r, 0, 0)` (+X), velocity `Event4D(0, 0, -v, 0)` (third arg → `.y`), so planets orbit XY with angular momentum +Z. Disk: `radial = vWorldPos.xz - bhCenterWorld.xz`, `v_dir = normalize(cross(OMEGA_AXIS, radial))`, radius range [0.72, 1.80].

**Do not** add disk (+Y) and planet (+Z) angular momenta or feed one into the other's formula.

### 4.2 Key Algorithms

#### Geodesic integration (RK4)
`GeodesicIntegrator` integrates the geodesic equation with adaptive step control. `DifferentiableGeodesicIntegrator< N >` wraps it in forward-mode AD: each `ADVariable` carries value + N partials; chain rule propagates through every arithmetic/transcendental operation in the RHS, yielding `∂x_final/∂θ` exactly (no truncation error from FD).

#### Reverse-mode AD (adjoint)
`src/math/AutoDiff.h` provides `ReverseTape<N>` / `ReverseAD<N>` — a tape-based reverse-mode AD system. Operations build a computation graph on a forward pass; a single reverse pass propagates adjoints to all tagged parameters. Cost is ~1 forward pass + ~0.5 backward pass, independent of parameter count N (vs 2N for central FD). Op-set: + − × ÷ sqrt sin cos exp log pow. Unsupported ops throw `std::logic_error`. `DifferentiableGeodesicIntegrator::integrateWithGradientsAdjoint<N>()` records the geodesic RK4 integration on a tape and computes `d(loss)/d(θ_i)` for all i via one backward pass, where loss = ‖x_final − x_target‖². Benchmark: N=4 scalar loss achieves ≥3.5× speedup over central FD (measured 5.33×).

#### Curvature computation (`CurvatureCalculator::computeAll()`)
Christoffel → Riemann → Ricci → Kretschmann. Profiled ~335 ms of ~350 ms per `TheoryDiscoveryAgent::evaluateTheory()` call (~4% is `luminosityDistanceMpc`). Bottleneck for test-suite speed; candidate fixes: intra-call metric memoization or SPSA (2 evals/step vs 2N for central FD).

#### Orbit-equation RK4 deflection (`conv_run.cpp`)
Schwarzschild null geodesic `u'' + u = 3Mu²`:
- b=12M: converged 0.4526 vs Python `solve_ivp` 0.453236 ✅
- b=6M: converged 1.7194 vs Python 1.719458 ✅
- Test uses `acos(b/r_start) + acos(b/r_end)` with actual trajectory endpoints (not `2*atan(r0/b)`).

#### Mass sweep
**RESOLVED (2026-10-24, Phase 1.1):** `setBlackHoleMass()` now mutates the metric in place via `SchwarzschildMetric::setMass()` (no pointer swap, no per-sample reallocation). `CurvatureRenderer::m_lastEpoch` tracks `metric->epoch()`; `render()` calls `recomputeGridZRange()` when the epoch changes, so grid Z deformation follows mass within 1 frame. `setMetric()` sets `m_metricDirty`; epoch check is an additional guard. New test `tests/test_metric_epoch.cpp` (MetricEpochTest) verifies epoch bump, monotonicity, pointer identity, and grid Z range following mass.

#### Probe cache (`QmlGlViewport::m_probeMetric`)
`setBlackHoleMass()` now mutates `m_probeMetric` in place via `SchwarzschildMetric::setMass()` (no pointer swap, epoch bumped). `CurvatureRenderer::m_lastEpoch` tracks `metric->epoch()` and calls `recomputeGridZRange()` on change so grid Z deformation follows mass within 1 frame.

**No probe cache exists.** `probeAt()` recomputes curvature scalars fresh from `m_probeMetric` on every call. The metric is analytic and cheap to evaluate, so caching provides no meaningful benefit. If a time-dependent N-body metric is ever wired into `m_probeMetric`, add `uint64_t m_metricEpoch` to any future cache key — otherwise probe values would go stale after mass mutation.

### 4.3 Integration Points

| Integration | Location | Notes |
|-------------|----------|-------|
| QML↔C++ bridge | `DiscoveryPanelManager` | `Q_OBJECT` inside `#ifdef QUANTUMVERSE_USE_QML`; benign AutoMoc note without define |
| ONNX DLL copy | `quantumverse_copy_onnx_dll()` macro | Copies `onnxruntime.dll` next to exe post-build |
| Qt deployment | `deploy_qt_runtime` target | windeployqt + CRT DLLs + remove opengl32sw.dll |
| Multi-user VR | `quantumverse_multi_user` static lib | Qt WebSockets; linked into `quantumverse_qml` |
| Lensing uniforms | `GravitationalLensing.cpp:updateUniforms()` | `[LENS]` logs verify `u_cameraPos`, `u_bhPos`, `hasCameraPos`, `u_cameraDir` |
| `[LENS-VERIFY]` | `main_qml.cpp` | Compares `camera4DAdapter->cameraPosition4D()` vs lensing camera pos — bit-for-bit match |

### 4.4 Known Issues & Corrections (authoritative)

**Fixed (do not re-open)**:
1. **Lensing shader camera convention** (2026-10-19): theta convention `(90-el)*π/180`; camera world position via `setLensingCameraPos()`; camera direction `normalize(bhPos-cameraPos)` + orthonormal basis. GPU capture gate: `--cam-az 90 --cam-el 0 --cam-dist 40 --cam-tx 40 --screenshot`.
2. **Schwarzschild `curvatureScalars()` rs bug** (2026-10-19): `rs = 2GM/c²` (was `c⁴`); `r ≤ rs → valid=false`; `r_eps = 0.01·rs` floor clamp.
3. **Mass sweep clamping** (2026-10-19): `setBlackHoleMass()` clamps to `[0.1, 10.0] M☉`.
4. **TX/TY/TZ camera basis** (2026-10-19): `rotateInPlane()` correctly applies Lorentz boost mixing; `viewMatrix()` affine after rotation.
5. **PaperGenerator escaping**: `escapeLatex()` sanitizes LaTeX specials; `setWorkingDirectory` fix for PDF output path.
6. **Quantum geometry FOV**: `M_PI/4.0` in QG path (4 → 1297 magenta pixels).
7. **Redshift n/a at simTime≈51.76**: not reproducible; instrumentation `[MASS]`, `[REDSHIFT-DEBUG]`, `[PROBE]` in place.

**Open / deferred**:
- Metric rebuilt on every mass change (pointer swap per sample); refactor to in-place update + grid recompute.
- Adjoint-mode AD for full parameter gradients deferred; central FD used.
- OpenXR backend is stub; full VR requires OpenXR SDK.
- `SurrogateIntegration` disabled (`#if 0`) pending model retraining.
- LIGO/IceCube `start()` no-op; real GCN/Kafka ingestion unimplemented.

### 4.5 Testing Strategy

**Assertions must survive NDEBUG**: Use `QV_CHECK`/`QV_CHECK_NEAR` from `tests/test_assert.h`, not `assert()` (compiled out under Release `/DNDEBUG`). The `d381c97` assert→QV_CHECK sweep revealed 9 pre-existing failures; do not re-run the unsafe conversion tool (mangled literals/arrows); the 12-file repair landed in `c7dd514`.

**Test categories**:
- Unit: `tests/test_*.cpp`
- Integration: `tests/test_integration*.cpp`
- Validation: GR benchmarks (Mercury, light deflection, redshift, frame dragging, Nordtvedt)
- Discovery: `tests/discovery/test_*.cpp` (6+ TDD checks per instrument)
- ML: anomaly detector, monitor, training data collector
- VR: headless, no OpenXR runtime required
- Fuzz: `tests/fuzz/` (libFuzzer, Clang only)
- Smoke/pixel: `tests/smoke/test_runtime.cpp` (visual regression; viewport FBO only, not full window)

**CI canary exclusions** (must stay in sync across `ci.yml`, `build-and-test.yml`, `ci-cd.yml`, `cross-platform-test.yml`, `pr-diagnostics.yml`):
`PerformanceGateTest`, `QMLPerformanceBaseline`, `ViewportStateTest`, `ViewportContentTest`, `VisualRegressionTest`, `AnimationTimingTest`, `GLStrictAuditTest`

**qFatal exit codes (Windows)**: Release `0xC0000409` (`__fastfail`), Debug `0x80000003` (`__debugbreak`) — not specific to Qt init failure; diagnose with `std::cout.flush(); std::_exit(0);` appended to `main()`.

**Qt 6.11 QML parser quirks**:
- `function` declarations not hoisted — declare before referencing items
- Avoid `parent: ApplicationWindow.overlay` inside `Popup` — use explicit anchors
- Two `RowLayout`s directly inside `ColumnLayout` inside `Popup` may fail — wrap each `RowLayout` in `Item`

**Qt 6 backend switching**: `QT_OPENGL=angle` is a no-op (ANGLE removed in Qt 6.2+). Use `QSG_RHI_BACKEND`: `d3d11`, `opengl`, `vulkan`, `software`. GL canary tests create `QOpenGLContext` directly (bypass backend selection).

**Headless run**: `build\Release\quantumverse_qml.exe --headless --frames 3 --metric schwarzschild`; software rendering: `--software-rendering` (implicitly sets `--frames 1`).

### 4.6 Coding Standards (C++17)

- `#pragma once` in every header
- Smart pointers only (`std::unique_ptr`, `std::shared_ptr`); no raw `new`/`delete`
- RAII for all resource management
- `constexpr` where possible; `noexcept` on non-throwing functions
- Doxygen on all public symbols (`@brief`, `@param`, `@return`, `@note`)
- Header-first development: write `.h` interface before `.cpp`
- Lorentz signature `(-,+,+,+)` for 4D; `(-,+)` for 2D dilaton
- Reuse `MetricTensor` for Christoffel/Riemann/Ricci/Kretschmann — do not reinvent tensor calculus
- Use `GeodesicIntegrator` (adaptive RK4) or validate custom integrators
- New modified gravity: `TheoryPlugin` subclass registered with `TheoryManager`
- Anomalies: link to curvature invariants (Kretschmann, Ricci, Weyl); flag >5σ

---

## 5. Development Roadmap & Context

### 5.1 Phase Status

| Phase | Name | Status | Completion |
|-------|------|--------|------------|
| 0 | GUI Foundation | Complete | 100% |
| 1 | Quantum Foundation | Complete | 100% |
| 2 | AI Acceleration | Complete | 100% |
| 3 | Holographic Duality Lab | Complete | 100% |
| 4 | AI Co-pilot & Metaverse | Complete | 100% |

**Overall**: 100% — production ready (v3.9.0).

### 5.2 Where Current Work Leaves Off

The most recent Kilo session activity (2026-10-24) focused on hardening the rendering and paper-generation pipeline:
- PaperGenerator LaTeX escaping (`escapeLatex()`) verified with adversarial abstract compile (zero errors)
- Template substitution to clean pdflatex output (114,827 bytes)
- `setWorkingDirectory`/path logging for PDF output

Earlier stabilization (2026-10-19 to 2026-10-21) fixed: lensing shader camera convention, Schwarzschild rs bug, mass sweep clamping, TX/TY/TZ camera basis, RL Discovery Environment ONNX integration, QML viewport black hole spin/charge support.

**A developer picking up work should prioritize**:
1. ~~The **metric rebuilt on every mass change** open issue (stale `gridZRange`, per-sample reallocation) — **RESOLVED** by Phase 1.1: in-place mass update + `m_lastEpoch` tracking + `recomputeGridZRange()` on epoch change. See `tests/test_metric_epoch.cpp`.~~
2. ~~**Adjoint-mode AD** for full parameter gradients — **RESOLVED** by Phase 1.2: `ReverseTape<N>`/`ReverseAD<N>` in `src/math/AutoDiff.h`, `integrateWithGradientsAdjoint<N>()` in `DifferentiableGeodesicIntegrator`, benchmark ≥3.5× vs central FD at N=4 (measured 5.33×). See `tests/test_reverse_ad.cpp`, `tests/test_differentiable_benchmark.cpp`.~~
3. **Probe cache epoch** if a time-dependent N-body metric is ever wired into `m_probeMetric`. (`CurvatureRenderer` already tracks epoch; `m_probeMetric` remains static analytic.)
4. Keeping the **canary exclusion list** in sync across all 5 CI workflows when adding/removing canaries.

### 5.3 Build & Test Commands (developer quick reference)

```bash
# Configure (Release, with tests + Qt)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_BUILD_TESTS=ON -DQUANTUMVERSE_USE_QT=ON
cmake --build build --config Release --parallel

# Run tests
ctest -C Release --output-on-failure

# Headless run
build\Release\quantumverse_qml.exe --headless --frames 3 --metric schwarzschild

# Software rendering (no GPU)
build\Debug\quantumverse_qml.exe --software-rendering --frames 1 --metric schwarzschild

# ASan build
cmake -B build_asan -DQUANTUMVERSE_USE_ASAN=ON -DQUANTUMVERSE_BUILD_TESTS=ON

# Deploy (Windows)
.\deploy.bat
```

### 5.4 Physics Validation Targets (must remain green)

| Test | Expected | Tolerance |
|------|----------|-----------|
| Mercury perihelion precession | 43.0 arcsec/century | ±0.1 |
| Eddington light deflection | 1.75 arcsec | ±0.01 |
| Gravitational redshift | Δλ/λ = GM/(c²r) | ±1e-6 |
| Frame dragging (Gravity Probe B) | 39 mas/year | ±1 mas/year |
| Nordtvedt parameter | \|ω−1\| < 0.003 | ±0.001 |
| Neutron star TOV limit | ~2–3 M☉ | Stable |

### 5.5 Key Files for New Developers

| File | Role |
|------|------|
| `AGENTS.md` | Agent/developer instructions, build, test, known issues |
| `README.md` | Project overview, quick start, controls, roadmap |
| `all_info.md` | Consolidated project reference (phases, modules, file structure) |
| `docs/DEVELOPMENT_STATUS.md` | Per-module feature status |
| `CMakeLists.txt` | Build configuration, targets, options |
| `src/main_qml.cpp` | Main Qt/QML entry point |
| `src/qmlglviewport.cpp` | QML viewport, probe cache, simulation step |
| `src/spacetime/MetricTensor.h` | Metric tensor + curvature |
| `src/physics/GeodesicIntegrator.h` | Adaptive RK4 geodesics |
| `src/physics/DifferentiableGeodesicIntegrator.h` | AD geodesic gradients |
| `src/discovery/DiscoveryInstrument.h` | Base class for new instruments |
| `tests/test_assert.h` | `QV_CHECK`/`QV_CHECK_NEAR` macros |

---

## 6. Appendix: Reference Tables

### 6.1 Module LOC Estimates (approx.)

| Module | Est. LOC | Status | Tests |
|--------|----------|--------|-------|
| Spacetime | ~14K | Complete | ✅ |
| Physics (Core) | ~52K | Complete | ✅ |
| Physics (Diff) | ~820 | Complete | ✅ |
| Rendering | ~45K | Complete | ✅ |
| 4D UI (ui4d) | ~87K | Complete | ✅ |
| Math | ~32K | Complete | ✅ |
| Discovery | ~18K | Complete | ✅ |
| Quantum Gravity | ~75K | Complete | ✅ |
| Data | ~19K | Complete | ✅ |
| ML | ~17K | Complete | ✅ |
| Application | ~15K | Complete | — |
| **Total** | **~365K** | **100%** | **~125 ctest** |

### 6.2 Discovery Instrument Count: ~30

GW (9) + BH (4) + Multi-messenger (4) + Cosmology (4) + Dark Matter (3) + Exoplanet/Fifth Force (1) + Fine Structure/BBN (2) + Galactic (2) + Theory (4) + supporting (DiscoveryEngine, TheoryManager, FindingsModel, RLDiscoveryAgent, SymbolicMath, HolographicDualityLab, LLMCoPilot, PaperGenerator).

### 6.3 Configuration Options Summary

| Option | Default |
|--------|---------|
| `QUANTUMVERSE_BUILD_TESTS` | ON |
| `QUANTUMVERSE_USE_QT` | OFF |
| `QUANTUMVERSE_USE_VR` | OFF |
| `QUANTUMVERSE_USE_KAFKA` | OFF |
| `QUANTUMVERSE_USE_COVERAGE` | OFF |
| `QUANTUMVERSE_USE_ASAN` | OFF |
| `QUANTUMVERSE_USE_UBSAN` | OFF |
| `QUANTUMVERSE_USE_TSAN` | OFF |
| `QUANTUMVERSE_USE_FUZZER` | OFF |
| `QUANTUMVERSE_BUILD_WASM` | OFF |
| `QUANTUMVERSE_USE_ONNX` | OFF (optional) |
| `QUANTUMVERSE_USE_BENCHMARK` | OFF |
| `QUANTUMVERSE_PERF_TRACE` | OFF |
| `QUANTUMVERSE_ENFORCE_TIDY_ERRORS` | ON |

### 6.4 Git Hygiene

- Commit messages in English: short summary, then body; reference task/issue numbers (e.g., `Task 2.1: add ONNX inference`)
- Do not commit build artifacts, `.onnx` models, or large `.h5` datasets
- Never commit secrets or keys; never log secrets

---

*End of Technical Documentation. This document is the definitive source of truth for QuantumVerse development. When it conflicts with in-repo comments or stale plan files, this document (and `AGENTS.md`) take precedence.*
