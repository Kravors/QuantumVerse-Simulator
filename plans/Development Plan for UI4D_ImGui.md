## Development Plan: Spacetime Flight & Advanced GUI for UI4D_ImGui

This plan extends the existing **UI4D_ImGui** system to enable **free flight through the 4D spacetime fabric** with **real-time physics feedback** and a **highly interactive, powerful UI**. The goal is to provide an immersive exploration experience where the user “flies” through curved spacetime while visualising the fabric, curvature scalars, causal structure, and quantum gravity effects in real time.

---

### Phase 0 – Prerequisites & Architecture Review

| Task | Description |
|------|-------------|
| 0.1 | Audit existing classes: `UI4D_ImGui`, `CurvatureRenderer`, `CausalGraph`, `DiscoveryProbe`, `PlanckMicroscopePanel`, `SliceViewPanel`. |
| 0.2 | Verify real‑time performance baseline (frame rate, curvature evaluation cost). |
| 0.3 | Identify integration points for flight controller and fabric renderer. |
| 0.4 | Set up profiling hooks to measure new features without degrading existing UI. |

**Deliverable:** Updated class diagram showing new components (see below) and performance baseline report.

---

### Phase 1 – Core Spacetime Flight Mechanics

Enable smooth, continuous navigation through 4D spacetime with user‑controllable velocity and geodesic assistance.

| Task | Description |
|------|-------------|
| 1.1 | **Create `SpacetimeFlightController` class**<br>- Stores 4D position (t, x, y, z) and 4‑velocity.<br>- Handles input mapping (WASD + Q/E + mouse) to accelerate/decelerate.<br>- Integrates equations of motion: `dxᵢ/dλ = uᵢ`, `duᵢ/dλ = -Γᵢⱼₖ uʲ uᵏ` (geodesic equation).<br>- Uses a Runge‑Kutta 4 integrator with adaptive step size. |
| 1.2 | **Add flight mode toggle** in UI (checkbox / key ‘F’).<br>When active, camera follows the `SpacetimeFlightController` position; otherwise classic orbital controls remain. |
| 1.3 | **Implement geodesic deviation feedback** – display nearby tidal forces (computed from Riemann tensor) as a small widget. |
| 1.4 | **Add “warp” assist** – a slider that multiplies proper velocity, allowing faster‑than‑light navigation without violating causality (purely for visual exploration). |

**New Files:**  
- `src/ui_imgui/SpacetimeFlightController.h/cpp`  
- `src/ui_imgui/FlightTelemetry.h/cpp` (for data logging)

**Modified Classes:** `UI4D_ImGui`, `SliceViewPanel` (to update slice origin based on flight position).

---

### Phase 2 – Real‑Time Spacetime Fabric Visualization

Replace / enhance the existing slice grid with a dynamic 3D mesh that warps according to the local curvature, resembling a “fabric”.

| Task | Description |
|------|-------------|
| 2.1 | **Design `FabricMeshRenderer` class**<br>- Generates a tessellated grid in a local 3D spatial slice (constant `t` or proper time).<br>- For each vertex, compute the displacement caused by the Weyl or Ricci curvature (e.g., vertical displacement in an extra dimension or colour‑coded displacement).<br>- Uses instanced rendering with LOD (denser mesh near the camera). |
| 2.2 | **Add fabric shading** – colour vertices by Kretschmann scalar or by the local “stretch” factor. Provide a legend in the UI. |
| 2.3 | **Integrate with `PlanckMicroscopePanel`** – as the user zooms into Planck scales, the fabric mesh automatically refines and shows quantum foam (noise / discrete geometry hints). |
| 2.4 | **Add “flow lines”** – render particle trajectories (geodesics) that update in real time, showing how test particles would fall through the current fabric. |
| 2.5 | **Allow slicing the fabric** – a 3D plane widget that clips the mesh, exposing internal curvature structures (reuses existing `SliceViewPanel` logic). |

