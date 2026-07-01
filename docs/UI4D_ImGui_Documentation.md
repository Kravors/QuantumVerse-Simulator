# UI4D_ImGui - Dear ImGui-based 4D User Interface

## Overview

`UI4D_ImGui` is the main user interface system for QuantumVerse Simulator, providing a complete Dear ImGui-based replacement for the original Qt UI. It implements a multi-panel docking interface for 4D spacetime visualization and physics interaction.

## File Location
- **Header**: [`src/ui_imgui/UI4D_ImGui.h`](src/ui_imgui/UI4D_ImGui.h)
- **Implementation**: [`src/ui_imgui/UI4D_ImGui.cpp`](src/ui_imgui/UI4D_ImGui.cpp)

## Architecture

### Class Hierarchy

```
UI4D_ImGui
├── Configuration
│   └── UI4DConfig - UI settings and preferences
├── Rendering Components
│   ├── Framebuffer - OpenGL framebuffer for viewports
│   ├── SliceViewPanel - 3D slice rendering
│   └── PlanckMicroscopePanel - Logarithmic zoom widget
├── Physics Panels
│   ├── DiscoveryPanel - Curvature and anomaly data
│   ├── CausalGraphPanel - Event graph visualization
│   └── TheoryManagerPanel - Quantum gravity theory selection
├── UI Components
│   ├── TimelineBar - Time navigation
│   ├── ContextMenu - Right-click physics tools
│   ├── ObjectBrowser - Scene object listing
│   ├── PropertyEditor - Object property editing
│   └── FloatingPanels - Anomaly alert overlays
└── Integration
    ├── OnboardingPanel - Tutorial system
    ├── AccessibilityManager - Screen reader support
    ├── VRIntegration - VR headset support
    ├── UIStyleManager - Theme management
    ├── LayoutPresets - Dock layout save/load
    ├── PerformanceProfiler - Performance overlay
    └── UndoRedoSystem - Command history
```

## Key Components

### UI4DConfig

Configuration structure for UI settings:

```cpp
struct UI4DConfig {
    bool showLightCones = true;
    bool showCausalStructure = true;
    bool showCurvatureGrid = true;
    bool showWorldLines = true;
    bool showTimeGeography = true;
    bool showTopologyIndicators = true;
    
    float curvatureScale = 1.0f;
    float lightConeTransparency = 0.3f;
    float worldLineThickness = 2.0f;
    
    int numSlices = 4;
    float sliceSpacing = 10.0f;
};
```

### Framebuffer

OpenGL framebuffer for off-screen rendering:

```cpp
class Framebuffer {
public:
    Framebuffer(int width, int height);
    ~Framebuffer();
    
    void bind();
    void unbind();
    void resize(int width, int height);
    
    GLuint getTexture() const;
    int getWidth() const;
    int getHeight() const;
};
```

### SliceViewPanel

Renders 3D slices of 4D spacetime with interactive controls:

- **Slice Parameter**: Double slider for 4D coordinate selection
- **Slicing Modes**: Fixed T, Proper Time, Null Slice, Complex Real, Complex Imag
- **Curvature Rendering**: Integrates with `CurvatureRenderer`

### PlanckMicroscopePanel

Logarithmic zoom widget spanning 35 orders of magnitude:

- **Scale Range**: 10^-35 m (Planck length) to 10^0 m (human scale)
- **LOD Levels**:
  - Planck foam (quantum gravity regime)
  - Quantum foam (discrete geometry)
  - Quantum corrections visible
  - Classical continuum

### DiscoveryPanel

Displays curvature scalars and anomaly detection results:

- **Kretschmann Scalar**: R_{abcd} R^{abcd}
- **Ricci Scalar**: R
- **Weyl Scalar**: Conformal curvature
- **Anomaly Table**: Name, Value, Expected, Deviation columns

### CausalGraphPanel

Event-based causal structure visualization:

```cpp
struct Node {
    std::string name;
    std::array<double, 4> coords;
    std::vector<int> causalFuture;
    std::vector<int> causalPast;
    bool selected = false;
};
```

### TheoryManagerPanel

Quantum gravity theory selection:

- **GR**: General Relativity - Classical spacetime
- **CDT**: Causal Dynamical Triangulations
- **Spin Foam**: Loop Quantum Gravity
- **GFT**: Group Field Theory
- **Causal Set**: Discrete causal structure
- **Dilaton**: 2D Dilaton gravity

## UI4D_ImGui Class

### Constructor

```cpp
UI4D_ImGui(
    std::shared_ptr<MetricTensor> metric,
    std::shared_ptr<CurvatureRenderer> curvatureRenderer,
    std::shared_ptr<CausalGraph> causalGraph = nullptr,
    std::shared_ptr<DiscoveryProbe> probe = nullptr,
    std::weak_ptr<UI4D> ui4d = std::weak_ptr<UI4D>()
);
```

