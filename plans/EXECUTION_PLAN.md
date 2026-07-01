# QuantumVerse Complete Global Integration Plan — Execution Plan

**Date**: 2026-05-15
**Author**: DHIAEDDINE0223
**Status**: READY FOR REVIEW  
**Version**: 2.1.0 ("Texture Integration" Release)  

---

## Executive Summary

This document provides a phased execution plan for the QuantumVerse Simulator global integration. It assesses the current state of the codebase, identifies critical gaps, and provides a step-by-step execution roadmap across 5 phases with checkpoints.

**Current State**: The project has a solid foundation — dilaton gravity module builds and passes tests, quantum gravity engines (CDT, Spin Foam, GFT, Causal Sets) are implemented, ImGui GUI is partially wired, and ML/AI infrastructure exists as headers and training scripts. However, **critical integration gaps** prevent a full build.

---

## Current State Assessment

### ✅ What's Built and Working

| Component | Status | Evidence |
|-----------|--------|----------|
| Dilaton gravity module | ✅ Built & tested | `dilaton.lib`, `test_dilaton.exe` pass all tests |
| Event4D, MetricTensor | ✅ Functional | Header-complete, used throughout |
| SingularityHandler | ✅ Functional | 10 singularity types, Hawking temp/evaporation |
| GeodesicIntegrator | ⚠️ Partial | RK4 works, Christoffel symbols are placeholder |
| CurvatureRenderer | ⚠️ Partial | OpenGL grid + shaders work, Riemann computation is stub |
| Quantum gravity engines | ✅ Built & tested | CDT, SpinFoam, GFT, CausalSet all have passing tests |
| Planck Microscope | ✅ UI exists | Qt widget with 35-order zoom |
| ImGui GUI shell | ⚠️ Partial | Window + docking works, panels render but lack content |
| Discovery Engine | ⚠️ Skeleton | Full interface defined, CurvatureNormalizingFlow exists |
| ML modules | ⚠️ Skeleton | GeodesicNeuralODE, MetricGNN, RL agent headers exist |
| VR multiplayer | ⚠️ Stub | Classes exist, WebRTC signaling is placeholder |
| Theory Plugin system | ✅ Functional | TheoryManager registers plugins, comparison works |

### ❌ Critical Gaps

| Gap | Severity | Phase | Description |
|-----|----------|-------|-------------|
| CMakeLists.txt dilaton-only | **CRITICAL** | 0 | Only builds dilaton.lib; all other modules excluded |
| launch_quantumverse.bat CMake args | **CRITICAL** | 0 | Uses `QUANTUMVERSE_USE_QT`, `QUANTUMVERSE_BUILD_TESTS` — not recognized by current CMake |
| main_glfw.cpp missing classes | **CRITICAL** | 0 | References `CausalGraph`, `DiscoveryProbe` — not defined as concrete classes |
| GeodesicIntegrator Christoffel | **HIGH** | 1 | Placeholder returns zero; geodesics in curved spacetime don't bend |
| CurvatureRenderer Riemann | **HIGH** | 1 | computeRiemannComponent returns 0.0; no real curvature visualization |
| UI4D slice rendering | **HIGH** | 1 | SliceView::updateSlice() generates grid but no actual hypersurface intersection |
| 11 test stubs | **MEDIUM** | 0 | test_gravity, test_spacetime, etc. are `assert(1==1)` |
| QuantumGeometryRenderer::render | **HIGH** | 2 | Declared but not implemented |
| DiscoveryEngine methods | **MEDIUM** | 3 | Most methods are declared, not implemented |
| VR signaling server | **MEDIUM** | 4 | Stub only |
| Phase 5 optimization | **LOW** | 5 | Not started — profiling, BVH, GPU compute, neural ODE inference |

---

## Phase Execution Plan

### PHASE 0: Foundation & Build System Modernisation

**Goal**: Make the project fully buildable with all modules integrated.

#### Tasks

