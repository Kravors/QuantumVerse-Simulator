# QuantumVerse Simulator — Technical Documentation Report

**Version:** 3.9.0  
**Date:** 2026-07-19  
**Status:** Production Ready  
**License:** MIT

---

## Table of Contents

1. [Cosmological Framework](#1-cosmological-framework)
2. [Scientific Foundations](#2-scientific-foundations)
3. [Technical Outputs](#3-technical-outputs)

---

## 1. Cosmological Framework

### 1.1 Overview

The QuantumVerse Simulator implements a multi-layered cosmological framework that spans classical general relativity, quantum gravity candidates, and string-theoretic extensions. The framework is organized around four pillars: **spacetime geometry**, **gravitational mechanics**, **quantum gravity engines**, and **AI-driven discovery**.

### 1.2 Implemented Cosmological Models

#### 1.2.1 General Relativity (GR) Baseline

The project's primary theoretical foundation is Einstein's general relativity with Lorentzian signature `(-, +, +, +)`.

- **Minkowski Spacetime:** Flat metric `η_μν = diag(-1, 1, 1, 1)` implemented in `MetricTensor` (`src/spacetime/MetricTensor.h:47`).
- **Schwarzschild Metric:** Non-rotating black hole solution
  ```
  ds² = -(1 - 2GM/rc²) dt² + (1 - 2GM/rc²)⁻¹ dr² + r²(dθ² + sin²θ dφ²)
  ```
  Implemented via `MetricTensor::schwarzschild()` (`src/spacetime/MetricTensor.h:65`) and `SchwarzschildMetric::evaluate()` (`src/spacetime/MetricTensor.h:357`) with exact Cartesian-basis evaluation to avoid coordinate singularities. Exact invariants: `K = 48 G² M² / (c⁴ r⁶)`, `R = 0` (vacuum) at `src/spacetime/MetricTensor.h:395`.
- **Kerr Metric:** Rotating black hole with frame-dragging
  ```
  g_tφ = -rs r a sin²θ / Σ,  Σ = r² + a² cos²θ,  Δ = r² - rs r + a²
  ```
  Implemented in `MetricTensor::kerr()` (`src/spacetime/MetricTensor.h:95`).
- **Reissner-Nordström:** Charged black hole (referenced in `SingularityHandler`).
- **Kerr-Newman:** Rotating charged black hole (referenced in `SingularityHandler`).
- **Stress-Energy Tensor:** Perfect fluid source `T_μν = (ρ + p/c²)u_μ u_ν + p g_μν` at `src/spacetime/MetricTensor.h:434`.

#### 1.2.2 Friedmann-Lemaître-Robertson-Walker (FLRW) Cosmology

The `FRWMetric` class (`src/spacetime/FRWMetric.h:31`) implements homogeneous, isotropic cosmological models:

```
ds² = -c² dt² + a(t)² [dr²/(1 - kr²) + r²(dθ² + sin²θ dφ²)]
```

**Supported models:**
- **Flat matter-dominated:** `a(t) = (t/t₀)^(2/3)` at `src/spacetime/FRWMetric.h:65`
- **Flat radiation-dominated:** `a(t) = (t/t₀)^(1/2)` at `src/spacetime/FRWMetric.h:80`
- **ΛCDM:** `a(t) = (Ω_m/Ω_Λ)^(1/3) * sinh(3/2 H₀ √Ω_Λ t)^(2/3)` with `Ω_m = 0.31`, `Ω_Λ = 0.69` at `src/spacetime/FRWMetric.h:96`
- **de Sitter:** `a(t) = exp(H₀ t)` at `src/spacetime/FRWMetric.h:119`

**Spatial curvatures:** `k = 0` (flat), `k = +1` (closed/3-sphere), `k = -1` (open/hyperbolic).

#### 1.2.3 2D Dilaton Gravity (CGHS Model)

The `DilatonMetric` class (`src/spacetime/DilatonMetric.h:32`) implements the Callan-Giddings-Harvey-Strominger model in conformal gauge:

```
ds² = -e^(2ρ) dx⁺ dx⁻,   e^(2ρ) = 1 + M e^(2σ) x⁺ x⁻
φ = -σ + ½ ln(1 + M e^(2σ) x⁺ x⁻)
```

Key implementations:
- `conformalFactor()` at `src/spacetime/DilatonMetric.h:58`
- `dilaton()` at `src/spacetime/DilatonMetric.h:66`
- `ricciScalar()` at `src/spacetime/DilatonMetric.h:86`
- `isApparentHorizon()` at `src/spacetime/DilatonMetric.h:97`
- `isSingularity()` at `src/spacetime/DilatonMetric.h:107`
- `hawkingTemperature()` at `src/spacetime/DilatonMetric.h:132`

Features:
- Static black hole of mass `M` with horizon at `x⁺ x⁻ = M`
- Linear dilaton vacuum for `M = 0` (flat spacetime, `R = 0`)
- Hawking temperature: `T = 1/(4π M)` (geometric units)
- Curvature singularity detection via `isSingularity()`
- Apparent horizon tracking via `isApparentHorizon()`

#### 1.2.4 Quantum Gravity Candidates

Four discrete quantum gravity approaches are implemented as `QuantumGravityPlugin` subclasses:

| Engine | File | Class | Description |
|--------|------|-------|-------------|
| **CDT** | `src/quantumgravity/CDTEngine.h` | `CDTEngine` (`:106`) / `CDTPlugin` (`:200`) | Causal Dynamical Triangulations with 4D simplices, Pachner moves (`:71-73`), Monte Carlo simulation (`:180`), spectral/Hausdorff dimension computation (`:158-159`) |
| **Spin Foam (LQG)** | `src/quantumgravity/SpinFoamEngine.h` | `SpinFoamEngine` (`:37`) / `SpinFoamPlugin` (`:152`) | EPRL/FK spin foam model with spin networks, 2-complex path integrals, vertex amplitude (`:65`) |
| **Group Field Theory (GFT)** | `src/quantumgravity/GFTEngine.h` | `GFTEngine` (`:41`) / `GFTPlugin` (`:155`) | Field theory on group manifold `SU(2)^4` (`:43-44`), condensate states (`:52-54`), isosurface visualization |
| **Causal Sets** | `src/quantumgravity/CausalSet.h` | `CausalSet` (`:50`) / `CausalSetDynamics` (`:107`) / `CausalSetEngine` (`:158`) / `CausalSetPlugin` (`:261`) | Discrete spacetime posets with sprinkling, dimension estimation (`:178-179`), Hasse diagram visualization |

#### 1.2.5 Modified Gravity Theories

The `TheoryManager` (`src/discovery/DiscoveryEngine.h:419`) provides a plugin registry for alternative theories:

- **f(R) Gravity:** `FRLGravityPlugin` (`src/discovery/DiscoveryEngine.h:305`) with chameleon screening (`α`, `n` parameters)
- **Brans-Dicke:** `BransDickePlugin` (`src/discovery/DiscoveryEngine.h:342`) with Brans-Dicke parameter `ω` and scalar field `φ₀`
- **Loop Quantum Gravity (effective):** `LQGPlugin` (`src/discovery/DiscoveryEngine.h:379`) with polymer correction scale `λ` and Immirzi parameter `γ`

#### 1.2.6 String Cosmology Extensions

The `docs/StringCosmology.md` module documents string-theoretic extensions:
- `DilatonField`: Evolves `φ̈ + 3Hφ̇ + dV/dφ = 0` with exponential potential `V(φ) = V₀ exp(-p φ)`
- `ModuliStabilisation`: KKLT-type potential `V(φ) = V_flux + V_antid3 exp(-φ)` with gradient descent
- `BraneInflationModel`: DBI inflation with cosh potential and brane separation as inflaton

#### 1.2.7 Holographic Duality

**`HolographicDualityLab`** (`src/discovery/HolographicDualityLab.h:23`) implements AdS/CFT correspondence:
- Bulk-to-boundary propagator (`:51`)
- Holographic stress tensor (`:61`)
- Entanglement entropy via Ryu-Takayanagi (`:71`)
- Holographic complexity CV/CA (`:81`, `:90`)
- AdS metric in Poincaré coordinates (`:108`)

### 1.3 Multi-Messenger Cosmology Pipeline

Real-time cosmological data ingestion is implemented via `src/data/` adapters:

| Channel | Adapter | Alert Struct | File |
|---------|---------|--------------|------|
| LIGO/Virgo | `LIGOAdapter` | `GravitationalWaveAlert` | `src/data/LIGOAdapter.h:34` |
| IceCube | `IceCubeAdapter` | `NeutrinoAlert` | `src/data/IceCubeAdapter.h:34` |
| TESS | `TESSAlertAdapter` | `TESSAlert` | `src/data/TESSAlertAdapter.h:35` |
| Fermi GBM | `FermiGBMAdapter` | `FermiGBMAlert` | `src/data/FermiGBMAdapter.h:36` |
| Swift BAT | `SwiftBATAdapter` | `SwiftBATAlert` | `src/data/SwiftBATAdapter.h:38` |

All adapters connect to the NASA GCN Kafka broker (`gcn-kafka.nasa.gov:9092`) with SASL/SCRAM authentication, or the test broker (`test.gcn.nasa.gov:9092`).

### 1.4 Discovery Instruments (Cosmological Scanners)

Seventeen specialized discovery instruments (`src/discovery/`) inherit from `DiscoveryInstrument` (`src/discovery/DiscoveryInstrument.h:62`) and scan for cosmological and astrophysical anomalies:

| Instrument | Category | File | Class Line |
|------------|----------|------|------------|
| `GalacticRotationCurveScanner` | Galactic | `src/discovery/GalacticRotationCurveScanner.h` | `:22` |
| `FineStructureConstantDriftObservatory` | Fundamental Constants | `src/discovery/FineStructureConstantDriftObservatory.h` | `:22` |
| `PrimordialLithiumCrisisSolver` | Cosmology | `src/discovery/PrimordialLithiumCrisisSolver.h` | `:22` |
| `RecombinationConstantVariationImager` | CMB Physics | `src/discovery/RecombinationConstantVariationImager.h` | `:26` |
| `CMBLensingScanner` | CMB Physics | `src/discovery/CMBLensingScanner.h` | `:23` |
| `PTAScanner` | Gravitational Waves | `src/discovery/PTAScanner.h` | `:23` |
| `FRBDispersionScanner` | Transients | `src/discovery/FRBDispersionScanner.h` | `:28` |
| `CosmicShearScanner` | Weak Lensing | `src/discovery/CosmicShearScanner.h` | `:25` |
| `GWMemoryDetector` | Gravitational Waves | `src/discovery/GWMemoryDetector.h` | `:29` |
| `GWRingdownScanner` | Black Holes | `src/discovery/GWRingdownScanner.h` | `:45` |
| `EMBrightGWCounterpartDetector` | Multi-Messenger | `src/discovery/EMBrightGWCounterpartDetector.h` | `:27` |
| `GalacticTidalStreamCartographer` | Galactic | `src/discovery/GalacticTidalStreamCartographer.h` | `:21` |
| `ExoplanetaryTTVFifthForceHunter` | Exoplanetary | `src/discovery/ExoplanetaryTTVFifthForceHunter.h` | `:30` |
| `UltralightDMWaveInterferometer` | Dark Matter | `src/discovery/UltralightDMWaveInterferometer.h` | `:22` |
| `NeutronStarGlitchPhaseDetector` | Neutron Stars | `src/discovery/NeutronStarGlitchPhaseDetector.h` | `:22` |
| `BosonStarCollisionPredictor` | Compact Objects | `src/discovery/BosonStarCollisionPredictor.h` | `:22` |
| `BlackHoleJetAnomalyRecogniser` | Black Holes | `src/discovery/BlackHoleJetAnomalyRecogniser.h` | `:22` |

---

## 2. Scientific Foundations

### 2.1 Physics Engine Architecture

The physics layer (`src/physics/`) is organized into four subsystems:

#### 2.1.1 Geodesic Integration

**`GeodesicIntegrator`** (`src/physics/GeodesicIntegrator.h`)

- **Algorithm:** Adaptive step-size 4th-order Runge-Kutta (RK4) with error control
- **Equation:** `d²x^μ/dτ² + Γ^μ_αβ (dx^α/dτ)(dx^β/dτ) = 0`
- **Configuration:**
  - Tolerance: `1e-8`
  - Min step: `1e-10`
  - Max step: `0.1`
  - Safety factor: `0.9`
  - Max iterations: `100,000`
- **Geodesic types:** `TIMELIKE` (massive particles), `LIGHTLIKE` (photons), `SPACELIKE` (not physical)
- **Metric field support:** Position-dependent metrics via `setMetricField()`
- **Christoffel caching:** `computeChristoffelSymbols()` with `getChristoffel()` access

**`DifferentiableGeodesicIntegrator`** (`src/physics/DifferentiableGeodesicIntegrator.h`)
- Forward-mode automatic differentiation for gradient computation
- `integrateWithGradients()` returns trajectory + parameter gradients
- Supports `setParameter()` / `getParameter()` for differentiable metrics

#### 2.1.2 Curvature Calculus

**`CurvatureCalculator`** (`src/physics/CurvatureCalculator.h`)
- Riemann tensor: `R^ρ_σμν = ∂_μ Γ^ρ_νσ - ∂_ν Γ^ρ_μσ + ...`
- Ricci tensor: `R_μν = R^λ_μλν`
- Ricci scalar: `R = g^μν R_μν`
- Kretschmann scalar: `K = R_αβγδ R^αβγδ`
- Weyl tensor: Conformal curvature for vacuum solutions

**`MetricTensor`** (`src/spacetime/MetricTensor.h`)
- Christoffel symbols: `Γ^λ_μν = ½ g^λρ (∂_μ g_νρ + ∂_ν g_μρ - ∂_ρ g_μν)`
- Index manipulation: `lowerIndex()`, `raiseIndex()`
- Lorentzian signature verification via Sylvester's criterion (`isLorentzian()`)
- Exact Schwarzschild invariants: `K = 48 G² M² / (c⁴ r⁶)`, `R = 0` (vacuum)

**`DifferentiableCurvature`** (`src/physics/DifferentiableCurvature.h`)
- AD-enabled curvature tensors: `computeRicciScalarAD<N>()`, `computeKretschmannAD<N>()`, `computeRiemannTensorAD<N>()`
- Gradient computation: `computeRicciGradient()`, `computeKretschmannGradient()`

#### 2.1.3 Singularity & Black Hole Physics

**`SingularityHandler`** (`src/physics/SingularityHandler.h`)

Supported black hole types:
- Schwarzschild, Kerr, Reissner-Nordström, Kerr-Newman
- Regular black holes: Hayward, Bardeen, Loop Quantum, Regular Gauss
- Fuzzball (string theory)

Key computations:
- Event horizon: `r_s = 2GM/c²`
- Ergosphere detection (`isInErgosphere()`)
- Tidal forces / spaghettification (`computeTidalForces()`)
- Hawking temperature: `T_H = ħc³/(8πGMk_B)`
- Evaporation evolution (`evolveHawkingEvaporation()`)
- Gravitational redshift (`getGravitationalRedshift()`)
- Frame dragging: `Ω = 2GJ/(c²r³)` (`getFrameDraggingAngularVelocity()`)

**`HawkingCalculator`** (`src/physics/HawkingCalculator.h`)
- Hawking radiation flux and temperature calculations

**`DilatonBlackHole`** (`src/physics/DilatonBlackHole.h`)
- 2D CGHS black hole with conformal gauge

#### 2.1.4 Ray Tracing & Polarization

**`GRRayTracer`** (`src/physics/GRRayTracer.h`)
- General relativistic ray tracing for light deflection
- Null geodesic integration

**`GWPolarization`** (`src/physics/GWPolarization.h`)
- Gravitational wave polarization modes (plus, cross, etc.)

#### 2.1.5 Geodesic Deviation

**`GeodesicDeviation`** (`src/physics/GeodesicDeviation.h`)
- Geodesic deviation equation: `D²ξ^μ/Dτ² = -R^μ_νρσ u^ν ξ^ρ u^σ`
- Tidal force calculations between neighboring geodesics

### 2.2 Mathematical Framework

#### 2.2.1 Core Math (`src/math/`)

| Class | File | Purpose |
|-------|------|---------|
| `Vector4D` | `src/math/Vector4D.h` | 4D vector with Lorentzian dot product, cross product (bivector), normalization |
| `Matrix4x4` | `src/math/Matrix4x4.h` | 4x4 transformations, determinant, inverse, transpose |
| `SO4Rotation` | `src/math/SO4Rotation.h` | 6 rotation planes: XY, YZ, XZ, TX, TY, TZ |
| `AutoDiff` | `src/math/AutoDiff.h` | Forward-mode AD for gradient computation |
| `DifferentiableMetric` | `src/math/DifferentiableMetric.h` | AD-enabled metric wrapper |

#### 2.2.2 Spacetime Primitives (`src/spacetime/`)

| Class | File | Purpose |
|-------|------|---------|
| `Event4D` | `src/spacetime/Event4D.h` | 4D event with Minkowski interval, proper time, Lorentz transforms |
| `LightCone` | `src/spacetime/LightCone.h` | Future/past light cones, causality checks |
| `WorldLine` | `src/spacetime/WorldLine.h` | Particle world-line tracking with proper time |
| `DifferentiableMetric` | `src/spacetime/DifferentiableMetric.h` | Differentiable metric wrapper for optimization |

### 2.3 Theoretical Accuracy & Validation

All core GR predictions are validated against analytical solutions:

| Test | Expected | Result | Tolerance | Status |
|------|----------|--------|-----------|--------|
| Mercury perihelion precession | 43.0 arcsec/century | 43.0 | ±0.1 | ✅ PASS |
| Eddington light deflection | 1.75 arcsec | 1.75 | ±0.01 | ✅ PASS |
| Gravitational redshift | `Δλ/λ = GM/(c²r)` | Match | ±1e-6 | ✅ PASS |
| Frame dragging (Gravity Probe B) | 39 mas/year | 39.0 | ±1 mas/year | ✅ PASS |
| Nordtvedt parameter | `|ω - 1| < 0.003` | 0.002 | ±0.001 | ✅ PASS |
| Neutron star TOV limit | ~2-3 M☉ | Stable | — | ✅ PASS |

**Validation infrastructure:**
- `PhysicsValidator` (`src/physics/PhysicsValidator.h`): Dual-engine comparison (adaptive RK4 vs symplectic), Christoffel analytic vs numeric, Ricci self-consistency
- `CBMCVerification`: Bounded model checking of critical invariants (zero violations)
- `DifferentialTester`: Compare multiple implementations
- `RuntimeMonitor`: Physics invariant monitoring at runtime
- Property-based tests for `Vector4D`/`Matrix4x4` invariants

### 2.4 Machine Learning & Differentiable Physics

#### 2.4.1 Neural ODE Surrogate

**`GeodesicNeuralODE`** (`src/ml/GeodesicNeuralODE.h`)
- ONNX Runtime inference for geodesic prediction
- `predict()` and `predictBundle()` with ThreadPool parallelism
- Training dataset: `data/geodesics_1M.h5` (1M samples, 109 MB)
- Checkpoints in `models/geodesic_ode/`

#### 2.4.2 Normalizing Flows

**`CurvatureNormalizingFlow`** (`src/ml/CurvatureNormalizingFlow.h`)
- RealNVP/MAF normalizing flow for curvature-based anomaly detection
- 15 curvature features extracted per spacetime point
- ONNX export/import with automatic mean/std normalization

#### 2.4.3 Differentiable Simulator

**`DifferentiableSimulator`** (`src/ml/DifferentiableSimulator.h`)
- End-to-end differentiable physics with Jacobian computation
- `simulateWithJacobian()` returns observables + parameter derivatives
- Gradient verification via `verifyGradient()`

#### 2.4.4 Graph Neural Networks

**`MetricGNN`** (`src/ml/MetricGNN.h`)
- Graph neural network for metric space analysis

### 2.5 Quantum Gravity Implementations

#### 2.5.1 Causal Dynamical Triangulations (CDT)

**`CDTEngine`** (`src/quantumgravity/CDTEngine.h`)
- 4D simplicial manifold with Pachner moves (2→6, 3→3, 4→2)
- Monte Carlo simulation with `runMonteCarlo()` and `thermalize()`
- Spectral dimension: `computeSpectralDimension()`
- Hausdorff dimension: `estimateHausdorffDimension()`
- Regge action: `computeReggeAction()`
- Deficit angle computation: `computeDeficitAngle()`, `computeAllDeficitAngles()`
- Visualization: `getWireframeVertices()`, `getGeometryColors()`

#### 2.5.2 Loop Quantum Gravity (Spin Foam)

**`SpinNetwork`** / **`SpinFoamEngine`** (`src/quantumgravity/SpinFoamEngine.h`)
- EPRL spin foam model implementation
- Spin network vertices/edges with color-coded spin values
- Path integral amplitude computation
- Spectral dimension output

#### 2.5.3 Group Field Theory

**`GFTEngine`** (`src/quantumgravity/GFTEngine.h`)
- Field configuration on group manifold
- Isosurface visualization
- Vertex/edge/color output for renderer

#### 2.5.4 Causal Sets

**`CausalSetEngine`** (`src/quantumgravity/CausalSet.h`)
- Random element sprinkling with parameters `α`, `β`
- Growth dynamics: `grow()`, `setParameters()`
- Dimension estimation (spectral + Hausdorff)
- Effective metric computation: `computeMetric()`, `computeChristoffel()`
- Hasse diagram visualization

### 2.6 Holographic Duality

**`HolographicDualityLab`** (`src/discovery/HolographicDualityLab.h`)
- AdS/CFT correspondence for quantum gravity research
- Integrated into the discovery instrument panel

---

## 3. Technical Outputs

### 3.1 3D Viewport Rendering

The rendering system (`src/rendering/`) uses OpenGL 4.5 Core Profile with a multi-pass pipeline.

#### 3.1.1 CurvatureRenderer

**File:** `src/rendering/CurvatureRenderer.h`

| Feature | Specification | Source Reference |
|---------|---------------|------------------|
| Class definition | `CurvatureRenderer` | `src/rendering/CurvatureRenderer.h:60` |
| Grid resolution | Configurable (default 50×50×50 vertices) | `src/rendering/CurvatureRenderer.h:68` (`gridResolution`) |
| Grid size | 100 meters (configurable) | `src/rendering/CurvatureRenderer.h:69` (`gridSize`) |
| Modes | `GRID_DEFORMATION`, `RIEMANN_COLOR`, `CURVATURE_SCALAR`, `GEODESIC_FLOW`, `TIME_DILATION` | `src/rendering/CurvatureRenderer.h:29-35` |
| Shaders | Grid deformation, curvature visualization, geodesic lines, HUD overlay | `src/rendering/CurvatureRenderer.h:88-91` |
| Caching | Metric cache + curvature cache with validity flag | `src/rendering/CurvatureRenderer.h:110-112` |
| LOD | Spatial partitioning with per-cell update flags | `src/rendering/CurvatureRenderer.h:125-131` |
| Light cones | Dedicated VAO/VBO/EBO with reusable buffers | `src/rendering/CurvatureRenderer.h:115-118` |
| Fallback | Null renderer guard with fallback geometry | `src/rendering/CurvatureRenderer.h:121-122` |
| Vertex structure | `CurvatureVertex` with position, normal, color, curvature, time_dilation | `src/rendering/CurvatureRenderer.h:38-44` |
| Key methods | `initializeGL()` (`:189`), `setMode()` (`:192`), `render()` (`:243`), `renderGeodesics()` (`:246`), `updateTime()` (`:222`), `regenerateGrid()` (`:225`) | |

**Rendering modes:**
- **Grid Deformation:** Spacetime lattice displaced by curvature
- **Riemann Color:** Color-coded Riemann tensor components
- **Kretschmann Scalar:** `K = R_ρσμν R^ρσμν` heatmap
- **Geodesic Flow:** Trajectory lines on curved surface
- **Time Dilation:** Gravitational time dilation heatmap

#### 3.1.2 QuantumGeometryRenderer

**File:** `src/rendering/QuantumGeometryRenderer.h`

Extends `CurvatureRenderer` for quantum gravity visualization:

| Geometry Type | Rendering Method | Source Reference |
|---------------|-----------------|------------------|
| CDT | 4D simplex wireframe/translucent volumes | `src/rendering/QuantumGeometryRenderer.h:124-125` |
| Spin Foam | Spin network vertices/edges colored by spin | `src/rendering/QuantumGeometryRenderer.h:126-128` |
| GFT | Field configuration isosurfaces | `src/rendering/QuantumGeometryRenderer.h:129-130` |
| Causal Sets | Hasse diagram in 4D spacetime | `src/rendering/QuantumGeometryRenderer.h:131-133` |

Features:
- **Class definition:** `QuantumGeometryRenderer` at `src/rendering/QuantumGeometryRenderer.h:78`
- **LOD system:** 4 levels (0=lowest, 3=highest) based on zoom scale (`:115-121`)
- **Color schemes:** `CURVATURE`, `SPIN`, `AGE` (`:93`)
- **Animation:** Monte Carlo step animation with `stepAnimation()` (`:204`)
- **4D→3D projection:** `projectToScreen()` with camera + viewport (`:209-212`)
- **Engine setters:** `setCDTEngine()` (`:148`), `setSpinFoamEngine()` (`:149`), `setGFTEngine()` (`:150`), `setCausalSetEngine()` (`:151`)
- **Render method:** `render(const Camera4D&, const std::vector<SliceView>&, const MetricTensor&)` at `:170`

#### 3.1.3 CelestialBodyRenderer

**File:** `src/rendering/CelestialBodyRenderer.h`

| Feature | Specification | Source Reference |
|---------|---------------|------------------|
| Class definition | `CelestialBodyRenderer` | `src/rendering/CelestialBodyRenderer.h:69` |
| Quality levels | LOW (16×32), MEDIUM (32×64), HIGH (64×128), ULTRA (128×256) | `src/rendering/CelestialBodyRenderer.h:75-80` |
| Lighting | Blinn-Phong with ambient/diffuse/specular | `src/rendering/CelestialBodyRenderer.h:146-161` |
| Atmospheres | `setShowAtmospheres()` toggle | `src/rendering/CelestialBodyRenderer.h:182` |
| Orbit rings | `setShowOrbitRings()` toggle | `src/rendering/CelestialBodyRenderer.h:183` |
| Textures | Texture array + procedural generation | `src/rendering/CelestialBodyRenderer.h:189-193` |
| Instancing | Multiple bodies with shared geometry | `src/rendering/CelestialBodyRenderer.h:239` (`BodyData`) |
| Max bodies | 64 (configurable) | `src/rendering/CelestialBodyRenderer.h:254` |
| Key methods | `initializeGL()` (`:98`), `addBody()` (`:116`), `render()` (`:167`), `renderBody()` (`:175`) | |
| Sphere VAO | `sphereVAO()` / `sphereIndexCount()` | `src/rendering/CelestialBodyRenderer.h:203-204` |

#### 3.1.4 OrbitRenderer

**File:** `src/rendering/OrbitRenderer.h`

> **Note:** `src/rendering/OrbitRenderer.h` is referenced throughout project documentation (`README.md`, `DEVELOPMENT_STATUS.md`) as the orbital path rendering module for solar system bodies. However, the file is not present in the current `src/rendering/` directory. This is a known documentation/code gap. Orbital path rendering may be implemented inline within `CelestialBodyRenderer` or `SceneGraphManager`, or planned for a future implementation phase.

#### 3.1.5 Texture System

**Files:** `src/rendering/Texture.h`, `src/rendering/ProceduralTextures.h`

- **Texture wrapper:** `Texture` class at `src/rendering/Texture.h:24` with `loadFromFile()` (`:66`), `loadFromMemory()` (`:79`), `bind()` (`:87`)
- **Texture array:** `TextureArray` class at `src/rendering/Texture.h:156` with `initialize()` (`:177`), `addLayerFromFile()` (`:184`), `bind()` (`:201`)
- **Texture cache:** `TextureCache` class at `src/rendering/Texture.h:231` with LRU eviction
- **Procedural generation:** `ProceduralTextureGenerator` at `src/rendering/ProceduralTextures.h:65`
  - `generatePlanetTexture()` (`:75`)
  - `generateStarTexture()` (`:82`)
  - `generateGasGiantTexture()` (`:89`)
  - `generateIceWorldTexture()` (`:96`)
  - `generateBarrenTexture()` (`:103`)
  - `generateLavaWorldTexture()` (`:110`)
- **Configuration:** `PlanetTextureConfig` struct at `src/rendering/ProceduralTextures.h:22` with planet type presets (`TERRESTRIAL`, `GAS_GIANT`, `ICE_WORLD`, `BARREN`, `LAVA_WORLD`, `CUSTOM`)

### 3.2 3D Model Data Export

#### 3.2.1 Geometry Data Availability

The rendering system exposes geometry data through standardized interfaces but does **not** include a dedicated mesh export pipeline (OBJ/STL/glTF/PLY writers are not present in the current codebase).

**Available geometry sources:**

| Source | Method | Data Format |
|--------|--------|-------------|
| CurvatureRenderer | `getVertices()` | `std::vector<CurvatureVertex>` (position[3], normal[3], color[4], curvature, time_dilation) |
| QuantumGeometryRenderer | `getGeometryVertices()` / `getGeometryEdges()` / `getGeometryColors()` | `std::vector<double>` / `std::vector<int>` |
| CDTEngine | `getGeometryVertices()` / `getGeometryEdges()` / `getGeometryColors()` | Raw float arrays |
| SpinFoamEngine | `getGeometryVertices()` / `getGeometryEdges()` / `getGeometryColors()` | Raw float arrays |
| GFTEngine | `getGeometryVertices()` / `getGeometryEdges()` / `getGeometryColors()` | Raw float arrays |
| CausalSetEngine | `getGeometryVertices()` / `getGeometryEdges()` / `getGeometryColors()` | Raw float arrays |
| CelestialBodyRenderer | `sphereVAO()` / `sphereIndexCount()` | OpenGL VAO + index count |
| LightCone | `getVertices()` | `std::vector<LightConeVertex>` |

#### 3.2.2 Data Export Capabilities

The project provides data-level export rather than mesh-level export:

| Export Type | Method | Format |
|-------------|--------|--------|
| Discovery results | `DiscoveryEngine::exportDiscovery()` | JSON |
| Hypotheses | `DiscoveryEngine::exportHypothesis()` | JSON |
| Theory comparison | `TheoryManager::exportComparison()` | String (JSON-like) |
| Findings | `DiscoveryPanelManager::exportFindings()` | QString (CSV/JSON) |
| Workflow analytics | `WorkflowAnalytics::exportJSON()` | JSON |
| LaTeX paper | `DiscoveryEngine::generateLaTeXPaper()` | LaTeX |
| Curvature features | `DiscoveryEngine::computeCurvatureFeatures()` | `std::vector<double>` (15 features) |

#### 3.2.3 Recommended Export Path

For users requiring 3D model export, the geometry data can be extracted via:

1. Access `CurvatureRenderer::getVertices()` for curvature grid meshes
2. Access `QuantumGeometryRenderer` geometry methods for quantum gravity structures
3. Access `CelestialBodyRenderer` sphere VAO data for celestial bodies
4. Write custom OBJ/STL/glTF serialization using the exposed vertex/index data

### 3.3 Immersive Visualization

#### 3.3.1 4D Viewport

The central visualization supports:
- **Two-slice projection:** 4D → 3D → 2D with configurable slicing hyperplane
- **Slicing modes:** `FIXED_T`, `PROPER_TIME`, `NULL_SLICE`, `COMPLEX_REAL/IMAG`
- **6 rotation planes:** XY, XZ, YZ, TX, TY, TZ via SO(4) rotations
- **4D camera:** Position `(t, x, y, z)`, look-at, dual up-vectors
- **Framebuffer:** Off-screen FBO with ImGui display

#### 3.3.2 Planck Microscope

**File:** `src/ui4d/PlanckMicroscope.h`
- Logarithmic zoom: 10⁻³⁵ m to 10⁰ m (35 orders of magnitude)
- LOD system with automatic level switching
- Quantum foam visualization at Planck scale

#### 3.3.3 VR Support

**Files:** `src/vr/OpenXRBackend.h`, `src/vr/MultiUserServer.h`
- OpenXR backend with stereo projection matrices
- Head pose + controller state tracking
- Multi-user collaboration server (network protocol implemented, WebRTC deferred)
- Build flag: `QUANTUMVERSE_USE_VR` (defaults to OFF)

### 3.4 Performance Specifications

| Scenario | Target | Hardware |
|----------|--------|----------|
| Solar System only | 60 FPS | GTX 1650+ |
| + Singularities | 45 FPS | RTX 3070 |
| + Full geodesics | 30 FPS | RTX 3070 |
| + 4D UI all views | 30 FPS | High-end GPU |
| Memory usage | <2 GB | 16 GB RAM |
| Startup time | <5 sec | SSD |

**Optimization techniques:**
- GPU instancing for grid cells and light cones
- Level-of-detail (LOD) based on 4D distance
- Spatial partitioning for grid updates
- Metric/curvature caching with validity flags
- Multi-threaded geodesic integration via `ThreadPool`
- Persistent buffer mapping for dynamic geometry

---

## Summary

The QuantumVerse Simulator provides a production-ready, mathematically rigorous platform for 4D spacetime cognition. Its cosmological framework spans classical GR, quantum gravity candidates (CDT, LQG, GFT, Causal Sets), modified gravity (f(R), Brans-Dicke), and string cosmology (dilaton gravity, brane inflation). The scientific foundations are validated against six independent GR tests with tight tolerances. Technical outputs include real-time OpenGL 4.5 curvature and quantum geometry rendering, 17 cosmological discovery instruments, and multi-messenger data ingestion from LIGO, IceCube, TESS, and Fermi GBM.
