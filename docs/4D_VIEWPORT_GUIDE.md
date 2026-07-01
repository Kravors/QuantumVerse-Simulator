# 4D Viewport Guide - QuantumVerse Simulator

## Overview

The 4D Viewport is the central visualization component of the QuantumVerse Simulator, enabling users to explore and interact with four-dimensional spacetime. This guide covers the viewport's architecture, controls, and visualization techniques.

## File Location
- **Header**: [`src/ui_imgui/UI4D_ImGui.h`](src/ui_imgui/UI4D_ImGui.h)
- **Implementation**: [`src/ui_imgui/UI4D_ImGui.cpp`](src/ui_imgui/UI4D_ImGui.cpp)

## Core Concepts

### 4D Coordinate System
The QuantumVerse Simulator uses a Lorentzian signature spacetime with coordinates `(t, x, y, z)`:
- **t**: Time dimension (can be treated as spatial for visualization)
- **x, y, z**: Standard spatial dimensions

### Two-Slice Projection Model
The 4D → 3D → 2D projection pipeline:
1. **4D to 3D**: Select a 3D hyperplane by fixing one coordinate
2. **3D to 2D**: Project onto screen using perspective projection

## Viewport Components

### Main 4D View Panel
Located in the central dock of the ImGui interface, the viewport displays:
- **Celestial bodies** (Sun, Black Holes, Wormholes)
- **Debug grid** (green cross-hatch for pipeline validation)
- **Test particles** (geodesic worldlines)
- **Curvature visualization** (optional overlays)

### Timeline Slider
Controls the 4D time coordinate (`w` position):
- Range: Configurable (default: -100 to 100)
- Objects fade in/out as they pass through the 4D slice
- Play/pause animation for dynamic exploration

### Camera System
4D camera with position and orientation in spacetime:
- **Position**: `Event4D(t, x, y, z)` - default: `(0, 0, 5, 15)`
- **Orientation**: Rotation in 4D space
- **Projection**: Perspective projection with configurable FOV

## Controls

### Keyboard Controls

| Key | Action |
|-----|--------|
| **W/A/S/D** | Translate camera in XY plane |
| **Q/E** | Translate camera in ZW plane (time-like) |
| **Arrow Keys** | Alternative translation |
| **Mouse Drag** | Rotate camera view |
| **Mouse Wheel** | Zoom in/out |
| **C** | Toggle curvature overlay |
| **F1** | Start onboarding tutorial |

### Mouse Controls

| Action | Function |
|--------|----------|
| Left drag | Camera rotation |
| Right click | Context menu (physics tools) |
| Mouse wheel | Zoom |
| Left click | Object selection |

## Visualization Modes

### Slice Views
Multiple slicing modes for 4D exploration:
- **Fixed T**: Constant time slice
- **Proper Time**: Worldline-centered slice
- **Null Slice**: Light-cone aligned slice
- **Complex Real/Imag**: Complex coordinate visualization

### Object Rendering

#### Celestial Bodies
- **Sun**: Instanced billboard with solar texture
- **Black Hole**: Sphere mesh with event horizon and accretion disk
- **Wormhole**: Torus mesh with throat geometry and portal logic

#### Debug Grid
A green cross-hatch grid (84 vertices) that:
- Validates the rendering pipeline
- Provides spatial reference
- Appears when scene graph is uninitialized

## Technical Implementation

### Rendering Pipeline

```cpp
// Main render loop
void UI4D_ImGui::render4DView() {
    // 1. Bind FBO for off-screen rendering
    m_fbo.bind();
    
    // 2. Clear with background color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 3. Render debug grid (fallback)
    renderDebugGrid();
    
    // 4. Render celestial bodies
    if (m_sceneGraph) {
        for (auto& body : m_sceneGraph->getBodies()) {
            body->render(shader, camera, time);
        }
    }
    
    // 5. Unbind FBO and display on ImGui
    m_fbo.unbind();
    ImGui::Image((void*)m_fbo.getTexture(), ImVec2(width, height));
}
```

### 4D Coordinate Slicing

Objects are rendered based on their 4D position relative to the current time slice:

```cpp
float alpha = smoothstep(threshold, 0.0, abs(w_position - m_globalW));
if (alpha > 0.01f) {
    body->render(shader, camera, alpha);
}
```

### Camera Position Sync

The camera position is synchronized with the flight telemetry panel:

```cpp
Event4D camPos = camera.getPosition();
flightController.setPosition(camPos);
```

## Troubleshooting

### Black Screen Issues

1. **Check OpenGL Context**: Ensure core profile is enabled
   ```cpp
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
   ```

2. **Verify Camera Position**: Default is `(0, 0, 5, 15)`, not origin
3. **Check Scene Graph**: Ensure `m_sceneGraph` is initialized
4. **Validate Shaders**: Check compilation logs in `CurvatureRenderer`

### RDP/Virtual Environment Issues

Remote Desktop and virtual machines may cause:
- `GL_INVALID_VALUE (0x501)` errors due to software rendering
- Missing geometry for complex meshes
- **Solution**: Run natively on host machine with hardware acceleration

## Performance Considerations

### Frame Rate Targets
- **Target**: >30 FPS on GTX 1650
- **Adjustable**: Particle count, bloom resolution, texture quality

### Optimization Techniques
- Instanced rendering for stars
- Persistent buffer mapping for dynamic geometry
- Level-of-detail for distant objects
- Occlusion culling for hidden bodies

## See Also

- [`docs/UI4D_ImGui_Documentation.md`](UI4D_ImGui_Documentation.md) - Full UI system documentation
- [`src/spacetime/Event4D.h`](src/spacetime/Event4D.h) - 4D event class
- [`src/physics/DifferentiableGeodesicIntegrator.h`](src/physics/DifferentiableGeodesicIntegrator.h) - Geodesic integration
- [`src/rendering/CurvatureRenderer.h`](src/rendering/CurvatureRenderer.h) - Curvature visualization