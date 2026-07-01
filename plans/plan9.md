# GUI Plan: QuantumVerse Solar System Scope-Inspired Explorer

## 1. Goal & Vision

Create an intuitive, visually stunning, and highly interactive graphical user interface for the **QuantumVerse 4D Spacetime Simulator**, inspired by the aesthetic and interaction paradigm of [Solar System Scope](https://www.solarsystemscope.com/). The interface must expose the full power of QuantumVerse—general relativity, quantum gravity, AI-driven discovery, and Planck‑scale exploration—through the approachable, game‑like experience of a modern solar system explorer.

**Core principles:**

- **Immediate usability:** A new user should be able to navigate a 4D spacetime, click on objects, zoom, and adjust time without reading documentation.
- **Depth without clutter:** The advanced features (Planck microscope, AI anomaly detection, theory plugins) remain accessible but are presented progressively.
- **Visual fidelity:** Realistic planetary rendering, smooth orbit/geodesic paths, real‑time curvature deformation, and seamless quantum‑scale transitions.

---

## 2. UX/UI Design Overview

Adopt a Solar System Scope‑like layout with:

- **Main 3D/4D Viewport** (center) – full OpenGL 4.5 rendering, supports multiple projections and overlays.
- **Left Sidebar** – object browser and properties panel, including search, filters (by type: black holes, stars, exotic objects), and detail panels (metrics, curvature invariants, quantum data).
- **Bottom Timeline Bar** – play/pause, time speed control, global proper time slider, and 4D slice index selector.
- **Top Toolbar** – quick actions: add celestial body, open Planck microscope, toggle overlays (worldlines, light cones, event horizons, causal structure), switch theory, launch discovery scan.
- **Right Info Panels** (collapsible) – Planck microscope controls, AI discovery logs, parameter sweep status, multi‑messenger event feed.
- **Mini‑map / 4D slice overview** – optional small inset showing a down‑projected view of the full 4D slice (replaces the current multiple‑slice‑view default, but multi‑view can be triggered as needed).

**Interaction patterns:**

- Click an object → sidebar shows its properties; camera automatically pivots to center on it.
- Double‑click → zoom in smoothly.
- Right‑click → context menu (edit object, trace geodesics, start parameter sweep from this point).
- Scroll wheel → zoom (logarithmic at large scales, linear at human scale, infinite zoom for Planck microscope via shift+scroll).
- Drag (with modifier keys) → rotate in any of the 6 SO(4) planes; tooltip shows current rotation plane.

---

## 3. Technical Architecture

### 3.1 Integration with Existing Codebase

> **Status: IMPLEMENTED** (2026-05-29)  
> The GUI was implemented using **Dear ImGui + GLFW** instead of Qt Quick/QML, providing a self-contained executable with zero DLL dependencies.

```mermaid
graph TD
    A[ImGui GUI Layer<br/>(Dear ImGui + GLFW)] --> B[OpenGL Viewport<br/>(GLFW + GLAD)]
    A --> C[Sidebar Panels]
    A --> D[Timeline Controls]
    B --> E[CurvatureRenderer<br/>QuantumGeometryRenderer]
    E --> F[Spacetime/Physics/Quantum Modules]
    B --> G[Camera4D<br/>(C++ implementation)]
    C --> H[Object Manager<br/>(SceneGraphManager)]
    C --> I[Discovery Engine APIs]
    D --> J[Time Controller]
```

**Key components:**

- `SceneGraphManager`: Holds a structured list of all sim objects (stars, black holes, particles, quantum geometries). Each object exposes properties (mass, spin, position, worldline, metric parameters) for ImGui binding.
- `Camera4D`: 4D camera with position in spacetime, rotation in 6 planes, and zoom control.
- `PlanckMicroscopeOverlay`: Special zoom controller that enables logarithmic zoom over 35 orders of magnitude, switching LOD levels on‑the‑fly.
- `DiscoveryUIMediator`: Bridges the DiscoveryEngine to ImGui panels.
- `ContextMenu`: Right-click context menu for physics tools (test GR, drop particle, measure precession).
- `FloatingPanels`: Anomaly alert system with real-time notifications.

### 3.2 Rendering Pipeline

Extend the existing `CurvatureRenderer` and `QuantumGeometryRenderer` to support:

- **Orbit/worldline rendering**: GL lines with colour‑coded affine parameter, thickness proportional to mass, and dashed style for spacelike paths.
- **Celestial body spheres**: Instanced rendering with realistic textures (or procedurally generated), lit by a point light at the central mass (or arbitrary light sources).
- **Event horizon / ergosphere surfaces**: Semi‑transparent shells, editable via sidebar.
- **Planck microscope LOD**: Already implemented; ensure smooth fade transitions when passing LOD thresholds.
- **Overlay system**: A set of toggleable 2D/3D overlays (light cones, causal graphs, curvature heatmaps) composited in the same viewport using a layered framebuffer approach.

### 3.3 Data Flow

- User interaction (ImGui) → C++ handler → physics engine → new simulation state → renderer → GPU to viewport.
- AI discovery events (anomaly detected) → push notification to ImGui via callback → displayed in a non‑intrusive popup or log panel.
- Parameter sweep status (long‑running task) → progresses via std::async → updates progress bar in sidebar.

---

## 4. Implementation Phases & Estimated Timeline

### Phase 0: Foundation (2 weeks)

> **Status: COMPLETED** (2026-05-29)

- Set up Dear ImGui + GLFW project with CMake.
- Integrate existing `CMakeLists.txt` to build static libraries (`quantumverse_core`, `quantumverse_physics`, etc.) and link against ImGui.
- Create a minimal GLFW window with an empty OpenGL viewport (using GLAD for function loading).
- Verify that existing `CurvatureRenderer` can render into the viewport.
- **Result**: `vcpkg.json` manifest created, all 48 targets compile successfully.

### Phase 1: Core Viewport & Navigation (4 weeks)

- Implement `SceneGraphManager` reading from `Event4D` and `MetricTensor` databases.
- Render basic celestial bodies (spheres) at worldline positions.
- Implement mouse/keyboard navigation via `Camera4DAdapter`, exposing rotation‑plane selection through a UI toolbar (e.g., six small buttons with axis labels).
- Add smooth zoom, pitch, yaw, and 4D slice advance (using arrow keys / timeline scrub).
- Benchmark and profile to maintain 60 fps during typical 4D rotation.

### Phase 2: Sidebar & Object Interaction (3 weeks)

- Build the QML sidebar with object list (tree or flat), search field, and detail tabs.
- Populate detail tabs: general info (mass, charge, spin), GR properties (Riemann invariants, event horizon radius), quantum properties (if applicable), and worldline plot.
- Implement click‑to‑select: ray‑cast from 2D screen coordinates into 4D scene (project onto the current 3D slice) to pick an object.
- Enable double‑click auto‑orbit and right‑click context menu.

### Phase 3: Timeline & Global Time Control (2 weeks)

- Design a bottom‑aligned timeline panel with:
  - Play/Pause/Stop buttons.
  - Speed multiplier slider (1× to 10⁶×).
  - Global proper time display (tau and coordinate t).
  - Slice index jog (for navigating fixed‑t slices).
- Bind to the existing `SimulationController` (to be created / extended from `QuantumVerseApp`).

### Phase 4: Overlays & Visual Flourish (3 weeks)

- Add toggleable overlays: orbits (worldlines), light cones, event horizons, ergospheres, causal links.
- Implement real‑time curvature deformation (grid shader) as a background overlay option.
- Add smooth transition between classical GR rendering and quantum geometry modes (switchable in Planck microscope).
- Integrate the `QuantumGeometryRenderer` LOD system with a beautiful zoom‑in effect (e.g., exponential zoom with depth‑of‑field blur).

### Phase 5: Planck Microscope Integration (3 weeks)

- Create a dedicated floating panel (or right‑side dock) with a logarithmic zoom slider (1 m ↔ 10⁻³⁵ m) and a theory selector (CDT, Spin Foam, GFT, Causal Sets).
- When zoom crosses a threshold, dynamically swap the renderer or change visualisation parameters without stutter.
- Display theory‑aware info in the sidebar as the user zooms in (spectral dimension, Hausdorff dimension, vertex amplitudes, etc.).

### Phase 6: Discovery & AI Panels (3 weeks)

- Build a “Discovery Console” panel:
  - Real‑time anomaly feed (curvature spikes, energy condition violations).
  - Start/stop an RL discovery agent, view its current exploration path.
  - Launch parameter sweeps with a simple form (parameter ranges, number of steps) and see results as a table/plot.
- Interface with `DiscoveryEngine` and provide buttons for exporting hypotheses (LaTeX, CSV) using the existing plan2.9 generator.

### Phase 7: Multi‑Messenger Integration & Polish (2 weeks)

- Pull LIGO events (via `MultiMessengerAdapter`) and render them as small waveforms or markers in the viewport.
- Add live connection status indicator.
- Performance tuning: LOD, occlusion culling, GPU instancing for many objects.
- UX testing, user feedback, and documentation.

### Phase 8: Optional Web Deployment (4 weeks, parallel)

- Investigate compiling QuantumVerse core to WebAssembly using Emscripten.
- Replace OpenGL with WebGL 2.0 backend; adapt UI to HTML/CSS/JS (or use Qt for WebAssembly if feasible).
- Provide a hosted version similar to solarsystemscope.com.

**Total estimated core effort (Phases 0–7): 22 weeks (~5.5 months)** with a small team (2–3 developers). Phases 4–6 can partially overlap.

---

## 5. Technology Stack

| Layer | Technology | Justification |
|-------|------------|---------------|
| UI framework | Dear ImGui 1.90+ with GLFW | Immediate mode UI, zero DLL dependencies, cross-platform |
| 3D rendering | OpenGL 4.5 via GLAD | Leverages existing C++ renderers |
| Backend | Existing C++20 modules (physics, math, quantum gravity, AI) | Upgraded from C++17 to C++20 |
| Data binding | Direct C++ function calls | No QML binding needed with immediate mode UI |
| Build system | CMake 3.20+ (existing) | Consistent with current project |
| Optional toolkits | ImGui docking branch, ImGui widgets | Flexible layout and controls |
| AI panel | Python subprocess (or embedded ONNX Runtime) | Existing ML training in Python, inference via ONNX C++ API |

---

## 6. Risk Analysis & Mitigation

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Performance drop with 4D rotation + ImGui overhead | Low | Medium | Profile early; use threaded physics simulation; ImGui is lightweight |
| Planck microscope LOD transitions cause stutter | Low | Medium | Pre‑load all LOD data; smooth blending shaders |
| Learning curve for ImGui integration with OpenGL | Low | Low | ImGui has excellent OpenGL examples; straightforward integration |
| Incompatibility of existing UI4D with new architecture | Low | High | New UI wraps existing modules via adapters, does not alter internal APIs |
| User expects web version immediately | Medium | Low | Clearly communicate timeline; Phase 8 optional |

> **Note**: The Qt → ImGui migration was successfully completed, eliminating the original risks related to Qt DLL dependencies and LGPL licensing.

---

## 7. Success Metrics

- A new user can navigate a basic Schwarzschild spacetime and see perihelion precession within 2 minutes.
- Planck microscope zoom from astronomical scale to 10⁻³⁵ m is smooth and visually informative.
- All existing tests (71+) continue to pass.
- Frame rate remains ≥ 45 fps during typical interaction on a mid‑range GPU (NVIDIA GTX 1660 equivalent).

---

## 8. Next Steps

> **Status: COMPLETED** (2026-05-29)

1. **Immediate**: ✅ Created prototype ImGui application with OpenGL viewport and 4D rendering.
2. **Week 1–2**: ✅ Implemented all UI panels (SliceView, PlanckMicroscope, Discovery, CausalGraph, TheoryManager).
3. **Then**: ✅ Completed all phases - see `MIGRATION_SUMMARY.md` for full details.

### Future Enhancements

- [ ] Add OpenGL shader-based rendering for curvature
- [ ] Implement framebuffer-based picking
- [ ] Add screenshot/video capture
- [ ] Save/load layout presets
- [ ] Add performance profiler overlay
- [ ] Implement undo/redo system
- [ ] Add Python scripting interface
- [ ] Multi-threaded physics simulation

This plan transforms QuantumVerse into a powerful, accessible, and visually captivating scientific tool, ready for both research and public outreach, much like Solar System Scope did for the solar system.