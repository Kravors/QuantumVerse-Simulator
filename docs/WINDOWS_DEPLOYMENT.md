# QuantumVerse Windows Deployment Guide

## Overview

This document provides comprehensive information about Windows deployment for the QuantumVerse Simulator v7.2, including build processes, installer creation, common issues, crash diagnostics, and troubleshooting steps.

## Release Summary

| Item | Value |
|------|-------|
| **Version** | v7.2 |
| **Tag** | `v7.2-final` |
| **Installer** | `QuantumVerse-7.2-Setup.exe` (~32.4 MB) |
| **Release URL** | `https://github.com/Kravors/QuantumVerse-Simulator/releases/tag/v7.2-final` |
| **Download URL** | `https://github.com/Kravors/QuantumVerse-Simulator/releases/download/v7.2-final/QuantumVerse-7.2-Setup.exe` |
| **Install location** | `%LOCALAPPDATA%\Programs\QuantumVerse\` (non-admin, no elevation) |
| **Installer type** | NSIS (`installer/quantumverse.iss`) |
| **Signing** | Unsigned — Windows SmartScreen will show "Windows protected your PC"; click "More info" → "Run anyway" |

## Build System

### Build Directories

| Directory | Purpose | Output |
|-----------|---------|--------|
| `build/` | Primary QML/Qt Quick build | `quantumverse_qml.exe` |

### Build Commands

```batch
REM Build QML version
cmake -B build -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_BUILD_TESTS=ON
cmake --build build --parallel

REM Run tests
cd build
ctest -C Release --output-on-failure

REM Full deployment
deploy.bat

