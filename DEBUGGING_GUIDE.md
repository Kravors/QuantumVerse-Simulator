# QuantumVerse Simulator - Debugging Guide

## Overview

This document provides a step-by-step debugging guide for the QuantumVerse Simulator GUI, addressing the reported issues where clicking buttons for solar system simulation, spacetime fabric visualization, and black hole rendering produces no response or output.

## Root Cause Analysis

After analyzing the codebase, the following issues were identified and fixed:

### 1. Qt Namespace Collision (CRITICAL - Windows/MSVC)
**Problem:** Qt 6.11+ auto-detects `QT_NAMESPACE` from the CMake project name "QuantumVerse", which becomes "quantumverse". This nests STL types as `quantumverse::std::` when inside the `quantumverse` namespace, breaking type resolution.

**Fix Applied:**
- [`CMakeLists.txt:12`](CMakeLists.txt:12) - Set `QT_NAMESPACE` to "Qt" before `project()`
- [`CMakeLists.txt:134-135`](CMakeLists.txt:134) - Set `QT_NAMESPACE=""` and `QT_NO_NAMESPACE=ON` before `find_package(Qt6)`
- [`cmake/QtNamespaceConfig.h.in:23-29`](cmake/QtNamespaceConfig.h.in:23) - Force-include header that undefines `QT_NAMESPACE`

### 2. M_PI Undefined on MSVC
**Problem:** MSVC's `<cmath>` doesn't define `M_PI` even with `_USE_MATH_DEFINES`.

**Fix Applied:**
- [`src/physics/SingularityHandler.h:19-21`](src/physics/SingularityHandler.h:19) - Added fallback `#define M_PI`
- [`src/ui4d/UI4D.h:33-35`](src/ui4d/UI4D.h:33) - Added fallback `#define M_PI`
- [`src/qmlglviewport.h:43-45`](src/qmlglviewport.h:43) - Added fallback `#define M_PI`

### 3. QmlGlViewport findChild() Failure
**Problem:** `findChild<QmlGlViewport*>()` in [`src/main_qml.cpp:210`](src/main_qml.cpp:210) failed because the viewport had no `objectName` set.

**Fix Applied:**
- [`src/qmlglviewport.cpp:1071`](src/qmlglviewport.cpp:1071) - Added `setObjectName("viewport")` in constructor

### 4. QuantumGeometryRenderer Destructor Override
**Problem:** `~QuantumGeometryRenderer()` had `override` specifier but `CurvatureRenderer` has non-virtual destructor.

**Fix Applied:**
- [`src/rendering/QuantumGeometryRenderer.h:143`](src/rendering/QuantumGeometryRenderer.h:143) - Changed to `~QuantumGeometryRenderer() = default;`

## Debugging Steps

### Step 1: Verify Build Configuration

```bash
# Clean build directory
rmdir /s /q build_qml
mkdir build_qml
cd build_qml

# Configure with QML enabled
cmake -B . -DQUANTUMVERSE_USE_QML=ON -DQUANTUMVERSE_USE_IMGUI=OFF

# Check for Qt6 detection
# Look for: "Qt6 found - building Qt/QML GUI"
```

### Step 2: Build and Check for Errors

```bash
cmake --build . --config Release --parallel
# Check for:
# - C2065: 'M_PI': undeclared identifier
# - C2665: 'std::make_shared': no matching overloaded function
# - C3646: 'override': method with override specifier 'override' did not override any base class method
```

### Step 3: Run with Debug Output

```bash
# Set environment variable for Qt debug output
set QT_LOGGING_RULES="qt.core.qobject=true;qt.qml=true"

# Run the application
.\Release\quantumverse_qml.exe
```

### Step 4: Check Console Output

Look for these messages in the console:
- `QuantumVerse: Created Schwarzschild BH, r_s = ... m` - Black hole initialization
- `QuantumVerse: Registered 10 discovery instruments` - Discovery panel setup
- `QuantumVerse: Found QmlGlViewport, wiring renderers...` - Viewport found
- `QuantumVerse: Renderers, UI4D, Camera4DAdapter, and CelestialBodyRenderer wired to QML viewport` - Success

If you see:
- `QuantumVerse: CRITICAL - Could not find QmlGlViewport in QML object tree` - QML viewport not found