**New Files:**  
- `src/rendering/FabricMeshRenderer.h/cpp`  
- `src/rendering/FlowLineRenderer.h/cpp`

**Modified Classes:** `CurvatureRenderer` (to supply curvature data on GPU), `PlanckMicroscopePanel` (to send LOD requests).

---

### Phase 3 – Real‑Time Physics Feedback Panels

Extend existing panels to update continuously during flight, giving the user instant insight into the local spacetime properties.

| Task | Description |
|------|-------------|
| 3.1 | **Enhance `DiscoveryPanel`**<br>- Add a real‑time graph of Kretschmann, Ricci, and Weyl scalars along the flight path (x‑axis = proper time).<br>- Highlight anomalies (e.g., curvature spikes) with visual markers and optional audio cue. |
| 3.2 | **Upgrade `CausalGraphPanel`**<br>- Show only events in a causal diamond around the current flight position.<br>- Animate the graph as the user moves – new events appear, old ones fade. |
| 3.3 | **Improve `PlanckMicroscopePanel`**<br>- Link to the fabric renderer: the current LOD level determines the fabric’s detail.<br>- Add a “quantum foam intensity” slider that injects procedural noise into the fabric mesh when near Planck scale. |
| 3.4 | **Create `FlightTelemetryPanel`** (new)<br>- Displays: 4D position, 4‑velocity magnitude, local proper acceleration, tidal force eigenvalues.<br>- Option to record flight path for later playback or analysis. |
| 3.5 | **Add a minimap** – a 2D projection of the fabric with a dot showing the user’s position (opens a new `MinimapPanel`). |

**New Files:**  
- `src/ui_imgui/FlightTelemetryPanel.h/cpp`  
- `src/ui_imgui/MinimapPanel.h/cpp`

**Modified Classes:** `DiscoveryPanel`, `CausalGraphPanel`, `PlanckMicroscopePanel`.

---

### Phase 4 – Advanced GUI Enhancements

Make the interface powerful, customisable, and professional.

| Task | Description |
|------|-------------|
| 4.1 | **Implement dockable floating panels** – reuse ImGui docking. Add a “Window” menu to show/hide any panel. |
| 4.2 | **Create a configurable HUD** – user can drag any telemetry value (e.g., curvature scalar) into a compact overlay. Save/load HUD layouts via `LayoutPresets`. |
| 4.3 | **Add flight recorder / time machine** – record the flight path (positions and time) and allow scrubbing on the `TimelineBar`. This integrates the existing timeline with flight data. |
| 4.4 | **Enhance `ObjectBrowser`** – show not only celestial bodies but also “curvature features” (wormhole candidates, high‑curvature regions). Right‑click to “fly to” that feature. |
| 4.5 | **Add a tool palette** – floating bar with quick actions: toggle fabric wireframe, reset flight, capture screenshot/start video, show geodesic heatmap. |
| 4.6 | **Integrate `AccessibilityManager`** – add voice announcements for curvature anomalies and flight events (e.g., “entering high curvature region”). |
| 4.7 | **Improve `PropertyEditor`** – allow editing of metric parameters (e.g., black hole mass, rotation) on the fly while flying. The fabric updates instantly. |

**Modified Classes:** `UI4D_ImGui`, `LayoutPresets`, `ObjectBrowser`, `PropertyEditor`, `TimelineBar`, `AccessibilityManager`.

---

### Phase 5 – Performance & Profiling

Ensure real‑time flight remains smooth even on complex metrics and high‑resolution fabric.

