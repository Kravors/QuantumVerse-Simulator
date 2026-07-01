# QuantumVerse Windows Deployment Guide

## Overview

This document provides comprehensive information about Windows deployment for the QuantumVerse Simulator v2.2.0, including build processes, common issues, crash diagnostics, and troubleshooting steps.

## Build System

### Build Directories

| Directory | Purpose | Output |
|-----------|---------|--------|
| `build_qml/` | QML/Qt Quick build | `quantumverse_qml.exe` |
| `build_imgui/` | ImGui/GLFW build | `quantumverse_imgui.exe` |
| `build_qt/` | Qt Widgets build | (deprecated) |
| `build_asan/` | AddressSanitizer build | For debugging |

### Build Commands

```batch
REM Build QML version
cd build_qml
cmake --build . --config Release --target quantumverse_qml

REM Build ImGui version
cd build_imgui
cmake --build . --config Release --target quantumverse_imgui

REM Full deployment
deploy.bat
```

## Deployment Structure

### Output Directory: `deploy/windows/`

```
deploy/windows/
├── quantumverse_qml.exe      # QML GUI executable (627 KB)
├── quantumverse_imgui.exe    # ImGui GUI executable (1.27 MB)
├── dilaton.lib               # Main library (25 MB)
├── glfw3.dll                 # GLFW runtime
├── Qt6Core.dll               # Qt core library
├── Qt6Gui.dll                # Qt GUI library
├── Qt6Widgets.dll            # Qt widgets library
├── Qt6Qml.dll                # Qt QML library
├── Qt6Quick.dll              # Qt Quick library
├── plugins/                  # Qt plugins
│   ├── platforms/
│   │   └── qwindows.dll
│   └── ...
├── qml/                      # QML source files
│   └── QuantumVerse/
│       ├── main.qml
│       └── qmldir
├── resources.qrc             # Qt resource file
└── user_prefs.ini            # User preferences
```

## Common Issues and Solutions

### 1. QML Crash After "Active quantum theory set to: CDT"

**Symptoms:**
- Application shows console output up to "Active quantum theory set to: CDT"
- Then immediately crashes without error message

**Root Causes:**
1. `CelestialBodyRenderer::addBody()` called before GL context is available
2. `synchronize()` method calls `isInitialized()` on null or uninitialized renderers
3. Shader compilation failure in `initializeGL()`

**Fixes Applied:**
- Removed premature `addBody` calls in `main_qml.cpp`
- Added try-catch blocks in `synchronize()` method
- Deferred body population to `renderGeodesics()` in `qmlglviewport.cpp`

**Code Location:**
- `src/main_qml.cpp:249-285` - Main entry point
- `src/qmlglviewport.cpp:246-261` - synchronize() method

### 2. ImGui UI Freeze

**Symptoms:**
- Application starts but UI becomes unresponsive
- No rendering in 4D View or Slice Views panels
- May show black/empty panels

**Root Causes:**
1. `curvatureRenderer` is null when `render4DView()` is called
2. `curvatureRenderer->isInitialized()` called on null pointer
3. Framebuffer incompleteness causing render loop to hang

**Fixes Applied:**
- Added null checks in `render4DView()` before accessing curvatureRenderer
- Added try-catch blocks around renderer calls
- Added fallback rendering for incomplete framebuffers

**Code Location:**
- `src/ui_imgui/UI4D_ImGui.cpp:1188-1293` - render4DView()
- `src/ui_imgui/UI4D_ImGui.cpp:152-191` - SliceViewPanel::render()

### 3. Missing DLL Errors

**Symptoms:**
- "The code execution cannot proceed because Qt6Core.dll was not found"
- "Missing vcruntime140.dll"

**Solution:**
Run `deploy.bat` to copy all required DLLs to the deployment directory.

### 4. OpenGL Context Issues

**Symptoms:**
- "Failed to initialize GLAD"
- Black screen or rendering artifacts

**Solution:**
- Ensure OpenGL 4.5 compatible graphics driver
- Check that `QSurfaceFormat` is set before window creation
- Verify `gladLoadGL()` returns true

**Code Location:**
- `src/main_glfw.cpp:116-125` - OpenGL initialization
- `src/main_qml.cpp:67-78` - Surface format setup

## Debugging Strategy