### Main Methods

| Method | Description |
|--------|-------------|
| `render()` | Main UI rendering loop |
| `processInput()` | GLFW input handling |
| `getConfig()` | Access UI configuration |
| `setShow*()` | Panel visibility control |
| `translateCamera()` | 4D camera translation |
| `rotateCamera()` | 4D camera rotation |
| `zoomCamera()` | 4D camera zoom |
| `setActiveTheory()` | Set active quantum gravity theory |
| `setupInputCallbacks()` | GLFW callback registration |

### Panel Accessors

```cpp
SliceViewPanel& getSliceViewPanel();
PlanckMicroscopePanel& getPlanckMicroscopePanel();
DiscoveryPanel& getDiscoveryPanel();
CausalGraphPanel& getCausalGraphPanel();
TheoryManagerPanel& getTheoryManagerPanel();
TheorySelectorPanel& getTheorySelectorPanel();
TimelineBar& getTimelineBar();
OnboardingPanel& getOnboardingPanel();
AccessibilityManager& getAccessibilityManager();
```

## Dock Layout

The interface uses ImGui's docking system with the following layout:

```
┌──────────────┬──────────────────────────────────────┐
│ Objects      │ 4D View           │ Discovery       │
│              │                   ├─────────────────┤
│ Properties   │ Slice Views       │ Theories        │
│              │                   │                 │
└──────────────┴─────────────────────┴─────────────────┤
│ Timeline                                           │
└────────────────────────────────────────────────────┘
```

## Input Handling

### Keyboard Controls

| Key | Action |
|-----|--------|
| W/A/S/D | 4D camera translation |
| Q/E | Time translation |
| Arrow keys | Alternative translation |
| C | Toggle curvature overlay |
| Ctrl+Z | Undo |
| Ctrl+Y | Redo |
| F1 | Start onboarding tutorial |

### Mouse Controls

| Action | Function |
|--------|----------|
| Left drag | Camera rotation |
| Right click | Context menu |
| Mouse wheel | Zoom |
| Left click | Object picking |

## Context Menu Actions

Right-click context menu provides physics tools:

- **Test General Relativity**: Run GR validation tests
- **Drop Test Particle**: Create test particle at object position
- **Measure Precession**: Calculate orbital precession
- **Compute Redshift**: Calculate gravitational redshift
- **Detect Frame-Dragging**: Calculate Lense-Thirring effect
- **Run Discovery Scan**: Automated anomaly detection
- **Fly To Object**: Focus camera on selected object
- **Add Waypoint**: Add navigation waypoint

## Integration Points

### Scene Graph

```cpp
void setSceneGraphManager(std::shared_ptr<SceneGraphManager> manager);
std::shared_ptr<SceneGraphManager> getSceneGraphManager() const;
```

**Note**: The scene graph is initialized when `setSceneGraphManager()` is called, which creates the solar system objects. This ensures proper initialization order.

### Celestial Body Renderer

```cpp
void setCelestialBodyRenderer(std::shared_ptr<CelestialBodyRenderer> renderer);
```

### Screenshot/Video

```cpp
bool captureScreenshot(const std::string& filepath);
bool startVideoRecording(const std::string& filepath, int fps = 30);
void stopVideoRecording();
void addVideoFrame();
```

## Dependencies

- **GLFW**: Window management
- **GLAD**: OpenGL loading
- **Dear ImGui**: UI framework
- **QuantumVerse modules**: spacetime, physics, rendering, ui4d, utils

## Build Configuration

Enabled via CMake option:

```cmake
option(QUANTUMVERSE_USE_IMGUI "Build the 4D ImGui GUI" ON)
```

## Usage Example

```cpp
// Create UI instance
auto metric = std::make_shared<quantumverse::MetricTensor>();
auto curvatureRenderer = std::make_shared<quantumverse::CurvatureRenderer>(30, 100.0f);
quantumverse::ui_imgui::UI4D_ImGui ui4d(metric, curvatureRenderer);

// Set up scene
auto sceneGraph = std::make_shared<quantumverse::SceneGraphManager>(metric);
sceneGraph->createSolarSystem();
ui4d.setSceneGraphManager(sceneGraph);

// Main loop
while (!glfwWindowShouldClose(window)) {
    ui4d.processInput();
    ImGui::NewFrame();
    ui4d.render();
    ImGui::Render();
    // ... render ImGui draw data
}
```

## See Also

- [`src/ui_imgui/ContextMenu.h`](src/ui_imgui/ContextMenu.h) - Context menu implementation
- [`src/ui_imgui/FloatingPanels.h`](src/ui_imgui/FloatingPanels.h) - Anomaly alerts
- [`src/ui_imgui/OnboardingPanel.h`](src/ui_imgui/OnboardingPanel.h) - Tutorial system
- [`src/rendering/CurvatureRenderer.h`](src/rendering/CurvatureRenderer.h) - Curvature visualization