| # | Task | Description | Dependencies | Est. Effort |
|---|------|-------------|--------------|-------------|
| 0.1 | **Fix CMakeLists.txt** | Extend from dilaton-only to full module build: spacetime, physics, math, rendering, ui4d, ui_imgui, discovery, quantumgravity, ml, vr, paper, bayesian | None | 4h |
| 0.2 | **Fix launch_quantumverse.bat** | Update CMake options to match actual CMakeLists.txt variables; remove stale `QUANTUMVERSE_USE_QT`, add `QUANTUMVERSE_USE_IMGUI` | 0.1 | 1h |
| 0.3 | **Create CausalGraph class** | Implement as concrete class in `src/ui4d/CausalGraph.h/.cpp` (currently only forward-declared in UI4D_ImGui) | None | 3h |
| 0.4 | **Create DiscoveryProbe class** | Implement as concrete class in `src/ui4d/DiscoveryProbe.h/.cpp` | None | 2h |
| 0.5 | **Fix main_glfw.cpp** | Update to use correct class names, link against full module set | 0.1–0.4 | 2h |
| 0.6 | **Fix QuantumVerseApp** | Remove Qt conditional paths; ensure ImGui path is self-contained | 0.1–0.5 | 2h |
| 0.7 | **Implement stub tests** | Flesh out `test_gravity.cpp`, `test_spacetime.cpp`, `test_event4d.cpp`, `test_metric_tensor.cpp`, `test_geodesic.cpp`, `test_singularity.cpp`, `test_integration.cpp`, `test_solar_system.cpp`, `test_relativity.cpp`, `test_curvature_renderer.cpp` with real assertions | Phase 1 core classes | 8h |
| 0.8 | **Build & test** | Run `ctest --output-on-failure` — all tests must pass | All above | 1h |

**Checkpoint 0**: `ctest` passes with ≥20 test executables. `quantumverse_imgui.exe` launches with ImGui docking workspace.

---

### PHASE 1: Core 4D Spacetime Engine

**Goal**: Make spacetime physics accurate — geodesics bend in curved spacetime, curvature is computed correctly.

#### Tasks

| # | Task | Description | Dependencies | Est. Effort |
|---|------|-------------|--------------|-------------|
| 1.1 | **Fix GeodesicIntegrator Christoffel symbols** | Implement analytical Christoffel symbol computation for Schwarzschild and Kerr metrics; replace placeholder numerical approach | MetricTensor, SingularityHandler | 6h |
| 1.2 | **Fix CurvatureRenderer Riemann tensor** | Implement `computeRiemannComponent()` using metric derivatives; compute Kretschmann scalar from Riemann | 1.1 | 6h |
| 1.3 | **Implement proper slicing** | Fix `SliceView::updateSlice()` to compute actual 3D hypersurface intersection with 4D geometry | UI4D, MetricTensor | 4h |
| 1.4 | **Fix Camera4D projection** | Implement proper 4D→3D→2D projection pipeline (two successive projection matrices) | UI4D.h | 3h |
| 1.5 | **Wire ImGui input to Camera4D** | Ensure WASD+QE and mouse input properly control 4D camera in `main_glfw.cpp` | 0.1–0.6 | 2h |
| 1.6 | **Integrate CurvatureRenderer with ImGui** | Render curvature grid into ImGui framebuffer; display in 4D View panel | CurvatureRenderer, UI4D_ImGui | 4h |
| 1.7 | **Add light cone visualization** | Implement null geodesic mesh generation in CurvatureRenderer | 1.2 | 3h |
| 1.8 | **Validation: Mercury precession** | Test that Schwarzschild geodesic reproduces 43"/century perihelion advance | 1.1 | 2h |
| 1.9 | **Validation: Light deflection** | Test that null geodesics reproduce 1.75" Eddington deflection | 1.1 | 2h |

**Checkpoint 1**: Geodesics visibly bend around black hole in ImGui viewport. Mercury precession within 1% of 43"/century. Light deflection within 1% of 1.75".

