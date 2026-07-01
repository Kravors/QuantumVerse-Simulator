# GUI Upgrade Implementation Plan

## Overview

This document provides a detailed implementation plan for the GUI upgrade described in `GUI_Upgrade_Plan.md`, mapping the vision to the existing codebase and identifying gaps.

---

## Current State Analysis

### Existing Components (Ready to Extend)

| Component | Location | Status |
|-----------|----------|--------|
| GLFW + ImGui setup | `src/main_glfw.cpp` | ✅ Complete with docking enabled |
| 4D Camera system | `UI4D_ImGui.h` (Camera4D) | ✅ Basic implementation exists |
| Slice View panels | `UI4D_ImGui.h` (SliceViewPanel) | ✅ 4 linked slice views implemented |
| Planck Microscope | `UI4D_ImGui.h` (PlanckMicroscopePanel) | ✅ Scale control with LOD levels |
| Discovery Panel | `UI4D_ImGui.h` (DiscoveryPanel) | ✅ Curvature scalars display |
| Theory Manager | `UI4D_ImGui.h` (TheoryManagerPanel) | ✅ Theory selection UI |
| Curvature Renderer | `src/rendering/CurvatureRenderer.h` | ✅ Grid deformation rendering |
| Quantum Geometry Renderer | `src/rendering/QuantumGeometryRenderer.h` | ✅ CDT, Spin Foam, GFT, Causal Set support |

### Missing Components (Need Implementation)

| Component | Priority | Description |
|-----------|----------|-------------|
| Timeline Bar | High | Bottom bar with time travel controls |
| Context Menu System | High | Right-click → physics tools |
| Object Browser | High | Celestial body navigation sidebar |
| Property Editor | High | Object property editing panel |
| Floating Panels | Medium | Curvature overlay, Geodesic tracer, Anomaly alerts |
| VR Integration | Low | OpenXR support |
| Collaboration | Low | Multi-user time-locking |

---

## Phase 0: GUI-First Modifications

### Task 0.8: Main Viewport with 4D Camera Controls

**Current State:** Basic 4D camera exists with WASD+QE translation, mouse rotation, scroll zoom.

**Enhancements Needed:**
- [ ] Add smooth camera interpolation for "fly to" actions
- [ ] Implement logarithmic zoom (1 AU → 10⁻³⁵ m)
- [ ] Add time-scrubbing (Ctrl + drag)
- [ ] Visual feedback for 4D rotation planes

**Files to Modify:**
- `src/ui_imgui/UI4D_ImGui.cpp` - `handleMouse()`, `handleKeyboard()`
- `src/ui4d/UI4D.h` - Camera4D class

### Task 0.9: Bottom Timeline Bar

**New Component Required:**

```cpp
// TimelineBar.h - New file
class TimelineBar {
public:
    void render(ImVec2 availSize);
    
    // Time control callbacks
    std::function<void()> onRewind;
    std::function<void()> onPreviousEvent;
    std::function<void()> onPlayPause;
    std::function<void()> onNextEvent;
    std::function<void()> onFastForward;
    std::function<void(double)> onTimeScrub;
    std::function<void(double)> onSpeedChange;
    
private:
    double currentTime = 0.0;
    double minTime = 0.0;
    double maxTime = 100.0;
    double speed = 1.0;
    bool playing = false;
    std::vector<std::pair<double, std::string>> events; // time, description
};
```

**Integration Points:**
- Add to `UI4D_ImGui` as member
- Connect to `QuantumVerseApp::state.simulationTime`

### Task 0.10: Context Menu System

**New Component Required:**

```cpp
// ContextMenu.h - New file
class ContextMenu {
public:
    void openAt(const Event4D& position, const std::string& objectName);
    void render();
    
    // Physics tool callbacks
    std::function<void()> onDropTestParticle;
    std::function<void()> onMeasurePrecession;
    std::function<void()> onComputeRedshift;
    std::function<void()> onDetectFrameDragging;
    std::function<void()> onTestQuantumGravity;
    std::function<void()> onRunDiscoveryScan;
    std::function<void()> onFlyToObject;
    
private:
    bool isOpen = false;
    Event4D menuPosition;
    std::string objectName;
};
```

**Integration Points:**
- Right-click detection in `handleMouse()`
- Connect to `GeodesicIntegrator` for test particles
- Connect to `DiscoveryProbe` for anomaly detection

### Task 0.11: Object Browser Sidebar

**New Component Required:**

```cpp
// ObjectBrowser.h - New file
class ObjectBrowser {
public:
    struct CelestialBody {
        std::string name;
        std::string type; // STAR, PLANET, MOON, BLACK_HOLE, etc.
        double mass;
        double radius;
        std::array<double, 3> position;
        std::string texturePath;
    };
    
    void render(ImVec2 availSize);
    void addBody(const CelestialBody& body);
    void selectBody(const std::string& name);
    
    std::function<void(const std::string&)> onFlyToBody;
    
private:
    std::vector<CelestialBody> bodies;
    std::string selectedBody;
    std::string filterText;
};
```

