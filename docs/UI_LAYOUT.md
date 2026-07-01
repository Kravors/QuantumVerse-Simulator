# QuantumVerse Simulator - Main UI Layout Documentation

## Overview

The QuantumVerse Simulator uses Dear ImGui with docking for its main user interface. The UI is implemented in `src/ui_imgui/UI4D_ImGui.cpp` and provides a multi-panel layout for 4D spacetime visualization.

## UI Architecture

### Main Entry Point
- **File**: `src/main_glfw.cpp`
- **Framework**: GLFW + OpenGL 4.5 Core Profile
- **UI Library**: Dear ImGui (docking branch)

### UI Initialization

```cpp
// In initImGui() - src/main_glfw.cpp:137-155
void initImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // Enable docking
    io.IniFilename = nullptr;  // Disable .ini file saving (prevents freezes)
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450");
}
```

## Dock Layout Structure

The main dock layout is created in `UI4D_ImGui::render()` (lines 770-846):

```
+----------------------------------------------------------+
| Menu Bar (File, View, Edit, Tools)                       |
+--------+------------------------------------------------+
|        |                                                |
| Objects| 4D View             | Slice Views               |
| (22%)  |                     | +------------------------+ |
|        |                     | | Planck Microscope      | |
|        |                     | +------------------------+ |
|        |                     | | Causal Graph           | |
|        |                     | +------------------------+ |
+--------+------------------------------------------------+
| Properties | Discovery | Theories | Timeline (20% height)  |
+-----------+------------+----------+-----------------------+
```

### Dock Node IDs (stored in UI4D_ImGui)

| Variable | Panel | Default Visibility |
|----------|-------|-------------------|
| `dock_4d_view` | 4D View | true |
| `dock_slice_views` | Slice Views | true |
| `dock_planck_microscope` | Planck Microscope | true |
| `dock_causal_graph` | Causal Graph | false |
| `dock_timeline` | Timeline | true |
| `dock_objects` | Objects | true |
| `dock_properties` | Properties | true |
| `dock_discovery` | Discovery | true |
| `dock_theories` | Theories | true |
| `dock_flight_telemetry` | Flight Telemetry | false |
| `dock_minimap` | Minimap | false |

## Panel Components

### 1. Menu Bar
**Location**: `UI4D_ImGui::renderMenuBar()` (lines 1528-1682)

Menus:
- **File**: New, Open, Save, Save As, Exit
- **View**: Toggle all panels, Capture (Screenshot/Recording), Layout Presets
- **Edit**: Undo, Redo
- **Tools**: Discovery Probe, Anomaly Detection, Light Cone controls

### 2. Tool Palette
**Location**: `src/ui_imgui/ToolPalette.cpp`
**Size**: Full width, 40px height
**Position**: Top of main view area

### 3. 4D View Panel
**Location**: `UI4D_ImGui::render4DView()` (lines 1196-1317)
**Content**:
- Framebuffer rendering of curvature grid
- Celestial body rendering (Sun, planets, black holes)
- Light cone visualization
- Camera position display
- Curvature mode selector

### 4. Slice Views Panel
**Location**: `UI4D_ImGui::renderSliceViews()` (lines 1323-1355)
**Content**:
- XY Slice, XZ Slice, YW Slice, ZW Slice (2x2 grid)
- Each slice shows 3D cross-section of 4D spacetime

### 5. Planck Microscope Panel
**Location**: `PlanckMicroscopePanel::render()` (lines 227-264)
**Content**:
- Scale indicator (10^exponent m)
- Zoom slider
- Quick buttons: 1m, Planck, Earth
- LOD level description

### 6. Discovery Panel
**Location**: `DiscoveryPanel::render()` (lines 284-333)
**Content**:
- Selected event display
- Curvature scalars (Kretschmann, Ricci, Weyl)
- Anomaly detection button
- Anomaly score display with color coding
- Anomaly table (name, value, expected, deviation)

### 7. Causal Graph Panel
**Location**: `CausalGraphPanel::render()` (lines 342-364)
**Content**:
- Event tree view
- Zoom and pan controls

### 8. Theory Manager Panel
**Location**: `TheoryManagerPanel::render()` (lines 380-399)
**Content**:
- Quantum gravity theory selection (GR, CDT, Spin Foam, GFT, Causal Set, Dilaton)
- Theory descriptions

### 9. Object Browser Panel
**Location**: `ObjectBrowser::render()` (src/ui_imgui/ObjectBrowser.cpp)
**Content**:
- Scene object list
- Fly-to, focus, waypoint actions

### 10. Property Editor Panel
**Location**: `PropertyEditor::render()` (src/ui_imgui/PropertyEditor.cpp)
**Content**:
- Object properties (mass, radius, position)
- Physics tools (measure gravity, test GR, compute redshift)

### 11. Timeline Bar
**Location**: `TimelineBar::render()` (src/ui_imgui/TimelineBar.cpp)
**Size**: Full width, 60px height
**Position**: Bottom of window

## Main Loop Structure

```cpp
// src/main_glfw.cpp:467-518
while (!glfwWindowShouldClose(window)) {
    perfMonitor.beginFrame();
    glfwPollEvents();
    ui4d.processInput();
    
    // ImGui frame - ALWAYS called
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    // Render UI (includes dockspace + all panels)
    if (curvatureRenderer && curvatureRenderer->isInitialized()) {
        ui4d.render();
    } else {
        // Fallback loading screen
        ImGui::Text("Initializing renderer...");
    }
    
    // Render ImGui
    ImGui::Render();
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    glfwSwapBuffers(window);
    perfMonitor.endFrame();
}
```

## Key Configuration Flags

| Flag | Location | Purpose |
|------|----------|---------|
| `ImGuiConfigFlags_DockingEnable` | main_glfw.cpp:142 | Enables dockspace |
| `ImGuiConfigFlags_ViewportsEnable` | main_glfw.cpp:144 | Multi-viewport (disabled) |
| `io.IniFilename = nullptr` | main_glfw.cpp:147 | Disables .ini file saving |

## Build Configuration

```cmake
# CMakeLists.txt:163-165
target_compile_definitions(quantumverse_imgui PRIVATE
    QUANTUMVERSE_USE_IMGUI
)
```

## Testing

Run the ImGui components test:
```bash
cd build_imgui
ctest -R ImGuiComponentsTest --output-on-failure
```

## Notes for Developers

1. **UI State Persistence**: Disabled via `io.IniFilename = nullptr`. To enable, set to `"imgui.ini"` and ensure the file is deleted on UI issues.

2. **Dock Layout Reset**: Set `firstFrame = true` to reset to default layout.

3. **Panel Visibility**: Controlled via `show*Panel` boolean variables, configurable in `user_prefs.ini`.

4. **Performance**: The `PerformanceMonitor` tracks FPS and frame time. Enable via `perf.showMetrics = true`.

5. **Headless Mode**: The app exits gracefully if `GetSystemMetrics(SM_CMONITORS) == 0`.