---

### PHASE 2: Advanced Physics & Quantum Gravity Integration

**Goal**: Connect quantum gravity engines to the rendering pipeline.

#### Tasks

| # | Task | Description | Dependencies | Est. Effort |
|---|------|-------------|--------------|-------------|
| 2.1 | **Implement QuantumGeometryRenderer::render()** | Wire up CDT/SpinFoam/GFT/CausalSet geometry to OpenGL VBOs and render in ImGui viewport | QuantumGeometryRenderer.h, quantum gravity engines | 8h |
| 2.2 | **Connect PlanckMicroscope to ImGui** | Wire `PlanckMicroscopePanel` to actual `PlanckMicroscope` class; implement LOD switching | PlanckMicroscope.h, QuantumGeometryRenderer | 4h |
| 2.3 | **Implement theory switching** | When user selects "CDT" / "Spin Foam" / etc. in TheoryManagerPanel, update QuantumGeometryRenderer | UI4D_ImGui, TheoryManager | 2h |
| 2.4 | **Wire SingularityHandler to MetricTensor** | Ensure `updateMetricForGR()` in QuantumVerseApp uses SingularityHandler's metric methods | SingularityHandler, MetricTensor | 2h |
| 2.5 | **Add regular black hole support** | Wire Hayward, Bardeen, Loop Quantum, Gauss metrics into SingularityHandler and test | SingularityHandler (already has methods) | 3h |
| 2.6 | **Implement gravitational redshift test** | Validate Δλ/λ = GM/(c²r) for Schwarzschild metric | 1.1, 2.4 | 2h |
| 2.7 | **Implement frame-dragging test** | Validate Lense-Thirring 39 mas/year for Kerr metric | 1.1, SingularityHandler | 2h |

**Checkpoint 2**: Quantum geometry (CDT spin network, etc.) renders in ImGui viewport. Planck microscope zooms smoothly. All GR validation tests pass.

---

### PHASE 3: AI-Accelerated Discovery Engine

**Goal**: Integrate ML/AI modules for autonomous discovery.

#### Tasks

| # | Task | Description | Dependencies | Est. Effort |
|---|------|-------------|--------------|-------------|
| 3.1 | **Implement DiscoveryEngine core** | Implement `detectAnomaly()`, `generateFieldEquation()`, `proposeHypothesis()`, `testHypothesis()` | DiscoveryEngine.h, DiscoveryEngine.cpp (stub) | 8h |
| 3.2 | **Integrate CurvatureNormalizingFlow** | Wire ONNX model inference into DiscoveryEngine for anomaly scoring | CurvatureNormalizingFlow.cpp, cmake/FindONNXRuntime.cmake | 4h |
| 3.3 | **Implement GeodesicNeuralODE inference** | Load ONNX model, replace numerical RK4 with neural ODE for fast geodesic integration | GeodesicNeuralODE.cpp, GeodesicIntegrator | 6h |
| 3.4 | **Implement MetricGNN inference** | Load ONNX model, predict metric from mass distribution | MetricGNN.cpp | 4h |
| 3.5 | **Implement RLDiscoveryAgent** | Wire PPO/SAC agent for theory parameter space exploration | RLDiscoveryAgent.cpp, DiscoveryEngine | 6h |
| 3.6 | **Implement BayesianEvidenceCalculator** | Wire nested sampling for model comparison | BayesianEvidenceCalculator.cpp | 4h |
| 3.7 | **Implement PaperGenerator** | Wire LaTeX template filling, figure generation | PaperGenerator.cpp | 3h |
| 3.8 | **Wire DiscoveryPanel to DiscoveryEngine** | Connect ImGui DiscoveryPanel to live anomaly detection results | UI4D_ImGui DiscoveryPanel, DiscoveryEngine | 3h |
| 3.9 | **Build & test ML pipeline** | Ensure all ONNX models export correctly, C++ inference works | All ML modules | 4h |