| Task | Description |
|------|-------------|
| 5.1 | **GPU‑accelerated curvature evaluation** – move Riemann tensor computation to compute shaders (GLSL). The fabric vertex shader directly reads curvature texture. |
| 5.2 | **Adaptive fabric LOD** – the mesh resolution decreases with distance from camera and increases near the user. Use tessellation shaders or dynamic index buffer. |
| 5.3 | **Cull invisible fabric parts** using a hierarchical view frustum (including time direction). |
| 5.4 | **Add `PerformanceProfiler` panel** – shows GPU/CPU time per subsystem (fabric render, curvature eval, UI). Highlight bottlenecks. |
| 5.5 | **Optimise geodesic integration** – use asynchronous compute for multiple test particles (flow lines). |

**Modified Classes:** `PerformanceProfiler`, `CurvatureRenderer`, `FabricMeshRenderer`.  
**New Shaders:** `fabric_vert.glsl`, `fabric_tesc.glsl`, `fabric_tese.glsl`, `curvature_eval.comp`.

---

### Phase 6 – Integration, Testing & Polish

| Task | Description |
|------|-------------|
| 6.1 | **Integrate with `VRIntegration`** – map flight controls to VR hand controllers, add head‑tracked view. Test comfort. |
| 6.2 | **Update `OnboardingPanel`** – add a guided tutorial on how to fly, interpret the fabric, and use the new telemetry panels. |
| 6.3 | **Stress test** with different metrics (Schwarzschild, Kerr, FLRW, CDT discrete geometries) and validate real‑time updates. |
| 6.4 | **Add unit tests** for flight integrator (geodesic accuracy against known orbits). |
| 6.5 | **Write user documentation** – extend the existing documentation with flight controls, fabric visualisation, and advanced UI tips. |
| 6.6 | **Final code review & merge** – ensure all new classes follow the project’s naming and style conventions. |

---

## Timeline Estimate

| Phase | Effort (person‑days) | Dependencies |
|-------|----------------------|---------------|
| Phase 0 | 1 | – |
| Phase 1 | 5 | Phase 0 |
| Phase 2 | 8 | Phase 0, 1 (flight position needed) |
| Phase 3 | 6 | Phase 1, 2 (real‑time data) |
| Phase 4 | 6 | Phase 3 (telemetry available) |
| Phase 5 | 5 | Phase 2, 3 (to profile) |
| Phase 6 | 3 | All previous |

**Total ~34 person‑days** (approx. 7 weeks for one developer).

---

## Example Code Snippet – Flight Controller Integration

```cpp
// Inside UI4D_ImGui::processInput()
if (flightActive) {
    flightController->update(deltaTime, metricTensor);
    camera4D->setPosition(flightController->getPosition());
    camera4D->setOrientation(flightController->getOrientation());
    // Real‑time telemetry update
    flightTelemetryPanel->update(flightController->getLocalCurvature());
} else {
    // classic camera controls
}
```

---

## New UI Mockup (Concept)

```
┌──────────────┬────────────────────────────────────────────┬─────────────┐
│ Objects      │ 4D Flight View (fabric mesh)               │ Telemetry   │
│   ☉ Sun      │   [wireframe toggle] [reset flight]        │   pos (t,x,y,│
│   ⬤ Earth    │   ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   │   z)        │
│   ⬤ Moon     │   ░░░░░ fabric grid ░░░░░░░░░░░░░░░░░░░░░   │   v⁴ = 0.95 │
│ Properties   │   ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   │   Kretch    │
│   Mass: 1M☉  │   [user’s ship marker]                      │   2.3e-3    │
│   Spin: 0.9  │                                             │   Tidal     │
│              │                                             │   λ₁=1.2e-5 │
├──────────────┼────────────────────────────────────────────┼─────────────┤
│ Causal Graph │ Planck Microscope   │ Discovery (graph)    │ Minimap     │
│   (events)   │   [log zoom: -35 █─] │   K ↑                │   [map]     │
│   ···        │                      │     ··              │             │
└──────────────┴──────────────────────┴─────────────────────┴─────────────┘
```

This plan transforms UI4D_ImGui into a **state‑of‑the‑art 4D spacetime flight simulator** with a responsive, data‑rich interface. Each step is modular, allowing incremental delivery and testing.