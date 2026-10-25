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

## Lensing Camera Verification (GPU Machine)

### Frame-0 cold-start artifact (corrected 2026-10-20)

Frame #0 renders before `engine.load()` completes, so `QmlGlRenderer::synchronize()`
copies `m_lensing = null` into the renderer at line 882 before `main_qml.cpp:1336`
builds it via `syncLensingParams()`. The first `[DIAG-renderLensing]` SKIP with
`m_lensing= null` at frame 0 is expected — not a shipped bug. Lensing renders
correctly from frame 1 onward. Always verify at `--frames ≥ 2` (recommend
`--frames 10`), never `--frames 1`.

The half-frame split in Kerr black hole shadow rendering was fixed by matching
the lensing shader's camera to the QML viewport camera. On a machine with a
display, verify via:

```powershell
build\Debug\quantumverse_qml.exe --cam-az 90 --cam-el 0 --cam-dist 40 --cam-tx 40 --screenshot a0_baseline.png --frames 10
```

Check the CPU-side `[LENS]` debug log for:
- `u_cameraPos` matches QML camera world position (`camera4DAdapter->cameraPosition4D()`)
- `u_bhPos` matches config black hole position (e.g. `(40, 0, 0)`)
- `theta ≈ 1.5708` (π/2 equatorial, not pole)
- `u_cameraDir` points from camera toward BH (`normalize(bhPos - cameraPos)`)
- `hasCameraPos = true`

Watch the screenshot for:
- **Green pixels** → step budget insufficient (increase `raySteps`), not camera
- **Off-center shadow** → residual position/aim mismatch
- **Persistent half-split** → driver-side uniform upload or stale shader program

If `[LENS]` logs are correct but the render is wrong, re-check that the
`.glsl` shader recompiled after edits and that `u_cameraDir`/`u_cameraUp` are
bound to the active program at `lensing_fragment.glsl:465–468`.

## GL-1281 Investigation

The original GL-1281 error (GL_INVALID_VALUE) was the motivation for the diagnostic test. With the enhanced test:
1. Run on Linux CI with Mesa llvmpipe
2. Inspect PNG outputs for visual anomalies
3. Check GL error counts reported by GLDebug
4. Compare against reference images for regression detection
