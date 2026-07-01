## Migration Plan: Qt → Dear ImGui for QuantumVerse Simulator

### 1. Scope & Goals

**Objective**  
Replace all Qt6 dependencies with **GLFW + Dear ImGui** while keeping the entire physics, rendering, math, discovery, quantum gravity, data, and ML layers unchanged.

**What Changes**
- Entry point: `main_qt.cpp` → `main_glfw_imgui.cpp`
- UI layer: rewrite all panels currently in `src/ui4d/` (UI4D, Camera4D wrapper, SliceView, CausalGraph, DiscoveryProbe, PlanckMicroscope, topology explorer, toolbars, menus) as ImGui windows and custom widgets
- Event system: replace Qt signals/slots with GLFW callbacks + internal observer pattern
- Build: remove Qt6 from CMake, add GLFW, ImGui (docking branch), GLAD/GLEW (if not already)

**What Stays Intact**
- `src/spacetime/`, `src/physics/`, `src/rendering/`, `src/math/`, `src/discovery/`, `src/quantumgravity/`, `src/data/`, `src/ml/`
- All OpenGL 4.5 rendering code (CurvatureRenderer, QuantumGeometryRenderer)
- Camera4D logic (only the input-wiring changes)

**Licensing**  
GLFW (zlib/libpng), Dear ImGui (MIT) → fully permissive, no LGPL concerns.

### 2. Pre-Migration Audit

| Qt Component | Lines (est.) | ImGui Equivalent | Notes |
|--------------|--------------|------------------|-------|
| Main window + docking | ~3K | ImGui docking branch + GLFW window | Layout system similar, drag-panels |
| `QOpenGLWidget` viewports | ~5K | ImGui framebuffer + `ImGui::Image` or child window with shared OpenGL context | 4‑6 slice views plus main 4D view |
| `QSlider`, `QSpinBox`, `QPushButton` | many | ImGui widgets (`SliderFloat`, `InputInt`, `Button`, `Combo`) | Immediate‑mode, no retained objects |
| `QAction`, menus, toolbars | ~2K | ImGui main menu bar + custom toolbar bar | Quick to replicate |
| `QTreeView`, `QTableView` (TheoryManager, Discovery) | ~3K | ImGui `TreeNode` + `Table` (from docking branch) or custom list | Simpler but fully sufficient |
| `QString`, signals/slots | pervasive | `std::string`, simple callbacks | Qt‑specific utilities removed |
| Planck Microscope (custom OpenGL widget) | ~1K | ImGui child window + texture display + custom slider | The heavy OpenGL logic (LOD, rendering) remains unchanged; only hosting changes |
| 4D Camera input (mouse/keyboard) | ~2K | GLFW callbacks mapping to `Camera4D` | Need to handle multi‑viewport focus |

### 3. Detailed Migration Phases

#### Phase 0: Build System & Boilerplate (Week 1)
- **0.1** Integrate GLFW, Glad/GLEW, Dear ImGui (docking branch) into CMake
- **0.2** Delete `main_qt.cpp`, create `main_glfw.cpp` with GLFW window creation and ImGui initialisation
- **0.3** Establish a shared OpenGL context for all viewports (using GLFW’s offscreen context or texture sharing)
- **0.4** Create a minimal application loop: clear, render ImGui, swap buffers
- **0.5** Verify that existing `CurvatureRenderer` can draw into the main framebuffer

#### Phase 1: Skeleton Application Shell (Week 1–2)
- **1.1** Port the application menu bar (File, Edit, View, Tools, Help) using `ImGui::BeginMainMenuBar()`
- **1.2** Recreate the main docking workspace: a central 4D view and dockable side panels
- **1.3** Implement a `SliceView` panel as a dedicated ImGui child window that renders a framebuffer texture via `ImGui::Image()`. Each slice view will hold its own `Camera4D` state.
- **1.4** Restore core keyboard shortcuts (C, T, S, E, Ctrl+N, etc.) using GLFW key callbacks and ImGui’s input system
- **1.5** Keep the existing UI4D coordinator class, but refactor it to accept GLFW input and manage ImGui windows instead of Qt widgets.

#### Phase 2: 4D Camera & Input (Week 2–3)
- **2.1** Wrap GLFW mouse motion, scroll, and button events to update the active `Camera4D` (rotation in 6 planes, translation, zoom, slice offset).
- **2.2** Handle multiple viewports: only the hovered/focused ImGui window receives camera controls (track via `ImGui::IsWindowFocused()`).
- **2.3** Re‑implement modifier‑based rotation plane selection (Shift, Ctrl, Alt) using GLFW key modifiers.
- **2.4** Ensure the time‑geography overlay and topology explorer toggles still work.