### Step 5: Verify QML Integration

The QML file [`src/main.qml`](src/main.qml) should have:
- `QmlGlViewport` properly declared with `id: viewport`
- MouseArea handlers connected to `viewport.handleMousePress()`, `viewport.handleMouseReleased()`, etc.
- Timer connected to `viewport.updateSimulation(dt)`

## Event Handler Flow

### Solar System Simulation
1. QML Timer (line 911-922) triggers `viewport.updateSimulation(dt)`
2. [`QmlGlViewport::updateSimulation()`](src/qmlglviewport.cpp:1271) updates time and calls renderer
3. [`QmlGlRenderer::render()`](src/qmlglviewport.cpp:119) calls `renderGeodesics()`
4. [`renderGeodesics()`](src/qmlglviewport.cpp:710) gets solar system data from `m_ui4d`
5. Celestial bodies are rendered via `m_celestialBodyRenderer`

### Spacetime Fabric Visualization
1. Menu "Show Grid" toggles `viewport.showGrid`
2. [`QmlGlViewport::setShowGrid()`](src/qmlglviewport.cpp:1096) updates renderer
3. [`QmlGlRenderer::render()`](src/qmlglviewport.cpp:165) calls `renderGrid()`
4. Grid shader deforms vertices based on curvature

### Black Hole Rendering
1. Black hole created in [`main_qml.cpp:102-112`](src/main_qml.cpp:102)
2. Singularity added to curvature renderer
3. Grid deformation shows spacetime curvature around BH

## Common Issues and Solutions

### Issue: "Could not find QmlGlViewport"
**Check:**
1. Qt6 is installed at `F:/qt/6.11.1/msvc2022_64`
2. `qmlRegisterType<QmlGlViewport>()` is called in [`registerQmlTypes()`](src/main_qml.cpp:48)
3. QML imports `QuantumVerse 1.0`
4. `QmlGlViewport` has `setObjectName("viewport")`

### Issue: No rendering output
**Check:**
1. OpenGL 4.5 context is created (check `createSurfaceFormat()`)
2. GLAD loads successfully (`gladLoadGL()` returns true)
3. Shaders compile without errors
4. `m_curvatureRenderer` is not null

### Issue: Buttons don't respond
**Check:**
1. QML `MouseArea` is properly connected
2. `QmlGlViewport` methods are `Q_INVOKABLE`
3. `setAcceptedMouseButtons(Qt::AllButtons)` is called

## Cross-Platform Compatibility

### Windows-Specific Fixes
- Use `F:/qt/6.11.1/msvc2022_64` (not 6.11.0)
- Add `/permissive-` and `/Zc:__cplusplus` for MSVC
- Define `NOMINMAX` to prevent Windows `min/max` macro conflicts

### Linux/macOS
- Use `-include` flag instead of `/FI` for force-include
- Ensure GLAD is compiled with correct OpenGL version

## Testing Commands

```bash
# Run QML viewport test
.\test_qml_viewport.exe

# Run with ctest
ctest -R QmlViewportTest --output-on-failure
```

## File Reference Summary

| File | Purpose | Key Lines |
|------|---------|-----------|
| [`CMakeLists.txt`](CMakeLists.txt) | Build configuration | 12, 134-135, 147-162 |
| [`src/main_qml.cpp`](src/main_qml.cpp) | QML entry point | 102-112, 205-261 |
| [`src/qmlglviewport.h`](src/qmlglviewport.h) | QML OpenGL viewport | 178-187, 401-460 |
| [`src/qmlglviewport.cpp`](src/qmlglviewport.cpp) | Viewport implementation | 1055-1074, 119-196 |
| [`src/ui4d/UI4D.h`](src/ui4d/UI4D.h) | 4D UI logic | 506-657, 859-733 |
| [`src/ui4d/UI4D.cpp`](src/ui4d/UI4D.cpp) | UI implementation | 506-657, 659-681 |
| [`src/main.qml`](src/main.qml) | QML UI definition | 364-370, 911-922 |
| [`cmake/QtNamespaceConfig.h.in`](cmake/QtNamespaceConfig.h.in) | Qt namespace fix | 23-29, 57-60 |