**Checkpoint 3**: DiscoveryEngine detects anomalies in curvature data. Neural ODE produces geodesics within 1% of RK4 in <1ms. RL agent explores parameter space. Bayesian evidence comparison works.

---

### PHASE 4: High-Fidelity GUI & VR Metaverse

**Goal**: Complete the user interface and enable collaborative VR.

#### Tasks

| # | Task | Description | Dependencies | Est. Effort |
|---|------|-------------|--------------|-------------|
| 4.1 | **Complete ImGui panel system** | Object browser, property editor, timeline bar, bottom status bar with play/pause/speed | UI4D_ImGui, UI4D.h | 8h |
| 4.2 | **Implement docking layout** | Central 4D view + right/left/bottom docks using ImGui docking | UI4D_ImGui | 4h |
| 4.3 | **Add context menus** | Right-click on objects to edit, trace geodesics, run parameter sweep | UI4D_ImGui, GeodesicIntegrator | 3h |
| 4.4 | **Implement overlay system** | Orbits, light cones, event horizons, causal links, curvature heatmaps as toggleable overlays | CurvatureRenderer, UI4D | 4h |
| 4.5 | **Implement MultiUserServer** | Central authority, 30 Hz broadcast, last-writer-wins conflict resolution | MultiUserServer.cpp (stub) | 8h |
| 4.6 | **Implement VR client** | OpenXR integration, 6-DOF controllers, 4D rotation, avatar world-lines | VRClient.cpp (stub), SignalingServer | 12h |
| 4.7 | **Implement WebRTC signaling** | Node.js or C++ libdatachannel signaling server | SignalingServer.cpp (stub) | 6h |
| 4.8 | **Time-locking protocol** | Users can "park" at specific proper time and reconvene | MultiUserServer, UI4D | 4h |
| 4.9 | **Live data adapters** | WebSocket → GCN alerts for LIGO/Virgo/KAGRA, IceCube, CHIME | DiscoveryEngine, external APIs | 6h |
| 4.10 | **Anomaly alert system** | Popup notifications, auto-log to discovery panel when >3σ discrepancy | DiscoveryEngine, UI4D_ImGui | 3h |
| 4.11 | **Spatial audio** | Gravitational wave sonification (frequency ∝ curvature) | CurvatureRenderer, audio API | 4h |

**Checkpoint 4**: Two users in VR explore same spacetime. Anomaly alerts fire. Timeline playback works. All overlays functional.

---

### PHASE 5: Continuous Optimisation & Validation

**Goal**: Hit performance targets and validate against all known physics tests.

#### Tasks

| # | Task | Description | Dependencies | Est. Effort |
|---|------|-------------|--------------|-------------|
| 5.1 | **Comprehensive validation suite** | Mercury perihelion (43"/cent), Eddington deflection (1.75"), redshift, GP-B (39 mas/yr), Nordtvedt (|ω-1|<0.003) | Phase 1-2 physics | 6h |
| 5.2 | **Neural ODE performance** | Target <1ms inference, <1% endpoint error vs RK4 | Phase 3.3 | 4h |
| 5.3 | **GPU compute shaders** | Move curvature grid evaluation to GLSL compute shaders | CurvatureRenderer | 6h |
| 5.4 | **BVH spatial index** | Implement BVH for 4D metric queries | MetricTensor, spacetime | 6h |
| 5.5 | **Multi-threading** | TBB/OpenMP for N-body + geodesic bundles | GeodesicIntegrator | 6h |
| 5.6 | **Memory optimization** | Pooling, cache-friendly layouts, SIMD | All hot paths | 8h |
| 5.7 | **Profiling** | Integrate Tracy or Intel VTune; identify and fix bottlenecks | All | 4h |
| 5.8 | **Performance target: 60 FPS** | GTX 1660 equivalent, Solar System + 1 black hole | 5.1–5.7 | 4h |
| 5.9 | **Documentation** | API reference (Doxygen), user manual, tutorial videos | All | 8h |
| 5.10 | **Final integration & release** | End-to-end discovery workflow, bug fixes, v2.0 "Genesis" release tag | All | 4h |

