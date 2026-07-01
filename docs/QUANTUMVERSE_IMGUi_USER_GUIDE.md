# QuantumVerse ImGui Application - User Guide

## Overview

`quantumverse_imgui.exe` is the main 4D spacetime visualization application for the QuantumVerse Simulator. It provides an interactive interface for exploring general relativity, quantum gravity theories, and spacetime curvature through a multi-panel ImGui-based UI.

## Building the Application

```bash
cd f:\syyyy
mkdir build_imgui
cd build_imgui
cmake .. -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_USE_IMGUI=ON -DQUANTUMVERSE_ENABLE_QUANTUM_GRAVITY=ON -DQUANTUMVERSE_BUILD_TESTS=ON
cmake --build . --config Release --parallel 4
```

## Running the Application

```bash
cd f:\syyyy\deploy\windows
.\quantumverse_imgui.exe
```

## UI Layout

The application features a multi-panel interface with the following components:

### Top Menu Bar
- **File**: New scene, Open, Save, Save As, Exit
- **View**: Toggle visibility of all panels
- **Edit**: Undo/Redo functionality
- **Tools**: Discovery probe, Anomaly detection, Light cone controls

### Main Panels (Docked)

| Panel | Description |
|-------|-------------|
| **4D View** | Main spacetime visualization with curvature rendering |
| **Slice Views** | 2D slices of 4D spacetime (XY, XZ, YW, ZW planes) |
| **Planck Microscope** | Zoom from cosmic to Planck scale (35 orders of magnitude) |
| **Discovery** | Anomaly detection and curvature analysis |
| **Causal Graph** | Event-based causal structure visualization |
| **Theories** | Quantum gravity theory selection (GR, CDT, Spin Foam, GFT, Causal Set, Dilaton) |
| **Objects** | Scene object browser (solar system, black holes, etc.) |
| **Properties** | Selected object properties editor |
| **Flight Telemetry** | Camera position and navigation data |
| **Minimap** | Overview minimap of the current view |
| **Timeline** | Time navigation and simulation control |

## Controls

### Keyboard Navigation
| Key | Action |
|-----|--------|
| W | Move camera forward (Y axis) |
| S | Move camera backward (Y axis) |
| A | Move camera left (X axis) |
| D | Move camera right (X axis) |
| Q | Move camera down (W axis) |
| E | Move camera up (W axis) |
| Up/Down | Move in time (T axis) |
| Left/Right | Move in Z axis |
| C | Toggle curvature overlay |
| F1 | Start onboarding tutorial |
| Ctrl+Z | Undo |
| Ctrl+Y | Redo |
| Ctrl+N | New scene |
| Ctrl+O | Open scene |
| Ctrl+S | Save scene |

### Mouse Controls
| Action | Description |
|--------|-------------|
| Left Drag | Rotate camera (XY plane) |
| Shift + Left Drag | Rotate camera (XZ plane) |
| Ctrl + Left Drag | Rotate camera (ZW plane) |
| Mouse Wheel | Zoom in/out |
| Right Click | Open context menu for selected object |

## Features

### 1. Spacetime Visualization
- Real-time curvature rendering using the `CurvatureRenderer`
- Multiple curvature modes: Grid Deformation, Riemann Color, Curvature Scalar, Geodesic Flow, Time Dilation
- Light cone visualization for causal structure

### 2. Solar System Simulation
- Built-in solar system with 8 planets, Sun, and Moon
- Celestial body rendering with procedural textures
- Orbital mechanics visualization

### 3. Black Hole Physics
- Schwarzschild and Kerr metric visualization
- Event horizon and ergosphere rendering
- Accretion disk and relativistic effects

### 4. Quantum Gravity Theories
- **GR**: General Relativity (classical spacetime)
- **CDT**: Causal Dynamical Triangulations
- **Spin Foam**: Loop Quantum Gravity spin networks
- **GFT**: Group Field Theory
- **Causal Set**: Discrete causal structure
- **Dilaton**: 2D dilaton gravity with Hawking radiation

### 5. Discovery Instruments
- 10+ automated discovery tools for anomaly detection
- CurvatureNormalizingFlow for statistical anomaly detection
- Symbolic regression for finding physical laws

### 6. Planck Microscope
- Zoom from 10^0 m to 10^-35 m scale
- 4 LOD levels: Classical, Quantum corrections, Quantum foam, Planck foam
- Real-time scale indicator

## Debug Windows

When running, the application displays:
- **ImGui Demo Window**: Shows all ImGui widgets and features
- **Metrics Window**: Displays window states, draw list statistics
- **Debug Overlay**: Shows DisplaySize, MousePos, WantCaptureMouse, panel visibility

## Troubleshooting

### Black Screen / No UI
If the UI is not visible:
1. Check that `renderPanels()` is called inside the dockspace window
2. Verify `SetNextWindowCollapsed(false, ImGuiCond_Always)` is set on all panels
3. Check the Debug Overlay for DisplaySize values (should be > 0)

### No Input Response
If mouse/keyboard input doesn't work:
1. Verify `ImGui_ImplGlfw_*Callback` functions are called in GLFW callbacks
2. Check that callbacks are set up via `setupInputCallbacks()`
3. Ensure the ImGui context is properly initialized

### Performance Issues
- Use `--super-safe` mode for reduced quality but better performance
- Disable quantum foam effect for large scenes
- Reduce framebuffer resolution in settings

## File Structure

```
src/ui_imgui/
├── UI4D_ImGui.cpp/h       # Main UI implementation
├── TimelineBar.cpp/h      # Timeline control
├── ContextMenu.cpp/h      # Right-click context menu
├── ObjectBrowser.cpp/h    # Scene object browser
├── PropertyEditor.cpp/h   # Object property editor
├── ToolPalette.cpp/h      # Top tool palette
├── FloatingPanels.cpp/h   # Overlay panels
├── OnboardingPanel.cpp/h  # Tutorial system
├── AccessibilityManager.cpp/h
├── TheorySelectorPanel.cpp/h
├── ScreenshotCapture.cpp/h
├── LayoutPresets.cpp/h
├── PerformanceProfiler.cpp/h
├── UndoRedoSystem.cpp/h
├── SpacetimeFlightController.cpp/h
├── FabricMeshRenderer.cpp/h
├── FlightTelemetryPanel.cpp/h
└── MinimapPanel.cpp/h
```

## Dependencies

- **GLFW 3.4**: Window and input management
- **OpenGL 3.3+**: Rendering backend
- **Dear ImGui 1.91+**: UI framework
- **Qt6**: Optional QML backend

## Build Configuration Flags

| Flag | Default | Description |
|------|---------|-------------|
| `QUANTUMVERSE_USE_IMGUI` | ON | Enable ImGui backend |
| `QUANTUMVERSE_ENABLE_QUANTUM_GRAVITY` | ON | Enable quantum gravity engines |
| `QUANTUMVERSE_BUILD_TESTS` | ON | Build test suite |
| `QUANTUMVERSE_IMGUI_DX11` | OFF | Use DirectX 11 for ImGui textures |

## License

MIT License - See LICENSE file for details.