REM Build installer (Inno Setup)
cd installer
iscc quantumverse.iss
```

## Installer (NSIS)

The v7.2 installer is built with Inno Setup using `installer/quantumverse.iss`.

### Installer Configuration
- **WorkingDir**: `{app}` on shortcuts
- **Install location**: `%LOCALAPPDATA%\Programs\QuantumVerse\`
- **Non-admin install**: No elevation required
- **Excluded from installer**: `*.h5`, `*.hdf5`, `*.pth`, `*.ckpt`, `*.onnx` (training artifacts, 852 MB removed)
- **Removed**: `[UninstallDelete]` directive (redundant — Inno Setup auto-deletes `{app}` on uninstall)
- **MiKTeX prerequisite check**: Included

### Shipping Notes
- Installer is **unsigned** — Windows SmartScreen will block on first launch
- User workaround: click "More info" → "Run anyway"
- For public distribution, purchase a code-signing certificate (~$100-300/yr)
- Test on clean VM (Windows Sandbox) before public release
- `Documents\QuantumVerse\output` survives uninstall

## Deployment Structure

### Output Directory: `deploy/windows/`

```
deploy/windows/
├── quantumverse_qml.exe      # QML GUI executable
├── Qt6Core.dll               # Qt core library
├── Qt6Gui.dll                # Qt GUI library
├── Qt6Quick.dll              # Qt Quick library
├── Qt6Qml.dll                # Qt QML library
├── Qt6WebSockets.dll         # Qt WebSockets library
├── Qt6Network.dll            # Qt Network library
├── plugins/                  # Qt plugins
│   └── platforms/
│       └── qwindows.dll
├── qml/                      # QML source files
│   └── QuantumVerse/
│       ├── main.qml
│       └── qmldir
└── user_prefs.ini            # User preferences
```

## Common Issues and Solutions

### 1. Missing DLL Errors

**Symptoms:**
- "The code execution cannot proceed because Qt6Core.dll was not found"
- "Missing vcruntime140.dll"

**Solution:**
Run `deploy.bat` to copy all required DLLs to the deployment directory.

### 2. OpenGL Context Issues

**Symptoms:**
- "Failed to initialize OpenGL"
- Black screen or rendering artifacts

**Solution:**
- Ensure OpenGL 4.5 compatible graphics driver
- Verify `QSurfaceFormat` is set before window creation
- Use `QSG_RHI_BACKEND=d3d11` or `opengl` as needed

**Code Location:**
- `src/main_qml.cpp:67-78` - Surface format setup

### 3. Headless Mode DLL Loading

**Symptoms:**
- `0xC0000135` error when running `--headless`

**Solution:**
Ensure Qt `bin` directory is on `PATH` before running headless mode.

## Debugging Strategy

### Step 1: Check Console Output

```
QuantumVerse: Starting QML application...
QuantumVerse: OpenGL format set to 4.5 Core Profile
QuantumVerse: MultiUserServer initialized
QuantumVerse: Renderers, UI4D, Camera4DAdapter, and CelestialBodyRenderer wired to QML viewport
```

If output stops at any point, that's where the crash occurs.

### Step 2: Enable Debug Logging

Add `qDebug()` or `std::cout` statements in key locations.

### Step 3: Use AddressSanitizer Build

```batch
cmake -B build_asan -DCMAKE_BUILD_TYPE=Debug -DQUANTUMVERSE_USE_ASAN=ON
cmake --build build_asan --parallel
```

This will catch memory issues and null pointer dereferences.

### Step 4: Check OpenGL Errors

```cpp
GLenum err;
while ((err = glGetError()) != GL_NO_ERROR) {
    std::cerr << "OpenGL error: " << err << std::endl;
}
```

## Pre-Build Checks

- [ ] CMake configuration succeeds (no generator mismatch)
- [ ] All source files compile without errors
- [ ] No missing header includes

## Post-Build Checks

- [ ] Executables exist in `build/Release/`
- [ ] File sizes are reasonable (>100 KB)
- [ ] No missing symbols in dependency check
- [ ] Installer builds successfully with `iscc`

## Runtime Checks

- [ ] Console output shows all initialization steps
- [ ] GL context is created (OpenGL version printed)
- [ ] Renderers initialize without shader errors
- [ ] QML loads `main.qml` successfully
- [ ] MultiUserServer initializes without WebSocket errors
- [ ] Installer installs to `%LOCALAPPDATA%\Programs\QuantumVerse\`

## Known Limitations

### Windows-Specific Issues
1. **High DPI Scaling**: May require `Qt::AA_EnableHighDpiScaling` attribute
2. **OpenGL Core Profile**: Some drivers may not fully support 4.5 Core Profile
3. **Thread Safety**: Qt's `synchronize()` runs on render thread; ensure GL context is current
4. **Unsigned Installer**: Windows SmartScreen will block on first launch

### Renderer Dependencies
1. **CurvatureRenderer**: Requires valid `MetricTensor` before `initializeGL()`
2. **CelestialBodyRenderer**: Requires GL context; defer `addBody()` calls
3. **QuantumGeometryRenderer**: Requires CDT engine to be set

## Testing Procedure

### Automated Testing
```batch
cd build
ctest --output-on-failure
```

### Manual Testing
1. Run `deploy\windows\quantumverse_qml.exe`
   - Should show QML window with 4D viewport
   - Check for solar system objects
   - Verify camera controls work
2. Run installer `QuantumVerse-7.2-Setup.exe`
   - Verify non-admin install to `%LOCALAPPDATA%\Programs\QuantumVerse\`
   - Verify all data loads (config, data, tours, ML model)
   - Verify uninstall removes install dir + shortcuts
   - Verify `Documents\QuantumVerse\output` survives uninstall

## Version History

| Version | Date | Changes |
|---------|------|---------|
| **v7.2** | 2026-10-26 | Kerr observables + deliverable installer; grid deformation fix; discovery paper pipeline |
| 3.9.0 | 2026-09-29 | VR multi-user sessions, volumetric disk, CI hygiene |
| 3.8.0 | 2026-07-18 | Real-time multi-messenger pipeline, 16 discovery instruments |
| 3.7.0 | 2026-07-12 | Removed ImGui, Qt-only build |

## Related Files

- `deploy.bat` - Deployment script
- `CMakeLists.txt` - Build configuration
- `installer/quantumverse.iss` - Inno Setup installer script
- `src/main_qml.cpp` - QML entry point
- `src/qmlglviewport.cpp` - QML OpenGL viewport
- `src/vr/MultiUserServer.cpp` - Multi-user VR server
