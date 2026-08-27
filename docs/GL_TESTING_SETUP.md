# Visual / GL Testing Setup Guide

## Current Status

The rendering pipeline is mature and well-structured:
- `CelestialBodyRenderer` — UV spheres with Phong lighting, procedural textures
- `CurvatureRenderer` — shader-based curvature visualization (5 modes)
- `GLDebug` — OpenGL debug callback with error counting
- `test_rendering_diagnostic.cpp` — comprehensive diagnostic with PNG screenshots

## Windows GL Setup Attempts

### Attempted Solutions (All Failed)

1. **Qt Built-in Software GL** (`QT_OPENGL=software`) — Not available in Qt 6.11.1
2. **ANGLE** (`QT_OPENGL=angle`) — Not bundled with this Qt installation
3. **Microsoft OpenGL Compatibility Pack** — Provides headers/libs but no software renderer
4. **WSL (Ubuntu)** — Disk mounting broken at system level
5. **MSYS2** — Incomplete installation (missing core utilities)
6. **Mesa3D 26.2.0 (llvmpipe)** — DLLs deployed but `QOpenGLContext::create()` still fails

### Root Cause

This Windows machine lacks a GPU driver that supports OpenGL 4.5. Qt's `QOpenGLContext` requires a working OpenGL driver (hardware or software) to create a context. The Mesa llvmpipe software renderer was deployed per-application but the context creation still fails, likely due to:
- Missing Visual C++ runtime dependencies for Mesa DLLs
- Qt loading its own OpenGL stub before Mesa's
- Incompatibility between Qt's WGL implementation and Mesa's WGL

## Recommended Next Steps

### Option 1: Use a Machine with GPU Support
Run the diagnostic test on a machine with:
- NVIDIA/AMD GPU with OpenGL 4.5+ drivers, OR
- Intel integrated graphics with latest drivers

### Option 2: Linux CI
The diagnostic test is designed for Linux CI with:
```bash
xvfb-run -a ./build/test_rendering_diagnostic
```
This generates PNG screenshots for visual inspection.

### Option 3: Remote Rendering
Use a cloud VM (AWS, Azure, GCP) with:
- Ubuntu 22.04+
- Mesa llvmpipe (`apt install mesa-utils libgl1-mesa-dri`)
- Xvfb for headless rendering

## Mock GL Mode (Current Capability)

The diagnostic test's mock GL mode validates CPU-side rendering logic without a GL context:
- Grid generation (vertex/index count)
- Kretschmann scalar computation
- Color mapping (RGBA validity)
- Configuration validation
- Mode switching

Run with:
```bash
QT_QPA_PLATFORM=offscreen ./build/test_rendering_diagnostic
```

## Diagnostic Test Output

When GL is available, the test generates:
- `diagnostic_celestial_body.png` — planet + star rendering
- `diagnostic_curvature_grid.png` — Schwarzschild grid deformation
- `diagnostic_curvature_riemann.png` — Riemann tensor color mapping
- `diagnostic_curvature_scalar.png` — Kretschmann scalar visualization

## GL-1281 Investigation

The original GL-1281 error (GL_INVALID_VALUE) was the motivation for the diagnostic test. With the enhanced test:
1. Run on Linux CI with Mesa llvmpipe
2. Inspect PNG outputs for visual anomalies
3. Check GL error counts reported by GLDebug
4. Compare against reference images for regression detection
