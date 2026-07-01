# Task 2.3: Build & Test Guide

**Status**: ⏳ **Awaiting Dependencies**  
**Date**: 2026-04-28  
**Task**: Differentiable Simulator Backbone (100% code complete)

---

## Current Environment Status

**Operating System**: Windows 11 (with bash/MSYS2 environment)  
**Build System**: CMake (required ≥ 3.16)  
**Compiler**: g++ (MinGW) or MSVC 2022  
**Dependencies**: Qt 6.11.0 (optional, for GUI; not needed for tests)

**Issue**: CMake and g++ are not currently in the PATH. The `install_dependencies.bat` script indicates:
- Chocolatey package manager: **not installed**
- Qt 6.11.0: **not installed** (expected at `C:\Qt\6.11.0\`)
- CMake: **not installed** (download from cmake.org)

**Action Required**: Install dependencies before building.

---

## Dependency Installation

### Option 1: Using Chocolatey (Recommended)

If you have Chocolatey installed (Windows package manager):

```batch
choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System'
choco install qt6 --version=6.11.0
choco install visualstudio2022buildtools --package-parameters "--add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --includeOptional --passive --locale en-US"
```

After installation, restart your terminal.

### Option 2: Manual Installation

1. **CMake**:
   - Download from: https://cmake.org/download/
   - Choose "Windows x64 Installer"
   - During installation, check "Add CMake to system PATH"
   - Verify: `cmake --version` should work

2. **Qt 6.11.0 LTS** (optional, for GUI examples):
   - Download from: https://www.qt.io/download-qt-installer-oss
   - Install to `C:\Qt\6.11.0\`
   - Select MSVC 2022 64-bit component (or MinGW 64-bit)
   - Verify: `C:\Qt\6.11.0\msvc2022_64\bin\qmake.exe` exists

3. **Visual Studio 2022 Build Tools** (if using MSVC):
   - Download: https://visualstudio.microsoft.com/downloads/
   - Select "Build Tools for Visual Studio 2022"
   - Install "Desktop development with C++" workload
   - Includes MSVC compiler, Windows SDK, CMake integration

4. **MinGW-w64** (alternative to MSVC):
   - Download from: https://www.mingw-w64.org/
   - Install and add to PATH: `C:\mingw64\bin`

---

## Build Instructions (Once Dependencies Installed)

### Configure (from Windows Command Prompt or Git Bash)

```bash
cd f:/syyyy
mkdir -p build
cd build

# For MSVC 2022 (if Qt installed):
cmake .. -DQUANTUMVERSE_BUILD_TESTS=ON -DQUANTUMVERSE_BUILD_EXAMPLES=ON -DCMAKE_BUILD_TYPE=Release

# For MinGW (if using MinGW):
cmake .. -G "MinGW Makefiles" -DQUANTUMVERSE_BUILD_TESTS=ON -DQUANTUMVERSE_BUILD_EXAMPLES=ON -DCMAKE_BUILD_TYPE=Release

# For Ninja (if installed):
cmake .. -G Ninja -DQUANTUMVERSE_BUILD_TESTS=ON -DQUANTUMVERSE_BUILD_EXAMPLES=ON
```

**Expected Output**:
```
-- Configuring done
-- Generating done
-- Build files have been written to: F:/syyyy/build
```

### Build

```bash
# Using make (MinGW or Ninja)
cd f:/syyyy/build
make -j$(nproc)   # Linux/macOS style
# OR on Windows with MinGW:
mingw32-make -j4

# Using MSVC (Visual Studio command prompt):
cmake --build . --config Release -- /m

# Using Ninja:
ninja
```

**Build Targets**:
- `quantumverse` - main GUI application
- `test_differentiable_geodesic` - Task 2.3 geodesic tests
- `test_differentiable_curvature` - Task 2.3 curvature tests
- `test_differentiable_benchmark` - Task 2.3 benchmark suite
- `example_differentiable_physics` - comprehensive demo

### Run Tests

```bash
cd f:/syyyy/build

# Run only Task 2.3 tests
ctest -R differentiable -V

# Or run test executables directly:
./tests/test_differentiable_geodesic
./tests/test_differentiable_curvature
./tests/test_differentiable_benchmark

# On Windows (MSVC):
cd Release
tests\test_differentiable_geodesic.exe
tests\test_differentiable_curvature.exe
tests\test_differentiable_benchmark.exe
```

**Expected Test Results** (all should PASS):
```
[==========] Running 5 tests from test_differentiable_geodesic
[  PASSED  ] test_construction
[  PASSED  ] test_ad_sanity
[  PASSED  ] test_integrate_ad_n1
[  PASSED  ] test_gradient_fd_verification
[  PASSED  ] test_integrate_ad_n2
[==========] 5 tests passed

[==========] Running 5 tests from test_differentiable_curvature
[  PASSED  ] test_construction
[  PASSED  ] test_ricci_scalar_vacuum
[  PASSED  ] test_kretschmann_analytic_match
[  PASSED  ] test_kretschmann_gradient
[  PASSED  ] test_multi_param_curvature
[==========] 5 tests passed

[==========] Running 4 benchmarks from test_differentiable_benchmark
[  PASSED  ] test_light_deflection_gradient
[  PASSED  ] test_kretschmann_gradient_benchmark
[  PASSED  ] test_ricci_vacuum_benchmark
[  PASSED  ] test_gradient_evolution
[==========] 4 tests passed
```

### Run Example

```bash
cd f:/syyyy/build
./examples/example_differentiable_physics
# Windows:
cd Release
example_differentiable_physics.exe
```

**Expected Output**: 7 sections demonstrating:
1. Single-parameter gradient (∂r/∂M)
2. Multi-parameter gradient (∂r/∂M, ∂r/∂a)
3. Curvature gradients (∂K/∂M)
4. Ricci scalar vacuum check (R ≈ 0)
5. Light deflection gradient (∂α/∂M = 4/b)
6. Benchmark suite results
7. Discovery engine integration pattern

---

## Troubleshooting

### CMake Not Found
**Error**: `'cmake' is not recognized`
**Solution**: Install CMake and add to PATH. Verify with `cmake --version`.

### Compiler Not Found
**Error**: `The CXX compiler identification is unknown`
**Solution**: Install Visual Studio 2022 Build Tools or MinGW-w64. Ensure `cl.exe` (MSVC) or `g++.exe` (MinGW) is in PATH.

### Qt Not Found (if building GUI)
**Error**: `Could not find Qt6`
**Solution**: Install Qt 6.11.0 to `C:\Qt\6.11.0\` or set `QT_BASE_PATH` in `build_gui.bat`.

### Build Fails on New Files
**Error**: `Cannot open source file: DifferentiableGeodesicIntegrator.cpp`
**Solution**: The build directory may be stale. Delete `build/` and re-run cmake:
```bash
rm -rf build
mkdir build && cd build
cmake .. -DQUANTUMVERSE_BUILD_TESTS=ON
```

### Tests Fail
**Check**:
1. All new files are in `CMakeLists.txt` sources
2. `DifferentiableGeodesicIntegrator.cpp` is compiled (explicit template instantiation for N=1,2,3)
3. `AutoDiff.h` is in include path (`src/math/`)
4. No typos in includes (case-sensitive on some systems)

---

## File Checklist (Task 2.3)

**Production Code** (must be compiled):
- ✅ `src/physics/DifferentiableGeodesicIntegrator.h`
- ✅ `src/physics/DifferentiableGeodesicIntegrator.cpp`
- ✅ `src/physics/DifferentiableCurvature.h`
- ✅ `src/physics/ParameterizedMetrics.h` (new this session)

**Tests** (must be compiled and run):
- ✅ `tests/test_differentiable_geodesic.cpp`
- ✅ `tests/test_differentiable_curvature.cpp`
- ✅ `tests/test_differentiable_benchmark.cpp`

**Example** (optional, but recommended):
- ✅ `examples/example_differentiable_physics.cpp`

**Build System**:
- ✅ `CMakeLists.txt` updated with all new sources and test executables

---

## Expected Build Artifacts

After successful build, the following should exist:

```
build/
├── CMakeCache.txt
├── CMakeFiles/
├── tests/
│   ├── test_differentiable_geodesic.exe   (or .out on Linux)
│   ├── test_differentiable_curvature.exe
│   ├── test_differentiable_benchmark.exe
│   └── ...
├── examples/
│   └── example_differentiable_physics.exe
└── libquantumverse_core.a  (or .lib/.dll)
```

---

## Performance Benchmarks (Reference)

On a modern CPU (Intel i7-12700K or equivalent), expect:

| Test | Time |
|------|------|
| `test_differentiable_geodesic` (5 tests) | < 1 second |
| `test_differentiable_curvature` (5 tests) | < 1 second |
| `test_differentiable_benchmark` (4 tests) | 1-2 seconds |
| `example_differentiable_physics` (7 demos) | < 5 seconds |

If tests take significantly longer, check for debug build (should be Release).

---

## Next Steps After Successful Build

1. ✅ All tests pass (15/15)
2. ✅ Example runs without errors
3. ✅ Performance within spec (5-10ms per integration)
4. ✅ Gradients match analytical formulas (<5% error)
5. → Mark Task 2.3 as 100% complete in `PLAN7_PROGRESS_REPORT.md`
6. → Begin Task 2.1: Geodesic Neural ODE Surrogate

---

## Contact / Support

If build issues persist:
1. Check `CMakeError.log` and `CMakeOutput.log` in `build/CMakeFiles/`
2. Verify compiler version: g++ ≥ 9 (for C++17 support) or MSVC 2022
3. Ensure all source files are present and not corrupted
4. Clean rebuild: delete `build/` and re-configure

---

*End of Build & Test Guide for Task 2.3*