#### Phase 3: UI Panels (Week 3–4)
- **3.1** **Planck Microscope** widget:
  - ImGui child window with a raw OpenGL viewport (using `ImGui::GetWindowDrawList()` and a custom texture or directly rendering during the window callback).
  - Logarithmic zoom slider (`ImGui::SliderFloat` with logarithmic mapping).
  - Theory info panel (text, parameters) and LOD indicator.
- **3.2** **DiscoveryProbe**: real‑time curvature readouts displayed as ImGui text, color‑coded fields.
- **3.3** **CausalGraph**: move the existing graph logic (nodes, links) into an ImGui drawing canvas (using `ImDrawList` primitives or a dedicated GL view).
- **3.4** **DiscoveryEngin**e controls: parameter sweep configuration, anomaly alerts as ImGui tables/callouts.
- **3.5** **TheoryManager** panel: tree of plugins using `ImGui::TreeNode`, active theory selection, metric export.

#### Phase 4: Integration & Testing (Week 4–5)
- **4.1** Connect all UI actions to the backend (unchanged): event creation, celestial body editing, discovery runs, LIGO adapter triggers.
- **4.2** Verify all GR validation tests still run (they are backend‑only, should be unaffected).
- **4.3** Compare visual output of curvature rendering and quantum geometry between old Qt build and new ImGui build—must be pixel‑identical.
- **4.4** Full functional test of all keyboard shortcuts, drag‑resize of panels, multiple slice views, Planck Microscope zoom transitions.
- **4.5** Performance profiling: ensure ImGui’s draw overhead does not degrade 4D rendering FPS (expected to be minimal; ImGui is efficient).

#### Phase 5: Clean‑up & Documentation (Week 5–6)
- **5.1** Remove all remaining Qt‑specific code (`#include <Q...>`, Qt resource files, `.ui` files).
- **5.2** Purge Qt from CMakeLists.txt and build instructions; update launcher scripts.
- **5.3** Write new `README` sections explaining GLFW+ImGui build, key bindings, and panel usage.
- **5.4** Create a short migration guide for developers familiar with the Qt version.

### 4. Risk & Mitigation

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Docking layout not as flexible as Qt’s | Medium | Minor | ImGui docking supports tabbing, splitting, floating – sufficient for 4–6 slice views. |
| Planck Microscope custom GL rendering requires off-screen framebuffer | Low | Medium | Use a separate FBO, render into it, then display as `ImGui::Image` – well‑documented pattern. |
| Input conflicts between ImGui and 4D camera | Medium | Medium | Use `ImGui::GetIO().WantCaptureMouse/WantCaptureKeyboard` to prevent camera movement when interacting with UI. |
| Performance drop due to immediate‑mode redraws | Very low | Low | ImGui is extremely fast; we still issue the same OpenGL draw calls. No retained widget overhead. |
| Developer unfamiliarity with ImGui | Medium | Low | ImGui has excellent documentation and many examples; learning curve is shallow. |

### 5. Timeline Estimate

| Phase | Task | Duration |
|-------|------|----------|
| 0 | Boilerplate, GLFW window, ImGui setup | 3 days |
| 1 | Application shell, main menu, docking workspace | 4 days |
| 2 | 4D camera input wiring | 5 days |
| 3 | All UI panels (SliceView, Planck Microscope, Discovery, Causal) | 10 days |
| 4 | Integration & testing | 7 days |
| 5 | Clean‑up, docs, removal of Qt | 3 days |
| **Total** | | **~5 weeks** (single developer, full‑time) |

Parallelisation possible: Phases 2 and 3 can partially overlap.

### 6. Important Technical Notes

- **OpenGL Context Sharing**: The existing renderers assume a current GL context. In the new design, all rendering (main 4D view, each slice view, microscope) will share a single GL context. Use separate FBOs per viewport to avoid state clashes.
- **ImGui Docking Branch**: You must use the `docking` branch (`https://github.com/ocornut/imgui/tree/docking`), not the master, to get the same multi‑panel workspace feel as Qt.
- **Font & Style**: Qt’s Fusion style can be roughly matched by loading a custom ImGui theme (e.g., Material‑dark). ImGui also supports icon fonts.
- **Threading**: Qt’s `QThread` usage (if any) should be replaced with `std::thread` or kept as is. The main loop remains single‑threaded for UI.
- **File Dialogs**: For native open/save dialogs, include `tinyfiledialogs` (one header, permissive) – a trivial addition.

### 7. Conclusion

Replacing Qt with Dear ImGui is a **focused, 5‑week effort** that maintains all the scientific power of QuantumVerse while simplifying the dependency tree and aligning with the project’s permissive licensing. The existing modular architecture protects the physics and rendering core from changes; the UI layer is confined to `src/ui4d/`, which becomes `src/ui_imgui/` after migration. The result will be a lighter, more responsive, and developer‑friendly scientific instrument.