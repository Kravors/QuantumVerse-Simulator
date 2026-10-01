# QuantumVerse Simulator — Complete Build Guide

**Version:** 3.9.0  
**Platform:** Windows 10/11 (MSVC 2022) — Linux/macOS commands noted where they differ  
**Last Updated:** 2026-09-30

---

## Table of Contents

1. [Prerequisites & Environment Setup](#1-prerequisites--environment-setup)
2. [Dependencies](#2-dependencies)
3. [CMake Configuration Options](#3-cmake-configuration-options)
4. [Step-by-Step Build Process](#4-step-by-step-build-process)
5. [Test Commands](#5-test-commands)
6. [Deployment & Packaging](#6-deployment--packaging)
7. [Runtime Modes](#7-runtime-modes)
8. [Platform-Specific Notes](#8-platform-specific-notes)
9. [Quick Reference](#9-quick-reference)

---

## 1. Prerequisites & Environment Setup

### Required Tools

| Component | Minimum Version | Recommended | Purpose |
|-----------|----------------|-------------|---------|
| **OS** | Windows 10 | Windows 11 | Target platform |
| **Visual Studio** | 2022 (v143) | 2022 (v143) | MSVC compiler |
| **CMake** | 3.20 | 3.25+ | Build system generator |
| **Qt** | 6.11.1 | 6.11.1 (msvc2022_64) | QML UI framework |
| **GPU Drivers** | OpenGL 4.5 compatible | Latest vendor drivers | Hardware rendering |

### Optional Tools

| Component | Version | Purpose |
|-----------|---------|---------|
| **Ninja** | Latest | Faster parallel builds |
| **NSIS** | Latest | Windows installer packaging |
| **ONNX Runtime** | 1.27.0 | ML inference fallback |
| **CUDA** | 12.x | Future GPU acceleration |
| **Python** | 3.10+ | ML training scripts only |
| **GSL** | 2.7+ | Wigner symbols (optional) |
| **glslangValidator** | Latest | Shader validation |

### Environment Setup Steps

```powershell
# 1. Ensure MSVC 2022 Developer Command Prompt is active
#    This sets up the compiler, linker, and Windows SDK paths.
#    Search for "x64 Native Tools Command Prompt for VS 2022" in Start Menu.

# 2. Add CMake to PATH (if not already)
$env:Path += ";C:\Program Files\CMake\bin"

# 3. Add Qt to PATH (for runtime)
$env:Path += ";C:\Qt\6.11.1\msvc2022_64\bin"

# 4. Verify toolchain
cmake --version          # Expect 3.25+
cl                       # Expect MSVC version info
where windeployqt        # Expect Qt bin path
```

**Important:** Do NOT use `%LOCALAPPDATA%\Microsoft\WindowsApps\python3.exe`. This is the Windows App Execution Alias stub that launches the Microsoft Store and stalls `std::system`/`_popen` calls for minutes.

---

## 2. Dependencies

### Automatically Downloaded by CMake (FetchContent)

| Library | Version | Usage |
|---------|---------|-------|
| **nlohmann/json** | v3.11.3 | JSON parsing (header-only) |

### System Dependencies (find_package)

| Library | How Found | Usage |
|---------|-----------|-------|
| **Qt6 Core/Gui/Widgets/Qml/Quick/QuickControls2/QuickTemplates2/QuickLayouts/OpenGL/OpenGLWidgets/Network/WebSockets/Test** | `find_package(Qt6 REQUIRED ...)` | QML UI, networking, testing |
| **Qt6 Multimedia** | Optional (`find_package(Qt6 QUIET COMPONENTS Multimedia)`) | Audio features |
| **CURL** | `find_package(CURL QUIET)` | HTTP adapters |
| **librdkafka** | `find_package(librdkafka QUIET)` | Kafka/GCN live ingest |
| **OpenXR SDK** | `find_path` + `find_library` | VR/OpenXR backend |

### Bundled Third-Party

| Component | Location | Purpose |
|-----------|----------|---------|
| **stb_image.h** | `third_party/stb_image.h` | Image loading |
| **glad/** | `third_party/glad/` | OpenGL function loader |
| **imgui/** | `third_party/imgui/` | Legacy Dear ImGui (inactive; Qt-only build) |

### Qt Modules Linked by `quantumverse_qml`

```
Qt6::Core, Qt6::Gui, Qt6::Widgets, Qt6::Qml, Qt6::Quick,
Qt6::QuickControls2, Qt6::QuickTemplates2, Qt6::QuickLayouts,
Qt6::OpenGL, Qt6::OpenGLWidgets, Qt6::Network, Qt6::WebSockets
```

Plus optionally `Qt6::Multimedia`.

---

## 3. CMake Configuration Options

All options are defined in `CMakeLists.txt` (lines 41–49).

### Core Options

| Option | Default | Description |
|--------|---------|-------------|
| `QUANTUMVERSE_BUILD_TESTS` | `ON` | Build the test suite |
| `QUANTUMVERSE_USE_QT` | **`OFF`** | **Must be `ON`** to build `quantumverse_qml` |
| `QUANTUMVERSE_USE_VR` | `OFF` | Enable OpenXR VR backend |
| `QUANTUMVERSE_USE_KAFKA` | `OFF` | Enable Kafka/GCN live ingest |
| `QUANTUMVERSE_USE_COVERAGE` | `OFF` | Enable `--coverage` instrumentation |

### Sanitizer Options (Debug only)

| Option | Default | Description |
|--------|---------|-------------|
| `QUANTUMVERSE_USE_ASAN` | `OFF` | AddressSanitizer (detects memory errors) |
| `QUANTUMVERSE_USE_UBSAN` | `OFF` | UndefinedBehaviorSanitizer |
| `QUANTUMVERSE_USE_TSAN` | `OFF` | ThreadSanitizer |
| `QUANTUMVERSE_USE_FUZZER` | `OFF` | libFuzzer targets (Clang only) |

### Performance Options

| Option | Default | Description |
|--------|---------|-------------|
| `QUANTUMVERSE_USE_BENCHMARK` | `OFF` | Google Benchmark targets |
| `QUANTUMVERSE_PERF_TRACE` | `OFF` | Enable per-stage render timers |
| `QUANTUMVERSE_ENFORCE_TIDY_ERRORS` | `ON` | Treat clang-tidy warnings as errors |

### Test Thresholds

| Option | Default | Description |
|--------|---------|-------------|
| `CTEST_PERFORMANCE_THRESHOLD` | `200` | Avg frame-time threshold (ms) |
| `CTEST_PERFORMANCE_MAX_THRESHOLD` | `250` | Max frame-time threshold (ms) |

### Standard CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | `Release` | Build configuration (`Debug`, `Release`, `RelWithDebInfo`) |
| `CMAKE_INSTALL_PREFIX` | `C:/Program Files/QuantumVerse` | Install destination |
| `CMAKE_PREFIX_PATH` | (unset) | Path to Qt installation (e.g. `C:\Qt\6.11.1\msvc2022_64`) |

**Important:** If tests are enabled and `CMAKE_BUILD_TYPE` is unset, CMake forces `Debug` to keep assertions live.

---

## 4. Step-by-Step Build Process

### Step 1: Configure

Choose the configuration matching your needs. The most common configurations are listed below.

#### Release Build with Qt and Tests (Recommended for Development)

```powershell
cmake -B build -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_BUILD_TESTS=ON -DQUANTUMVERSE_USE_QT=ON -DCMAKE_PREFIX_PATH="C:\Qt\6.11.1\msvc2022_64"
```

| Flag | Purpose |
|------|---------|
| `-B build` | Output build directory |
| `-DCMAKE_BUILD_TYPE=Release` | Optimized build |
| `-DQUANTUMVERSE_BUILD_TESTS=ON` | Include test targets |
| `-DQUANTUMVERSE_USE_QT=ON` | Build `quantumverse_qml` (Qt UI) |
| `-DCMAKE_PREFIX_PATH="C:\Qt\6.11.1\msvc2022_64"` | Find Qt 6.11.1 |

#### Debug Build with AddressSanitizer

```powershell
cmake -B build -A x64 -DCMAKE_BUILD_TYPE=Debug -DQUANTUMVERSE_BUILD_TESTS=ON -DQUANTUMVERSE_USE_ASAN=ON -DQUANTUMVERSE_USE_QT=ON -DCMAKE_PREFIX_PATH="C:\Qt\6.11.1\msvc2022_64"
```

| Flag | Purpose |
|------|---------|
| `-A x64` | 64-bit architecture (Windows) |
| `-DCMAKE_BUILD_TYPE=Debug` | Debug symbols, no optimization |
| `-DQUANTUMVERSE_USE_ASAN=ON` | Enable AddressSanitizer |

#### Release Build with UndefinedBehaviorSanitizer (Linux)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_BUILD_TESTS=ON -DQUANTUMVERSE_USE_UBSAN=ON -DQUANTUMVERSE_USE_QT=ON -DCMAKE_PREFIX_PATH="/path/to/Qt/6.11.1"
```

#### ThreadSanitizer (Linux)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DQUANTUMVERSE_BUILD_TESTS=ON -DQUANTUMVERSE_USE_TSAN=ON -DQUANTUMVERSE_USE_QT=ON -DCMAKE_PREFIX_PATH="/path/to/Qt/6.11.1"
```

#### Coverage Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DQUANTUMVERSE_BUILD_TESTS=ON -DQUANTUMVERSE_USE_QT=ON -DQUANTUMVERSE_USE_COVERAGE=ON
```

#### Fuzz Targets (Clang only)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DQUANTUMVERSE_BUILD_TESTS=ON -DQUANTUMVERSE_USE_FUZZER=ON -DQUANTUMVERSE_USE_QT=ON -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
```

**Note:** Fuzzer mode requires Clang. GCC produces a CMake `FATAL_ERROR`.

#### Core-Only Fast Iteration (no GUI, no tests)

```powershell
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -- /maxcpucount
ctest -C Release --output-on-failure -E "(qml|imgui|vr|onnx)"
```

### Step 2: Build

#### Standard Build (all generators)

```powershell
cmake --build build --config Release --parallel
```

| Flag | Purpose |
|------|---------|
| `--config Release` | Build configuration (Windows only; ignored on single-config generators) |
| `--parallel` | Use all CPU cores |
| `--parallel 8` | Limit to 8 parallel jobs |

#### Ninja Generator (faster incremental builds)

```powershell
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_BUILD_TESTS=ON -DQUANTUMVERSE_USE_QT=ON -DCMAKE_PREFIX_PATH="<Qt6_DIR>"
cmake --build build --config Release --parallel 2
```

#### Visual Studio Generator

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_BUILD_TESTS=ON -DQUANTUMVERSE_USE_QT=ON
cmake --build build --config Release --parallel
```

### Step 3: Deploy Qt Runtime (Windows)

Before running from the build directory or packaging, deploy Qt DLLs and plugins:

```powershell
cmake --build build --config Release --target deploy_qt_runtime
```

This target:
- Runs `windeployqt` with `--compiler-runtime --no-translations`
- Copies MSVC CRT DLLs (`msvcp140.dll`, `vcruntime140.dll`, `concrt140.dll`) from the detected VC redist directory
- Makes the application portable to clean Windows installs

### Step 4: Run Tests

See [Test Commands](#5-test-commands) for detailed options.

```powershell
cd build
ctest -C Release --output-on-failure
```

### Step 5: Run Application

```powershell
# Headless benchmark
build\Release\quantumverse_qml.exe --headless --frames 3 --metric schwarzschild

# Software rendering (no GPU required)
build\Debug\quantumverse_qml.exe --software-rendering --frames 1 --metric schwarzschild
```

### Step 6: Package for Distribution

See [Deployment & Packaging](#6-deployment--packaging).

---

## 5. Test Commands

### Framework

- **C++ Unit Tests:** Custom `QV_CHECK` / `QV_CHECK_NEAR` macros in `tests/test_assert.h` (NDEBUG-safe; throw `std::runtime_error` instead of aborting)
- **Qt Test:** `Qt6::Test` for QML/UI tests
- **Runner:** `ctest` (CTest)

### Run All Tests (Release)

```powershell
cd build
ctest -C Release --output-on-failure
```

| Flag | Purpose |
|------|---------|
| `-C Release` | Use Release configuration |
| `--output-on-failure` | Show full output for failed tests |

### Run Specific Test

```powershell
ctest -C Release -R MercuryPrecessionTest --output-on-failure
```

### Run Physics Validation Suite

```powershell
ctest -C Release -R "MercuryPrecessionTest|LightDeflectionTest|GravitationalRedshiftTest|FrameDraggingTest" --output-on-failure
```

### Run Tests Excluding Canaries (CI)

```powershell
# Canary exclusions are listed in .github/workflows/_canary-exclusions.txt
$exclusions = Get-Content .github/workflows/_canary-exclusions.txt
ctest -C Release -E $exclusions --output-on-failure
```

**Canary tests** (non-blocking in CI):
```
GridDeformationTest|SoftwareTourHeadlessTest|UI4DTest|PerformanceGateTest|
QMLPerformanceBaseline|ViewportStateTest|ViewportContentTest|VisualRegressionTest|
AnimationTimingTest|GLStrictAuditTest|PerformanceRegressionTest
```

### Headless Tests (Linux)

```bash
EXCLUSIONS=$(cat ../.github/workflows/_canary-exclusions.txt)
xvfb-run -a ctest -C Release -E "$EXCLUSIONS" --output-on-failure
```

### VR Tests (no OpenXR runtime required)

```powershell
ctest -C Release -R "SharedSessionTest|SignalingClientTest|MultiUserServerTest" --output-on-failure
```

### Shader Validation

```powershell
cmake --build build --config Release --target validate_shaders
```

**Requirement:** `glslangValidator` must be on `PATH`.

---

## 6. Deployment & Packaging

### Method A: `deploy.bat` (Windows)

```powershell
.\deploy.bat
```

**Output:** `deploy\windows\quantumverse_qml.exe`

This script:
1. Detects build type from `build/Release/quantumverse_qml.exe`
2. Cleans `deploy/windows/`
3. Copies executable
4. Runs `windeployqt` with `--release --no-translations --no-system-d3d-compiler --compiler-runtime --qmldir src`
5. Copies `models/` directory (ONNX checkpoints) if present
6. **Removes `opengl32sw.dll`** to force native OpenGL
7. Copies icon SVGs to `deploy/windows/icons/`
8. Copies QML module files to `deploy/windows/qml/QuantumVerse/`
9. Creates `qt.conf`:
   ```ini
   [Paths]
   Prefix = .
   Plugins = plugins
   Qml2Imports = qml
   ```

### Method B: CMake Install + CPack (Windows)

```powershell
# Configure with install prefix
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_BUILD_TESTS=ON -DQUANTUMVERSE_USE_QT=ON -DCMAKE_PREFIX_PATH="<Qt6_DIR>" -DCMAKE_INSTALL_PREFIX="F:\syyyy\install"

# Build
cmake --build build --config Release

# Deploy Qt runtime
cmake --build build --config Release --target deploy_qt_runtime

# Install to staging tree
cmake --install build --config Release --prefix install

# Package as ZIP
cd install/bin
7z a ../../quantumverse_v3.9.0_win64.zip quantumverse_qml.exe *.dll *.conf qt.conf plugins/ qml/ data/

# Package as NSIS installer
cpack -B build -G NSIS --config build/CPackConfig.cmake
```

**CPack Configuration:**
- **Generators:** `ZIP;NSIS`
- **Package name:** `QuantumVerse`
- **Version:** `3.9.0` (must match `CPACK_PACKAGE_VERSION` in `CMakeLists.txt`)
- **NSIS:** Creates Start Menu shortcut to `$INSTDIR\bin\quantumverse_qml.exe`

---

## 7. Runtime Modes

### Executable Locations

```
build/Release/quantumverse_qml.exe   # Release build
build/Debug/quantumverse_qml.exe     # Debug build
deploy/windows/quantumverse_qml.exe  # Deployed package
```

### CLI Flags

| Flag | Purpose |
|------|---------|
| `--headless` | Run without window; render to offscreen FBO |
| `--frames <N>` | Number of frames to render (required for headless) |
| `--metric <name>` | Select metric (`schwarzschild`, `kerr`, `frw`, etc.) |
| `--software-rendering` | Use Qt software raster backend; implies `--frames 1` |
| `--gl-strict` | Enable strict OpenGL error checking; exits non-zero on any GL error |
| `--start-tour <name>` | Start specific educational tour (e.g. `black_hole_basics`) |
| `--mock` | Bypass `QGuiApplication` entirely (for headless CI tests without platform plugin) |

### Example Commands

```powershell
# Standard headless benchmark
build\Release\quantumverse_qml.exe --headless --frames 3 --metric schwarzschild

# Software rendering (no GPU required; CI / headless)
build\Debug\quantumverse_qml.exe --software-rendering --frames 1 --metric schwarzschild

# GL strict audit
build\Release\quantumverse_qml.exe --headless --frames 10 --gl-strict

# Start educational tour
build\Release\quantumverse_qml.exe --start-tour black_hole_basics
```

### Environment Variables

| Variable | Value | Purpose |
|----------|-------|---------|
| `QT_QPA_PLATFORM` | `offscreen` | Use Qt offscreen platform plugin (CI/headless) |
| `QSG_RHI_BACKEND` | `d3d11`, `opengl`, `vulkan`, `software` | Select Qt Scene Graph backend (Windows) |
| `QT_OPENGL` | *deprecated* | **No-op on Qt 6.2+**; do not use |
| `ASAN_OPTIONS` | `detect_leaks=1:symbolize=1:print_stacktrace=1` | AddressSanitizer tuning |
| `LSAN_OPTIONS` | `suppressions=.github/lsan.supp:print_suppressions=0` | LeakSanitizer suppressions |
| `UBSAN_OPTIONS` | `halt_on_error=1:print_stacktrace=1` | UBSan tuning |
| `TSAN_OPTIONS` | `suppressions=.github/tsan.supp:halt_on_error=0:second_deadlock_stack=1:history_size=4` | TSan tuning |

---

## 8. Platform-Specific Notes

### Windows / MSVC

- **Qt 6 backend switching:** `QT_OPENGL=angle` is a **no-op** on Qt 6.2+. Use `QSG_RHI_BACKEND` (`d3d11`, `opengl`, `vulkan`, `software`) instead.
- **MSVC runtime deployment:** `windeployqt --compiler-runtime` ships the VC++ redist installer, not raw DLLs. The CMake `deploy_qt_runtime` target and `deploy.bat` copy raw CRT DLLs directly from the VC redist directory.
- **qFatal exit codes:**
  - Release: `0xC0000409` (`STATUS_STACK_BUFFER_OVERRUN` via `__fastfail`)
  - Debug: `0x80000003` (`STATUS_BREAKPOINT` via `__debugbreak`)
  - Both indicate the same Qt init failure (`QGuiApplicationPrivate::init` / `QtPrivate::sizedFree`). Do not treat `0x80000003` as a distinct bug.
- **High DPI:** May require `Qt::AA_EnableHighDpiScaling` application attribute.
- **GL context on CI:** Windows runners have no usable GL driver. GL-dependent tests are demoted to canary status.

### Linux

- **Headless testing:** Use `xvfb-run -a` to provide a virtual X server.
- **Sanitizers:** GCC/Clang use `-fsanitize=address|undefined|thread` plus `-fno-omit-frame-pointer` / `-g`.
- **Assertions:** Tests must use `QV_CHECK` / `QV_CHECK_NEAR`; `assert()` is compiled out under `-DNDEBUG`.

### macOS

- **Qt backend:** Use `QSG_RHI_BACKEND=metal` or `opengl` as appropriate.
- **Codesigning:** Application bundles require codesigning before distribution.

---

## 9. Quick Reference

### Complete Developer Cycle (Windows)

```powershell
# 1. Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_BUILD_TESTS=ON -DQUANTUMVERSE_USE_QT=ON -DCMAKE_PREFIX_PATH="C:\Qt\6.11.1\msvc2022_64"

# 2. Build
cmake --build build --config Release --parallel

# 3. Deploy Qt runtime
cmake --build build --config Release --target deploy_qt_runtime

# 4. Run tests
cd build
ctest -C Release --output-on-failure

# 5. Run application
build\Release\quantumverse_qml.exe --headless --frames 3 --metric schwarzschild

# 6. Package
cpack -C Release
```

### Debug + ASan Cycle (Windows)

```powershell
# Configure
cmake -B build -A x64 -DCMAKE_BUILD_TYPE=Debug -DQUANTUMVERSE_BUILD_TESTS=ON -DQUANTUMVERSE_USE_ASAN=ON -DQUANTUMVERSE_USE_QT=ON -DCMAKE_PREFIX_PATH="C:\Qt\6.11.1\msvc2022_64"

# Build
cmake --build build --config Debug --parallel 2

# Test
cd build
ctest -C Debug --output-on-failure
```

### Linux Headless CI

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_BUILD_TESTS=ON -DQUANTUMVERSE_USE_QT=ON

# Build
cmake --build build --config Release --parallel

# Test (with xvfb for display)
EXCLUSIONS=$(cat .github/workflows/_canary-exclusions.txt)
cd build
xvfb-run -a ctest -C Release -E "$EXCLUSIONS" --output-on-failure
```

### Common Issues

| Issue | Solution |
|--------|----------|
| `quantumverse_qml` target missing | Add `-DQUANTUMVERSE_USE_QT=ON` to CMake configure |
| `Qt6 not found` | Set `-DCMAKE_PREFIX_PATH="C:\Qt\6.11.1\msvc2022_64"` |
| Build succeeds but app crashes on startup | Run with `QT_DEBUG_PLUGINS=1` to check plugin loading |
| Tests pass but app exits with `-1` | Check QML loading errors in stderr; verify `qrc:/main.qml` resource is compiled |
| GL tests hang on CI | Use `--software-rendering` or `--mock` flags; GL tests are canaries on headless CI |
| Long instrument names truncated in filters | `instrumentFilter` has `elide: Text.ElideRight`; widen panel or accept truncation |

---

## Appendix: CMake Targets Reference

| Target | Purpose |
|--------|---------|
| `quantumverse_qml` | Main Qt/QML application |
| `quantumverse_multi_user` | VR multi-user server library |
| `deploy_qt_runtime` | Copy Qt DLLs and plugins (Windows) |
| `validate_shaders` | Run `glslangValidator` on all shaders |
| `test_*` | Individual test executables |

---

## Appendix: Test Categories

| Category | Pattern | Description |
|----------|---------|-------------|
| Unit Tests | `tests/test_*.cpp` | Individual component tests |
| Integration Tests | `tests/test_integration*.cpp` | Cross-module tests |
| Validation Tests | `tests/test_*validation*.cpp` | GR validation (Mercury, light deflection, etc.) |
| Performance Tests | `tests/test_*benchmark*.cpp` | Performance benchmarks |
| Physics Tests | `tests/test_*gravity*.cpp` | Physics accuracy tests |
| VR Tests | `tests/unit/test_shared_session.cpp`, `test_signaling_client.cpp`, `test_multi_user_server.cpp` | Headless, no OpenXR required |
| Discovery Tests | `tests/discovery/test_*.cpp` | 6+ TDD checks per instrument |