**Checkpoint 5**: All GR validation tests pass. ≥45 FPS on target hardware. Full end-to-end discovery workflow functional.

---

## Timeline Summary

| Phase | Duration | Start | End | Key Deliverable |
|-------|----------|-------|-----|-----------------|
| 0 — Foundation | 5 days | Day 1 | Day 5 | Full build works, ImGui launches |
| 1 — Core 4D Engine | 10 days | Day 6 | Day 15 | Geodesics bend, curvature renders, GR validated |
| 2 — Quantum Gravity | 8 days | Day 16 | Day 23 | Quantum geometry in viewport, Planck microscope |
| 3 — AI Discovery | 12 days | Day 24 | Day 35 | Neural ODE, anomaly detection, RL agent |
| 4 — GUI/VR | 15 days | Day 36 | Day 50 | Multi-user VR, live data, full UI |
| 5 — Optimisation | 12 days | Day 51 | Day 62 | 60 FPS, all validations pass, v2.0 release |
| **Total** | **~62 days** | | | |

---

## Risk Register

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| CMake integration breaks existing dilaton tests | Medium | High | Keep dilaton as first CMake target; incremental addition |
| Neural ODE ONNX models don't export correctly | Medium | High | Keep RK4 as fallback; validate outputs match |
| VR motion sickness | High | Medium | Comfort mode (snap turns, reduced FOV) |
| WebRTC latency too high for VR | Medium | High | Peer-to-peer optimization, relay fallback |
| Missing Qt dependency blocks Qt build | Medium | Low | Focus on ImGui path first; Qt optional |

---

## Immediate Next Actions

1. **Review and approve this plan** (this document)
2. **Switch to Code mode** to begin Phase 0, Task 0.1 — extend CMakeLists.txt
3. **Build and verify** after each task completion
4. **Run `ctest --output-on-failure`** after Phase 0 completion

---

## Appendix: Source Files by Module

### Already Implemented (verified)
- `src/spacetime/` — Event4D.h, MetricTensor.h
- `src/physics/` — GeodesicIntegrator.h, SingularityHandler.h, DifferentiableGeodesicIntegrator.h, DifferentiableCurvature.h, ParameterizedMetrics.h
- `src/quantumgravity/` — CDTEngine, GFTEngine, SpinFoamEngine, CausalSet, SpinNetwork
- `src/ml/` — GeodesicNeuralODE, MetricGNN, CurvatureNormalizingFlow
- `src/rendering/` — CurvatureRenderer, QuantumGeometryRenderer, OrbitRenderer, CelestialBodyRenderer, Texture
- `src/ui4d/` — UI4D.h/.cpp, PlanckMicroscope, Camera4DAdapter, SceneGraphManager
- `src/ui_imgui/` — UI4D_ImGui.h/.cpp
- `src/discovery/` — DiscoveryEngine, TheoryManager, RLDiscoveryAgent, all discovery instruments
- `src/vr/` — VRCommon, MultiUserServer, VRClient, SignalingServer
- `src/bayesian/` — BayesianEvidenceCalculator
- `src/paper/` — PaperGenerator

### Needs Implementation / Completion
- `src/ui4d/CausalGraph.h/.cpp` — NEW (for ImGui panel)
- `src/ui4d/DiscoveryProbe.h/.cpp` — NEW (for ImGui panel)
- `src/physics/GeodesicIntegrator.cpp` — Christoffel computation (currently placeholder)
- `src/rendering/CurvatureRenderer.cpp` — Riemann tensor computation (currently placeholder)
- `src/rendering/QuantumGeometryRenderer.cpp` — render() method
- `src/discovery/DiscoveryEngine.cpp` — core algorithm implementations
- `tests/` — 11+ test files need real assertions