**Integration Points:**
- Initialize with solar system data from `QuantumVerseApp::solarSystem`
- Connect to camera for "fly to" actions

### Task 0.12: Property Editor Panel

**New Component Required:**

```cpp
// PropertyEditor.h - New file
class PropertyEditor {
public:
    void render(ImVec2 availSize);
    void setObject(const std::string& name, const nlohmann::json& properties);
    
    std::function<void(const nlohmann::json&)> onPropertiesChanged;
    
private:
    std::string objectName;
    nlohmann::json properties;
    bool isExoticMode = false;
};
```

**Integration Points:**
- Connect to `SingularityHandler` for black hole properties
- Support exotic overrides (negative mass, charge, phantom energy)

### Task 0.13: Floating Panel System

**New Components Required:**

```cpp
// FloatingPanels.h - New file
class CurvatureOverlay {
public:
    void render();
    void setKretschmann(double v) { kretschmann = v; }
    void setRicci(double v) { ricci = v; }
    void setWeyl(double v) { weyl = v; }
    
private:
    double kretschmann = 0, ricci = 0, weyl = 0;
    float heatmapIntensity = 1.0f;
    bool showTidalVectors = false;
};

class GeodesicTracer {
public:
    void render();
    void launchParticle(const Event4D& start, const std::array<double, 4>& velocity);
    
private:
    enum ParticleType { MASSIVE, PHOTON };
    ParticleType particleType = MASSIVE;
    std::vector<Event4D> trajectory;
    double deviationFromGR = 0.0;
};

class AnomalyAlert {
public:
    void show(const std::string& message, double confidence);
    void render();
    
private:
    bool visible = false;
    std::string message;
    double confidence = 0.0;
    std::chrono::steady_clock::time_point showTime;
};
```

---

## Integration Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    UI4D_ImGui (Main)                        │
├─────────────────────────────────────────────────────────────┤
│  MenuBar  │  TimelineBar  │  ObjectBrowser  │  PropertyEditor │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │   4D View    │  │ Slice View A │  │ Slice View B │     │
│  │              │  │              │  │              │     │
│  │  [Main       │  │  [t=const]   │  │  [τ=const]   │     │
│  │   Viewport]  │  │              │  │              │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
│                                                             │
├─────────────────────────────────────────────────────────────┤
│  ContextMenu  │  CurvatureOverlay  │  GeodesicTracer  │  │
└─────────────────────────────────────────────────────────────┘
```

---

## File Structure Changes

### New Files to Create

| File | Purpose |
|------|---------|
| `src/ui_imgui/TimelineBar.h` | Timeline controls |
| `src/ui_imgui/TimelineBar.cpp` | Timeline implementation |
| `src/ui_imgui/ContextMenu.h` | Right-click context menu |
| `src/ui_imgui/ContextMenu.cpp` | Context menu implementation |
| `src/ui_imgui/ObjectBrowser.h` | Celestial body browser |
| `src/ui_imgui/ObjectBrowser.cpp` | Object browser implementation |
| `src/ui_imgui/PropertyEditor.h` | Object property editor |
| `src/ui_imgui/PropertyEditor.cpp` | Property editor implementation |
| `src/ui_imgui/FloatingPanels.h` | Overlay panels |
| `src/ui_imgui/FloatingPanels.cpp` | Floating panel implementation |

### Files to Modify

| File | Changes |
|------|---------|
| `src/ui_imgui/UI4D_ImGui.h` | Add new panel members, integrate components |
| `src/ui_imgui/UI4D_ImGui.cpp` | Add render calls, input handling |
| `src/main_glfw.cpp` | No changes needed (docking already enabled) |

---

## Implementation Order

1. **Week 1:** Timeline Bar + Context Menu
2. **Week 2:** Object Browser + Property Editor
3. **Week 3:** Floating Panels (Curvature, Geodesic Tracer)
4. **Week 4:** Integration testing + Polish

---

## Success Metrics Mapping

| Metric | Implementation Target |
|--------|----------------------|
| Time to fly to any celestial body < 3s | ObjectBrowser + smooth camera interpolation |
| Context menu response < 100ms | Immediate popup on right-click |
| Zoom smoothness 60 FPS | Logarithmic zoom with LOD transitions |
| Discovery alert visibility | AnomalyAlert popup with sound |
| New user onboarding < 2 min | Intuitive UI with tooltips |

---

## Next Steps

1. Review this plan with the team
2. Confirm file structure and component boundaries
3. Begin implementation with Timeline Bar (Task 0.9)