### Step 1: Check Console Output

Both executables output diagnostic information to the console:

```
QuantumVerse: Starting QML application...
QuantumVerse: OpenGL format set to 4.5 Core Profile
QuantumVerse: Created Schwarzschild BH, r_s = 29530 m
QuantumVerse: Setting active quantum theory to CDT
QuantumVerse: Active quantum theory set to: CDT
QuantumVerse: Renderers, UI4D, Camera4DAdapter, and CelestialBodyRenderer wired to QML viewport
```

If output stops at any point, that's where the crash occurs.

### Step 2: Enable Debug Logging

Add qDebug() or std::cout statements in key locations:

```cpp
// In synchronize()
qDebug() << "QmlGlRenderer::synchronize() called";
qDebug() << "curvatureRenderer =" << m_curvatureRenderer.get();
qDebug() << "quantumRenderer =" << m_quantumRenderer.get();
qDebug() << "celestialBodyRenderer =" << m_celestialBodyRenderer.get();
```

### Step 3: Use AddressSanitizer Build

```batch
cd build_asan
cmake --build . --config Debug
```

This will catch memory issues and null pointer dereferences.

### Step 4: Check OpenGL Errors

Add after each OpenGL call:

```cpp
GLenum err;
while ((err = glGetError()) != GL_NO_ERROR) {
    std::cerr << "OpenGL error: " << err << std::endl;
}
```

## Error Detection Checklist

### Pre-Build Checks
- [ ] CMake configuration succeeds (no generator mismatch)
- [ ] All source files compile without errors
- [ ] No missing header includes

### Post-Build Checks
- [ ] Executables exist in `build_qml/Release/` and `build_imgui/Release/`
- [ ] File sizes are reasonable (>100 KB)
- [ ] No missing symbols in dependency check

### Runtime Checks
- [ ] Console output shows all initialization steps
- [ ] GL context is created (OpenGL version printed)
- [ ] Renderers initialize without shader errors
- [ ] QML loads main.qml successfully
- [ ] ImGui dock layout builds without errors

## Known Limitations

### Windows-Specific Issues
1. **High DPI Scaling**: May require `Qt::AA_EnableHighDpiScaling` attribute
2. **OpenGL Core Profile**: Some drivers may not fully support 4.5 Core Profile
3. **Thread Safety**: Qt's `synchronize()` runs on render thread; ensure GL context is current

### Renderer Dependencies
1. **CurvatureRenderer**: Requires valid `MetricTensor` before `initializeGL()`
2. **CelestialBodyRenderer**: Requires GL context; defer `addBody()` calls
3. **QuantumGeometryRenderer**: Requires CDT engine to be set

## Testing Procedure

### Automated Testing
```batch
cd build_test
ctest --output-on-failure
```

### Manual Testing
1. Run `deploy\windows\quantumverse_qml.exe`
   - Should show QML window with 4D viewport
   - Check for solar system objects
   - Verify camera controls work

2. Run `deploy\windows\quantumverse_imgui.exe`
   - Should show ImGui window with dock layout
   - Check 4D View panel renders
   - Check Slice Views panel renders
   - Verify menu bar and tool palette

## Troubleshooting Flowchart

```
Application crashes?
├─ Check console output
│  ├─ Stops at "Active quantum theory" → QML crash fix
│  ├─ Stops at "Initializing celestial" → GL context issue
│  └─ No output → Missing DLL or startup error
├─ UI freezes?
│  ├─ Check render4DView null checks
│  ├─ Check framebuffer completeness
│  └─ Check curvatureRenderer initialization
└─ Rendering issues?
   ├─ Check OpenGL version
   ├─ Check shader compilation
   └─ Check renderer initialization order
```

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 2.2.0 | 2026-06-08 | Initial Windows deployment with QML and ImGui |
| 2.2.1 | 2026-06-10 | Fixed QML crash, ImGui freeze, added error handling |

## Related Files

- `deploy.bat` - Deployment script
- `CMakeLists.txt` - Build configuration
- `src/main_qml.cpp` - QML entry point
- `src/main_glfw.cpp` - ImGui entry point
- `src/qmlglviewport.cpp` - QML OpenGL viewport
- `src/ui_imgui/UI4D_ImGui.cpp` - ImGui interface