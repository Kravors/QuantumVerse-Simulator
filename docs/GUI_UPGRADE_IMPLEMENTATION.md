# QuantumVerse GUI Upgrade Implementation Guide

## Overview

This document provides implementation details for the QuantumVerse Simulator GUI upgrade, covering all completed features and their integration points.

---

## Table of Contents

1. [Architecture](#architecture)
2. [Security Module](#security-module)
3. [Performance Monitoring](#performance-monitoring)
4. [Accessibility Support](#accessibility-support)
5. [Onboarding System](#onboarding-system)
6. [VR Integration](#vr-integration)
7. [Theming System](#theming-system)
8. [Testing](#testing)
9. [Build Configuration](#build-configuration)

---

## Architecture

### Dual UI Implementation

The QuantumVerse Simulator maintains two parallel UI implementations:

| Component | Qt/QML Path | Dear ImGui Path |
|-----------|-------------|-----------------|
| Entry Point | `src/main_qml.cpp` | `src/main_glfw.cpp` |
| Main Window | `src/MainWindow.h/cpp` | `src/ui_imgui/UI4D_ImGui.h/cpp` |
| Viewport | `src/qmlglviewport.cpp` | `UI4D_ImGui` panels |
| Camera Control | `src/qmlcamcontroller.h` | `UI4D_ImGui` input handling |
| 4D Navigation | `src/ui4d/Camera4DAdapter.h` | `UI4D_ImGui` camera state |

### Module Dependencies

```
ui_imgui
├── ui4d (SceneGraphManager, UI4D)
├── rendering (CurvatureRenderer, CelestialBodyRenderer)
├── spacetime (MetricTensor, Event4D, LightCone)
├── physics (SingularityHandler)
├── utils (PerformanceMonitor, ConfigManager, Security)
└── discovery (DiscoveryPanelManager)
```

---

## Security Module

### Location
- Header: `src/utils/Security.h`
- Implementation: `src/utils/Security.cpp`
- Tests: `tests/test_security.cpp`

### API Reference

```cpp
namespace quantumverse::utils {

class Security {
public:
    // Path validation - prevents traversal attacks
    static std::filesystem::path validatePath(const std::string& path);
    
    // String sanitization - removes dangerous characters
    static std::string sanitizeString(const std::string& input, size_t maxLength = 0);
    
    // Physics parameter validation
    static bool validateMass(double mass);  // 0 < mass < 1e50 kg
    static bool validateSpin(double spin);  // 0 <= spin < 1.0
    
    // File extension whitelist
    static bool isAllowedExtension(const std::string& path);
    static const std::vector<std::string>& getAllowedExtensions();
};

} // namespace quantumverse::utils
```

### Allowed File Extensions

- `.spc` - Spacetime configuration files
- `.json` - JSON data files
- `.ini` - Configuration files
- `.txt` - Text files
- `.csv` - Comma-separated values
- `.dat` - Binary data files

### Security Best Practices

1. **Always validate paths** before file operations:
   ```cpp
   try {
       auto safePath = Security::validatePath(userInput);
       // Safe to use path
   } catch (const std::invalid_argument& e) {
       // Handle invalid path
   }
   ```

2. **Sanitize all user input**:
   ```cpp
   std::string clean = Security::sanitizeString(userInput, 256);
   ```

3. **Validate physics parameters**:
   ```cpp
   if (!Security::validateMass(mass)) {
       throw std::invalid_argument("Invalid mass value");
   }
   ```

---

## Performance Monitoring

### Location
- Header: `src/utils/PerformanceMonitor.h`
- Implementation: `src/utils/PerformanceMonitor.cpp`
- Tests: `tests/test_performance_monitor.cpp`

### Metrics Collected

| Metric | Type | Description |
|--------|------|-------------|
| `frameTimeMs` | double | Frame time in milliseconds |
| `fps` | double | Frames per second |
| `renderTimeMs` | double | Rendering time in ms |
| `updateTimeMs` | double | Update time in ms |
| `drawCalls` | int | Number of draw calls |
| `triangleCount` | int | Number of triangles rendered |
| `memoryUsedMB` | size_t | Memory used in MB |
| `vramUsedMB` | size_t | VRAM used in MB |

### Usage

```cpp
// In main loop
PerformanceMonitor monitor;
monitor.beginFrame();

// ... render frame ...

monitor.endFrame();

// Get metrics
const auto& metrics = monitor.getMetrics();
std::cout << "FPS: " << metrics.fps << std::endl;

// Scoped timing
{
    QUANTUMVERSE_SCOPED_TIMER(monitor, "render");
    // ... timed operation ...
}
```

### Performance Targets

- **UI Render Time**: <2ms per frame
- **Frame Rate**: ≥45 FPS at 1080p
- **Memory Usage**: <2GB baseline
- **Load Time**: <3 seconds cold start

---

## Accessibility Support

### Location
- Header: `src/ui_imgui/AccessibilityManager.h`
- Implementation: `src/ui_imgui/AccessibilityManager.cpp`

### Features

- **WCAG 2.1 AA Compliance**
- **Keyboard Navigation** - Full UI navigable via keyboard
- **High Contrast Mode** - Toggle for visual accessibility
- **Screen Reader Compatibility** - Proper labeling for assistive technologies
- **Configurable Focus Indicators** - Visual focus cues

### Usage

```cpp
AccessibilityManager a11y;
a11y.setHighContrast(true);
a11y.setFocusIndicatorColor(ImVec4(1, 1, 0, 1));  // Yellow focus
```

---

## Onboarding System

### Location
- Header: `src/ui_imgui/OnboardingPanel.h`
- Implementation: `src/ui_imgui/OnboardingPanel.cpp`

### Tutorial Steps

1. Welcome and Overview
2. 4D Navigation (WASD + QE)
3. 4D View Controls
4. Slice Views
5. Planck Microscope
6. Discovery Panel
7. Causal Graph
8. Theory Manager
9. Timeline Controls
10. Object Browser
11. Property Editor

### Usage

```cpp
// Auto-starts on first run
onboardingPanel.render(displaySize);

// Manual trigger
onboardingPanel.start();  // F1 key
```

---

## VR Integration

### Location
- Header: `src/ui_imgui/VRIntegration.h`
- Implementation: `src/ui_imgui/VRIntegration.cpp`

### Supported Modes

| Mode | Description |
|------|-------------|
| `None` | Desktop mode |
| `Stereo` | Side-by-side stereo 3D |
| `OpenXR` | OpenXR runtime VR |
| `AR` | Augmented reality overlay |

### Features

- **OpenXR-based VR framework**
- **Render scale configuration**
- **Hand tracking support detection**
- **Head pose tracking**
- **VR settings panel**

### Usage

```cpp
VRIntegration vr;
vr.setVRMode(VRMode::OpenXR);
vr.setRenderScale(1.0f);

if (vr.initialize() && vr.startSession()) {
    // VR session active
}
```

---

## Theming System

### Location
- Header: `src/ui_imgui/style/UIStyleManager.h`
- Implementation: `src/ui_imgui/style/UIStyleManager.cpp`

### Available Themes

- `DarkPro` - Dark theme with professional styling
- `Light` - Light theme for daylight use
- `HighContrast` - High contrast for accessibility
- `Solarized` - Solarized color scheme

### Usage

```cpp
UIStyleManager style;
style.applyTheme(UIStyleManager::Theme::DarkPro);
```

---

## Testing

### Test Categories

| Category | Test File | Description |
|----------|-----------|-------------|
| Security | `tests/test_security.cpp` | Input validation, path sanitization |
| Performance | `tests/test_performance_monitor.cpp` | Frame timing, metrics collection |
| Integration | `tests/test_integration.cpp` | Cross-module functionality |

### Running Tests

```bash
# Build all tests
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Run all tests
cd build && ctest --output-on-failure

# Run specific test
cd build && ctest -R SecurityTest --output-on-failure
```

### CI/CD Integration

GitHub Actions workflow at `.github/workflows/cross-platform-test.yml` provides:
- Windows, macOS, Linux testing
- Security test execution
- Performance test execution
- Code quality checks (Clang-Tidy, Cppcheck)

---

## Build Configuration

### CMake Options

```cmake
option(QUANTUMVERSE_USE_IMGUI "Build the 4D ImGui GUI" ON)
option(QUANTUMVERSE_USE_QML "Build the QML 4D GUI" OFF)
option(QUANTUMVERSE_USE_ONNX "Build with ONNX Runtime support" OFF)
```

### Platform-Specific Notes

**Windows:**
- Requires MSVC 2019+ or MinGW
- OpenGL 4.5 support via GLAD

**macOS:**
- Requires Xcode 13+
- Metal optional backend (OpenGL primary)

**Linux:**
- Requires GCC 10+ or Clang 12+
- X11 and Wayland support

---

## Integration Checklist

- [x] Security utilities implemented
- [x] Performance monitoring integrated
- [x] Accessibility support added
- [x] Onboarding tutorial created
- [x] VR integration stubbed
- [x] Theming system implemented
- [x] Unit tests for security
- [x] Cross-platform CI workflow
- [ ] Performance optimization (ongoing)
- [ ] Documentation completion (ongoing)