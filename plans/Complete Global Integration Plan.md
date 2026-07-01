# QuantumVerse Simulator – Complete Global Integration Plan

**Version**: 3.0 (Unified Master Plan)  
**Last Updated**: 2026-05-11  
**Status**: Consolidated from 15+ planning documents into a single actionable roadmap  

This document integrates all previous plans – from the 4D UI design, AI acceleration, discovery instruments, Qt→ImGui migration, Solar System Scope‑style interface, NASA texture integration, and the original project plan – into one coherent, end‑to‑end blueprint for building and evolving the QuantumVerse Simulator.

---

## Table of Contents

1. [Executive Vision & Core Mission](#1-executive-vision--core-mission)
2. [Guiding Design Principles](#2-guiding-design-principles)
3. [System Architecture Overview](#3-system-architecture-overview)
4. [Feature Inventory (Unified)](#4-feature-inventory-unified)
5. [Development Phases & Milestones](#5-development-phases--milestones)
   - Phase 0: Foundation & Build System Modernisation
   - Phase 1: Core 4D Spacetime Engine
   - Phase 2: Advanced Physics & Quantum Gravity Integration
   - Phase 3: AI‑Accelerated Discovery Engine
   - Phase 4: High‑Fidelity GUI & VR Metaverse
   - Phase 5: Continuous Optimisation & Validation
6. [Detailed Implementation Tasks (by Phase)](#6-detailed-implementation-tasks-by-phase)
7. [Technical Stack & Dependencies](#7-technical-stack--dependencies)
8. [Timeline & Resource Estimates](#8-timeline--resource-estimates)
9. [Success Criteria & KPIs](#9-success-criteria--kpis)
10. [Risk Mitigation](#10-risk-mitigation)
11. [Future Extensions (Post‑v3.0)](#11-future-extensions-post-v30)

---

## 1. Executive Vision & Core Mission

**QuantumVerse Simulator** is a cross‑platform, real‑time interactive laboratory for exploring, testing, and discovering physics in a 4‑dimensional Lorentzian manifold. Its mission is to **go beyond visualisation** – enabling users to manipulate spacetime, test General Relativity and its alternatives, simulate quantum gravity candidates, and automatically detect novel phenomena through AI‑driven discovery.

The platform targets three audiences:
- **Researchers** – test modified gravity theories, analyse multi‑messenger data, generate hypotheses.
- **Educators & students** – visualise relativistic effects, black holes, and quantum spacetime in an intuitive 3D/4D environment.
- **Citizen scientists** – explore exotic objects, collaborate in shared VR spaces, and contribute to discovery challenges.

**Unifying theme:** A single, modular codebase that progressively reveals complexity – from a beautiful Solar System explorer to a full 4D spacetime engineering toolkit.

---

## 2. Guiding Design Principles

- **Direct 4D Manipulation** – Events, world‑lines, and curvature are native 4D objects; projection to screen is intuitive but does not collapse the extra dimension.
- **Multiple Simultaneous Views** – Primary 4D viewport, spatial 3D slices, time‑geography prisms, and diagnostic panels are linked and synchronised.
- **Causal & Geometric Clarity** – Visual cues (colour, shading, transparency) always distinguish time‑like, light‑like, and space‑like directions.
- **Progressive Disclosure** – Core solar system navigation is immediately usable; advanced features (Planck microscope, theory plugins, AI co‑pilot) appear only when needed.
- **Performance‑First** – Target 60 FPS on mid‑range GPUs (GTX 1660 equivalent) for typical scenes; use neural surrogates and GPU compute for heavy workloads.
- **Cross‑Platform & Open** – Windows, Linux, macOS; no proprietary dependencies; permissive licensing (MIT/zlib/BSD where possible).

---

## 3. System Architecture Overview

The architecture is layered, allowing independent development and testing of each tier.

```
┌───────────────────────────────────────────────────────────────┐
│                     UI Layer (ImGui + GLFW)                   │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌────────┐   │
│  │Main 4D View │ │Slice Views  │ │Planck Scope │ │Panels  │   │ 
│  └──────┬──────┘ └──────┬──────┘ └──────┬──────┘ └─────┬──┘   │
├─────────┼───────────────┼───────────────┼──────────────┼──────┤
│         │       Spacetime & Physics Layer (C++)        │      │
│  ┌──────▼──────┐ ┌──────▼──────┐ ┌──────▼──────┐ ┌─────▼────┐ │
│  │MetricTensor │ │GeodesicInte-│ │Curvature-   │ │Stress-   │ │
│  │(GR + mod.)  │ │grator       │ │Renderer     │ │Energy    │ │
│  └──────┬──────┘ └──────┬──────┘ └──────┬──────┘ └─────┬────┘ │
├─────────┼───────────────┼───────────────┼──────────────┼──────┤
│    │    Quantum Gravity Plugins (CDT, LQG, GFT, Causal Sets)  │
│    │    AI / ML Modules (Neural ODE, GNN, Normalising Flows)  │
│    │    Discovery Engine (RL agent, anomaly detection, Bayes) │
├─────────┼─────────────────────────────────────────────────────┤
│    └────►       Data Layer (live observatories, blockchain)   │
│                 OpenGL 4.5 Renderer (curvature, instancing)   │
└───────────────────────────────────────────────────────────────┘
```

**Key modules** (existing or to be built):
- `src/spacetime/` – Event4D, MetricTensor, DifferentiableMetric.
- `src/physics/` – GeodesicIntegrator, EinsteinSolver, StressEnergy.
- `src/quantumgravity/` – CDT, SpinFoam, GFT, CausalSet engines.
- `src/ml/` – Neural ODE, MetricGNN, NormalisingFlow, RLAgent.
- `src/discovery/` – AnomalyDetector, BayesianEvidence, TheoryRanker.
- `src/rendering/` – CurvatureRenderer, CelestialBodyRenderer (with NASA textures), QuantumGeometryRenderer.
- `src/ui4d/` – Camera4D, SliceView, ImGui panels, PlancoMicroscope.
- `src/data/` – MultiMessengerAdapter, BlockchainLogger.

---

## 4. Feature Inventory (Unified)

The following features are drawn from all previous plans and organised by capability area.

### 4.1 Solar System & Celestial Mechanics
- **High‑fidelity planetary rendering** – NASA‑sourced albedo, specular, normal, and cloud maps (equirectangular, up to 8K).
- **Keplerian + N‑body dynamics** – with optional GR corrections (perihelion precession, frame‑dragging).
- **Interactive object editor** – mass, radius, spin, charge, exotic flags (negative/imaginary mass, phantom energy).
- **Celestial body browser** – search, filter by type (star, planet, moon, black hole, exotic), quick‑focus.

### 4.2 4D Spacetime Visualisation
- **Main 4D viewport** – perspective projection from ℝ⁴ to screen, with 4D camera (position, look‑at, two up‑vectors).
- **Multiple 3D slice views** – fixed‑coordinate time, proper time along a world‑line, null slices, complex slices.
- **4D coordinate widget** – interactive gizmo showing camera orientation and slicing hyperplane.
- **World‑line rendering** – 1D curves coloured by causal classification; temporal brush for evolution.
- **Light cones & causal structure** – future/past cones as transparent 3‑surfaces; causal graph panel.
- **Curvature visualisation** – deformable 4D grid, colour‑mapped by Kretschmann/Ricci/Weyl; geodesic spray; tidal force vectors.
- **Singularities & horizons** – translucent event horizons, ergospheres, ring singularities (Kerr).

### 4.3 Time‑Geography & Constraints
- **Capability prisms** – 4D volumes of reachable events given speed/energy limits.
- **Coupling zones** – tubes where two world‑lines can intersect.
- **Authority regions** – exclusion zones with hatched rendering.
- **Space‑time aquarium** – bundled world‑line trajectories for multiple agents.

### 4.4 Topology & Complex Spacetime
- **Topology selector** – manifold (standard), Zeeman (fine), Alexandrov (coarse) with visual indicators of openness.
- **Complex spacetime mode** – coordinates in ℂ⁴; real/imaginary sectors colour‑coded or side‑by‑side.
- **Wick rotation** – toggle Minkowski ↔ Euclidean signature; observe effects on geodesics.

### 4.5 Quantum Gravity & Planck‑Scale Physics
- **Causal Dynamical Triangulations (CDT)** – Monte Carlo evolved 4D simplicial manifolds; measure spectral/Hausdorff dimension.
- **Spin foam engine (LQG)** – SU(2) spin networks evolving via EPRL vertex amplitudes.
- **Group Field Theory (GFT)** – tensor model interactions; emergent spacetime condensates.
- **Causal sets** – random sprinkling; Benincasa–Dowker action; Myrheim–Meyer dimension estimator.
- **Planck microscope** – logarithmic zoom (1 m → 10⁻³⁵ m) with LOD switching; displays spectral dimension and quantum‑corrected metrics.

### 4.6 AI & Autonomous Discovery
- **Geodesic Neural ODE** – 100× speed‑up for ray/geodesic integration (<1 ms inference).
- **Metric surrogate GNN** – predicts metric tensor from mass distribution (10–100× faster than numerical relativity).
- **Normalising flow anomaly detector** – unsupervised outlier detection on curvature patterns (μs inference).
- **RL discovery agent** – explores theory parameter space, rewarded for novelty + validity + observational fit.
- **Differentiable simulator** – end‑to‑end gradients w.r.t. theory parameters (auto‑diff from Phase 1).
- **Large Physics Language Model (LPLM)** – fine‑tuned transformer to propose hypotheses from literature.
- **Automated LaTeX paper generator** – discovery reports, equations, figures, citations.

### 4.7 Multi‑Messenger & Live Data
- **Real‑time ingestion** – LIGO/Virgo/KAGRA, IceCube, CHIME/ASKAP, ZTF, EHT.
- **Anomaly alert service** – triggers parameter scans when observational discrepancy >3σ.
- **Bayesian evidence calculator** – model comparison between GR and modified theories.
- **Virtual observatory cross‑matcher** – subtracts simulated sky from real images to reveal residuals.

### 4.8 Collaborative VR & Metaverse
- **Multi‑user server** – central spacetime state; 30 Hz broadcast; last‑writer‑wins conflict resolution.
- **VR clients** – WebRTC data channels; each user has Camera4D + avatar world‑line; shared instruments.
- **Time‑locking protocol** – users can “park” at a specific proper time and reconvene.
- **Spatial audio** – gravitational wave sonification (frequency ∝ curvature).

### 4.9 UI & User Experience (ImGui + GLFW)
- **Docking workspace** – main 4D view as central panel; side panels for object browser, properties, timeline, discovery console.
- **Bottom timeline bar** – play/pause, time speed slider, global proper time display, slice selector.
- **Top toolbar** – quick actions: add body, Planck microscope, toggle overlays, switch theory, start discovery scan.
- **Context menus** – right‑click on any object to edit, trace geodesics, run parameter sweep.
- **Planck microscope panel** – logarithmic zoom slider, theory selector, real‑time spectral dimension readout.

### 4.10 Testing & Validation
- **Validation suite** – Mercury perihelion (43″/century), Eddington light deflection, Gravity Probe B, lunar laser ranging, binary pulsars.
- **Performance benchmarks** – geodesic integration speed, metric evaluation time, FPS with 100+ bodies.
- **Reproducibility module** – captures random seeds, software versions, configuration for all discoveries.

---

## 5. Development Phases & Milestones

The global plan is organised into **six logical phases**, each building on the previous. Phases can be executed sequentially, with some parallelisation where indicated.

### Phase 0: Foundation & Build System Modernisation (Weeks 1–5)
*Goal: Replace Qt with ImGui, set up modern build, and restore core rendering.*

| Task | Description | Status |
|------|-------------|--------|
| 0.1 | Integrate GLFW, Glad, Dear ImGui (docking branch) into CMake | Pending |
| 0.2 | Remove all Qt dependencies; rewrite main loop with ImGui | Pending |
| 0.3 | Port UI panels: main menu, docking workspace, bottom timeline, sidebars | Pending |
| 0.4 | Adapt Camera4D input to GLFW callbacks (multi‑viewport focus) | Pending |
| 0.5 | Restore `CurvatureRenderer` and basic planet rendering (solid colours) | Pending |
| 0.6 | Add `stb_image` and `Texture` class; load NASA textures (Earth, Jupiter, Sun) | Pending |
| 0.7 | Extend `CelestialBodyRenderer` with array texture support (sampler2DArray) | Pending |
| 0.8 | Implement planet UV mapping and shader modifications for diffuse/specular/normal maps | Pending |

**Deliverable:** ImGui‑based application showing a textured, rotating Earth with basic 4D camera controls.

### Phase 1: Core 4D Spacetime Engine (Weeks 6–20)
*Goal: Implement Minkowski/GR spacetime, geodesics, curvature, and 4D visualisation.*

| Task | Description |
|------|-------------|
| 1.1 | `Event4D` and `WorldLine` classes; proper time parametrisation |
| 1.2 | Minkowski metric with Lorentz transformations; time dilation / length contraction demo |
| 1.3 | General metric tensor interface; `SchwarzschildMetric`, `KerrMetric`, `FRWMetric` |
| 1.4 | Geodesic integrator (RK4, adaptive step) with support for massive and null particles |
| 1.5 | Curvature calculator (Christoffel → Riemann → Ricci → Kretschmann) |
| 1.6 | Main 4D viewport projection (4D→3D→2D via vertex shader) |
| 1.7 | `Camera4D` with full SO(4) rotation (6 planes) and slicing hyperplane control |
| 1.8 | Linked slice views (fixed‑t, proper‑time, null) |
| 1.9 | Light cone visualisation (transparent 3‑surfaces) and causal graph panel |
| 1.10 | Deformable 4D curvature grid (colour‑mapped) |

**Deliverable:** Interactive 4D Schwarzschild spacetime; user can drop test particles, see geodesics bend.

### Phase 2: Advanced Physics & Quantum Gravity Integration (Weeks 21–40)
*Goal: Add singularities, topology, complex spacetime, and quantum gravity candidates.*

| Task | Description |
|------|-------------|
| 2.1 | Singularity models: Schwarzschild, Kerr, Reissner‑Nordström, regular (Hayward, Bardeen) |
| 2.2 | Event horizon / ergosphere rendering (translucent shells) |
| 2.3 | Topology explorer (manifold, Zeeman, Alexandrov) with visual feedback |
| 2.4 | Complex spacetime mode (ℂ⁴) – real/imaginary sectors, Wick rotation toggle |
| 2.5 | Time‑geography layers: capability prisms, coupling zones, authority regions |
| 2.6 | Planck microscope – logarithmic zoom, LOD switching, theory selector (CDT/spin‑foam/GFT) |
| 2.7 | CDT engine – Monte Carlo moves, spectral dimension measurement |
| 2.8 | Spin foam engine – basic vertex amplitude, 3‑geometry transitions |
| 2.9 | Causal set sprinkling – dimension estimator, causal graph display |
| 2.10 | Quantum‑corrected black hole evaporation (Hawking flux) |

**Deliverable:** Complete Planck‑scale exploration; user can zoom from solar system down to 10⁻³⁵ m and see quantum geometry.

### Phase 3: AI‑Accelerated Discovery Engine (Weeks 41–60)
*Goal: Build neural surrogates, RL agent, differentiable simulator, and anomaly detection.*

| Task | Description |
|------|-------------|
| 3.1 | Differentiable simulator (auto‑diff wrapper for `GeodesicIntegrator` and `MetricTensor`) |
| 3.2 | Generate training dataset (10⁶ geodesics) – parallel RK4 |
| 3.3 | Train `GeodesicNeuralODE` (PyTorch → ONNX → C++ inference) |
| 3.4 | Train `MetricGNN` (from mass distribution to metric tensor) |
| 3.5 | Train normalising flow for curvature anomaly detection |
| 3.6 | Integrate neural surrogates into main simulator (fallback to numerical for validation) |
| 3.7 | `DiscoveryEnvironment` (OpenAI Gym interface) for RL agent |
| 3.8 | RL agent (PPO/SAC) – reward = novelty + validity – complexity + observational fit |
| 3.9 | Bayesian evidence calculator (nested sampling) for theory ranking |
| 3.10 | Large Physics Language Model (fine‑tuned on arXiv) – hypothesis generation |
| 3.11 | Automated LaTeX paper generator (Jinja2 templates, figures, BibTeX) |

**Deliverable:** AI co‑pilot that autonomously proposes new field equations and ranks them against observational data.

### Phase 4: High‑Fidelity GUI & VR Metaverse (Weeks 61–80)
*Goal: Deliver the Solar System Scope‑inspired interface, collaborative VR, and live multi‑messenger pipeline.*

| Task | Description |
|------|-------------|
| 4.1 | Complete ImGui panels: object browser, property editor, discovery console, timeline |
| 4.2 | Smooth zoom (logarithmic), double‑click to focus, right‑click context menu |
| 4.3 | Overlay system: orbits, light cones, event horizons, causal links, curvature heatmaps |
| 4.4 | Planck microscope panel with real‑time spectral dimension and quantum‑gravity theory info |
| 4.5 | Multi‑user server (central authority, broadcast 30 Hz) |
| 4.6 | VR client (OpenXR) – 6‑DOF controllers, 4D rotation, avatar world‑lines |
| 4.7 | Time‑locking protocol (users reconvene at same proper time) |
| 4.8 | Live data adapters: LIGO, IceCube, CHIME, ZTF (WebSocket → GCN) |
| 4.9 | Anomaly alert system – popup notifications, auto‑log to discovery panel |
| 4.10 | Virtual observatory cross‑matcher – image subtraction against SDSS, Gaia |
| 4.11 | Blockchain logger (IOTA DAG) – immutable discovery records |

**Deliverable:** A collaborative VR platform where two or more users can explore the same spacetime, see each other’s cursors, and receive real‑time anomaly alerts.

### Phase 5: Continuous Optimisation & Validation (Weeks 81–104+)
*Goal: Reach performance targets, validate against all known tests, and polish UX.*

| Task | Description |
|------|-------------|
| 5.1 | Comprehensive validation suite – Mercury, Eddington, GP‑B, binary pulsars |
| 5.2 | Performance benchmarks: geodesic integration (target <1 ms for neural ODE) |
| 5.3 | GPU compute shaders for curvature evaluation and grid deformation |
| 5.4 | Spatial hashing / BVH for metric queries |
| 5.5 | Multi‑threading (TBB, OpenMP) for N‑body + geodesic bundles |
| 5.6 | Memory pooling, cache‑friendly data layouts |
| 5.7 | User acceptance testing (target 60 FPS on GTX 1660 for typical solar system scene) |
| 5.8 | Documentation: API reference, user manual, tutorial videos |
| 5.9 | Final integration of all modules – end‑to‑end discovery workflow |
| 5.10 | Release QuantumVerse v2.0 (Genesis) |

**Deliverable:** A stable, high‑performance release that passes all validation tests and includes the full feature set.

---

## 6. Detailed Implementation Tasks (by Phase)

For brevity, only the most critical tasks are listed above. Each phase has a dedicated **sub‑plan** (see referenced files). Below is a consolidated task inventory with dependencies.

### Phase 0 Foundation – Subtasks
- [x] Set up CMake with GLFW, ImGui (docking), Glad, stb_image.
- [x] Create `main_glfw.cpp` – basic window + ImGui init.
- [x] Port UI4D manager to ImGui (no Qt signals/slots → observer pattern).
- [x] Implement `Texture` class (load from disk, generate mipmaps, bind).
- [x] Convert `CelestialBodyRenderer` to use `sampler2DArray`.
- [x] Write planet shader (diffuse, specular, normal mapping).
- [x] Download NASA textures: Earth, Jupiter, Sun (8K where available).

### Phase 1 Core – Subtasks
- [x] `Event4D` struct (t, x, y, z; imaginary flag).
- [x] `MetricTensor` virtual class; implement `MinkowskiMetric`, `SchwarzschildMetric`.
- [x] `GeodesicIntegrator` (RK4, Dormand‑Prince) with step‑size control.
- [x] `CurvatureRenderer` – compute Riemann via symbolic diff or finite difference.
- [x] `Camera4D` – position, look‑at, up1 (timelike), up2 (spacelike). Projection matrix 4×4 → 3×3.
- [x] `SliceViewPanel` widget – choose slice type (t‑const, tau‑const, null). Render as FBO texture → ImGui::Image.
- [x] `LightCone` – for a given event, sample null geodesics and generate mesh.

### Phase 2 Advanced – Subtasks
- [x] `BlackHole` class – mass, spin, charge; event horizon radius; ergosphere.
- [x] `TopologySwitcher` – change open‑set logic in rendering (Zeeman: refine along null/timelike directions).
- [x] `ComplexCoordinates` – split real/imag, Wick rotation method.
- [x] `TimeGeography` – `CapabilityPrism` compute reachable events via geodesic shooting.
- [x] `PlanckMicroscopePanel` – zoom slider (log10 meters); LOD thresholds (1e-15, 1e-30, 1e-35). Switch renderer.

### Phase 3 AI – Subtasks
- [x] `DifferentiableGeodesicIntegrator` – wrap numeric integrator, record Jacobians.
- [x] Python training scripts using PyTorch (for neural ODE, GNN, flow).
- [x] ONNX export and C++ inference (via ONNX Runtime or LibTorch).
- [x] `RLAgent` – stable‑baselines3 integration (C++ → Python bridge or pure C++).
- [x] `BayesianEvidence` – nested sampling (dynesty) or MCMC (PyMC) bridge.
- [x] `PaperGenerator` – collect discovery data, fill LaTeX template, call `pdflatex`.

### Phase 4 GUI/VR – Subtasks
- [x] Extend ImGui panels: object browser (tree), property editor (sliders), discovery log (table), timeline (play/pause/speed).
- [x] Implement docking layout (central view + right/left/bottom docks).
- [ ] WebRTC server (Node.js or C++ with libdatachannel).
- [x] VR integration – OpenXR (SteamVR/Oculus) with Camera4D binding.
- [ ] Live data adapters – cURL + JSON parsing for GCN alerts.
- [ ] Blockchain – IOTA client library (C++ API) to push discovery hashes.

### Phase 5 Optimisation – Subtasks
- [x] Profiling with Tracy or Intel VTune.
- [x] Replace RK4 geodesic with neural ODE (fallback for >1% error).
- [x] GPU compute shaders for curvature grid evaluation (GLSL compute).
- [ ] Implement BVH for metric queries (spatial index in 4D).
- [x] Multi‑threading: worker pool for N‑body force calculations.

---

## 7. Technical Stack & Dependencies

| Layer | Technology | Rationale |
|-------|------------|-----------|
| **UI** | Dear ImGui (docking) + GLFW | Immediate mode, flexible, no Qt licensing; docking branch provides multi‑panel workspace. |
| **Graphics** | OpenGL 4.5 + custom shaders | Cross‑platform, leverages existing code; compute shaders for curvature. |
| **Math** | Eigen3, GSL | Fast linear algebra, special functions. |
| **Physics** | Custom C++17 modules | Full control; easy to replace with neural surrogates. |
| **ML/AI** | PyTorch (training) → ONNX Runtime (inference) | Best of both worlds: Python for training, C++ for low‑latency inference. |
| **RL** | Stable‑Baselines3 (Python) + custom bridge | Reuse proven algorithms; can later convert to C++. |
| **VR** | OpenXR + OpenVR | Portable interface to SteamVR, Oculus, etc. |
| **Multi‑player** | WebRTC (libdatachannel) | Low‑latency, peer‑to‑peer with central signalling. |
| **Data** | cURL, JSON (nlohmann), SQLite (local cache) | Lightweight, no external server needed. |
| **Symbolic** | SymPy (via Python embedding) + optional Mathematica | Flexible; can generate analytic Christoffel symbols. |
| **Testing** | GoogleTest, Benchmark | Standard for C++. |

**Build system:** CMake ≥ 3.20 (with FetchContent for ImGui, GLFW, etc.)

**Minimum hardware (developer):** CPU: 4 cores, RAM: 16 GB, GPU: NVIDIA GTX 1660 / AMD equivalent, OpenGL 4.5 support.

---

## 8. Timeline & Resource Estimates

The total timeline to **QuantumVerse v2.0 (Genesis)** is **104 weeks (~2 years)** assuming a dedicated team of **3–5 developers** (mix of C++/graphics, physics/math, ML/AI). Phases can overlap where indicated.

| Phase | Duration (weeks) | Start Week | Dependencies | Parallelisable tasks |
|-------|-----------------|------------|--------------|----------------------|
| 0 – Foundation | 5 | 1 | None | – |
| 1 – Core 4D | 15 | 6 | Phase 0 | – |
| 2 – Advanced Physics | 20 | 21 | Phase 1 | Planck microscope (2.6) can start after 2.2 |
| 3 – AI Discovery | 20 | 41 | Phase 1 (differentiable needs 1.4) | GNN training (3.4) overlaps with Neural ODE (3.3) |
| 4 – GUI/VR | 20 | 61 | Phase 0,2,3 | VR can be separate team |
| 5 – Optimisation | 24 | 81 | Phases 1‑4 | continuous, but final polish at end |

**Total:** 104 weeks (2 years) for full v2.0 release.  
An **alpha** can be released after Phase 2 (week 40), **beta** after Phase 3 (week 60).

**Resource estimate (person‑years):** ~8–12 total (3–5 developers × 2 years).

---

## 9. Success Criteria & KPIs

Each phase has its own acceptance criteria; the overall project success is measured by:

- [ ] **All validation tests pass** (Mercury perihelion within 1% of GR prediction, light deflection exact, etc.)
- [ ] **Neural ODE** achieves <1 ms inference with <1% endpoint error (compared to RK4).
- [ ] **RL agent** discovers ≥1 novel anomaly not present in training set.
- [ ] **Planck microscope** zooms smoothly from 1 AU to 10⁻³⁵ m without stutter.
- [ ] **Multi‑user VR** supports 2+ users with <100 ms latency and shared time‑locking.
- [ ] **Performance** – ≥45 FPS on GTX 1660 for Solar System + 1 black hole scenario.
- [ ] **Documentation** – user manual, API reference, tutorial videos.
- [ ] **Community** – at least 10 external users successfully run and modify a discovery experiment.

---

## 10. Risk Mitigation

| Risk | Likelihood | Impact | Mitigation Strategy |
|------|------------|--------|----------------------|
| ImGui lacks some Qt features (e.g., sophisticated tree models) | Medium | Low | Use custom `ImGui::TreeNode` with search; fallback to simpler layout. |
| Neural ODE training fails to generalise | Medium | High | Keep numerical integrator as fallback; use ensemble of surrogates. |
| VR motion sickness from 4D rotation | High | Medium | Provide “comfort mode” (snap turns, reduced FOV during rotation); smooth vs. discrete rotation options. |
| Real‑time multi‑messenger data overload | Low | Medium | Implement throttling and data‑quality flags; user‑adjustable alert thresholds. |
| Performance on macOS (OpenGL depreciation) | Medium | Medium | Use ANGLE or Metal via MoltenVK (future); keep OpenGL core profile as primary. |
| Symbolic integration (SymPy) becomes bottleneck | Low | Low | Pre‑compute and cache symbolic results; only compute on‑demand. |

---

## 11. Future Extensions (Post‑v3.0)

After the Genesis release, the following features are planned (not required for v2.0):

- **WebAssembly build** – run QuantumVerse in browser (similar to Solar System Scope online).
- **Full tensor network visualisation** (MERA, AdS/CFT bulk reconstruction).
- **Integration with quantum computers** (IBM Qiskit) for path integral amplitudes.
- **Crowdsourced discovery challenges** – leaderboard for stable wormhole designs.
- **Haptic feedback** – force feedback devices to feel tidal forces.
- **Higher dimensions (5D, 11D)** – extra compact dimensions visualised as colour or small loops.

---

## Appendix A: References to Original Plans

This global plan consolidates content from:

- `4d_dashboard_integration_plan.md` – UI4D enhancements, real‑time editing.
- `PHASE2_AI_ACCELERATION_PLAN.md` – Neural ODE, RL agent, differentiable simulator.
- `plan.md` – Original 4D UI design philosophy.
- `plan1.md` – AI‑driven discovery, theory plugins, multi‑scale twin.
- `plan2.md` – GASS, wormhole scanner, microlensing, causal set detector.
- `plan3.md` – Neutrino tomography, echo seeker, PTA quantum‑gravity interrogator.
- `plan4.md` – Solar gravitational lens, ISO anomaly, boson cloud superradiance.
- `plan5.md` – TTV fifth force, rotation curve scanner, fine‑structure drift.
- `plan6.md` – Globular cluster CMD, EBL fluctuation, magnetar QED.
- `plan7.md` – Advanced discovery roadmap (quantum gravity, multi‑messenger, holography).
- `plan8.md` – Qt → ImGui migration plan.
- `plan9.md` – Solar System Scope‑inspired GUI.
- `plan10.md` – NASA texture integration.
- `QuantumVerse_Project_Plan.md` – Original high‑level project plan.

All tasks, milestones, and technical decisions are derived from or aligned with these sources.

---

## End of Global Integration Plan

This document serves as the **single source of truth** for the QuantumVerse Simulator development. All future detailed plans (sprint tasks, API specs, testing protocols) must be consistent with this master blueprint.  

**Next immediate action:** Start **Phase 0, Week 1** – set up GLFW/ImGui build and port the main window.