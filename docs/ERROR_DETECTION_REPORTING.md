# Error Detection & Reporting for QuantumVerse Simulator

This document describes the integrated error detection and reporting infrastructure for the QuantumVerse Simulator C++17/OpenGL project.

## Overview

The error detection system provides:
- **Static Analysis**: Clang-Tidy and Cppcheck for compile-time bug detection
- **Dynamic Analysis**: AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan) for runtime error detection
- **Test Coverage**: gcov/lcov for code coverage analysis
- **CI Integration**: GitHub Actions workflow for automated error detection

## Quick Start

### Local Usage

Run the error report generator:

```bash
# Linux/macOS
./scripts/generate_error_report.sh

# Windows
.\scripts\generate_error_report.bat

# Or using Python (cross-platform)
python scripts\generate_error_report.py

# Or using CMake
cmake -B build -DQUANTUMVERSE_GENERATE_REPORT=ON
cmake --build build --target report
```

### Using Existing Build

The script automatically detects existing builds. To use a specific build directory:

```bash
# Use Release build (recommended for Windows)
python scripts\generate_error_report.py --build-dir build/Release

# Use custom build directory
python scripts\generate_error_report.py --build-dir my_custom_build
```

### CMake Integration

Enable sanitizers and coverage in your build:

```bash
# Build with AddressSanitizer
cmake -B build -DQUANTUMVERSE_USE_ASAN=ON -DCMAKE_BUILD_TYPE=Debug

# Build with UndefinedBehaviorSanitizer
cmake -B build -DQUANTUMVERSE_USE_UBSAN=ON -DCMAKE_BUILD_TYPE=Debug

# Build with coverage
cmake -B build -DQUANTUMVERSE_USE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug

# Build with all error detection features
cmake -B build -DQUANTUMVERSE_USE_ASAN=ON -DQUANTUMVERSE_USE_UBSAN=ON -DQUANTUMVERSE_USE_COVERAGE=ON
```

## Static Analysis Tools

### Clang-Tidy

Modern C++ linter that detects:
- Bug patterns (use-after-free, memory leaks, null pointer dereferences)
- Performance issues
- Modern C++ style recommendations

```bash
# Install (Ubuntu/Debian)
sudo apt install clang-tidy

# Run
run-clang-tidy -p build -export-fixes=reports/clang-tidy-fixes.yaml
```

### Cppcheck

Comprehensive static analyzer for:
- Portability issues
- Undefined behavior
- Memory errors
- Style problems

```bash
# Install (Ubuntu/Debian)
sudo apt install cppcheck

# Run
cppcheck --enable=all --xml src/ 2> reports/cppcheck.xml
cppcheck-htmlreport --file=reports/cppcheck.xml --report-dir=reports/cppcheck-html
```

## Dynamic Analysis Tools

### AddressSanitizer (ASan)

Detects:
- Heap use-after-free
- Stack use-after-return
- Buffer overflows
- Memory leaks

```bash
# Build with ASan
cmake -B build -DCMAKE_CXX_FLAGS="-fsanitize=address -g" -DCMAKE_BUILD_TYPE=Debug

# Run tests
ASAN_OPTIONS=detect_leaks=1 ./build/test_dilaton
```

### UndefinedBehaviorSanitizer (UBSan)

Detects:
- Integer overflow
- Null pointer dereference
- Alignment issues
- Invalid enum values

```bash
# Build with UBSan
cmake -B build -DCMAKE_CXX_FLAGS="-fsanitize=undefined -g" -DCMAKE_BUILD_TYPE=Debug
```

## Test Coverage

### Linux/macOS (gcov + lcov)

```bash
# Build with coverage
cmake -B build -DCMAKE_CXX_FLAGS="--coverage -O0 -g" -DCMAKE_EXE_LINKER_FLAGS="--coverage"

# Run tests
cd build && ctest

# Generate coverage report
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

### Windows (OpenCppCoverage)

```powershell
# Run with OpenCppCoverage
OpenCppCoverage --sources src --export_type=html:coverage.html -- build\test_dilaton.exe
```

## CI/CD Integration

The `.github/workflows/error-detection-reporting.yml` workflow runs:

1. **Static Analysis Job**: Clang-Tidy and Cppcheck on all source files
2. **Sanitizer Tests Job**: Build and test with ASan/UBSan enabled
3. **Coverage Job**: Generate HTML coverage report
4. **Windows Memory Analysis**: Dr. Memory for Windows-specific memory issues

### Workflow Triggers

- Push to `main` or `develop` branches
- Pull requests to `main` or `develop`
- Weekly schedule (Sundays at 00:00 UTC)
- Manual dispatch with optional parameters

## Report Output

Reports are generated in the `reports/` directory:

| File | Description |
|------|-------------|
| `error_report_*.md` | Combined markdown error report with test count |
| `clang-tidy-fixes.yaml` | Clang-Tidy fixes in YAML format |
| `cppcheck.xml` | Cppcheck results in XML format |
| `asan.*` | AddressSanitizer log files |
| `coverage_html/` | HTML coverage report |
| `coverage.info` | Coverage data file |

### Sample Report

```markdown
# QuantumVerse Error Report

**Generated:** 2026-06-14T23:06:40Z

## Static Analysis (Clang-Tidy)
```
compile_commands.json not found, skipping clang-tidy
```

## Static Analysis (Cppcheck)
```
```

## Test Results
```
  test_anomaly_detection.exe: PASSED
  test_autodiff.exe: PASSED
  ...
  test_vr_multiplayer.exe: PASSED
```

## Summary
| Check | Status |
|-------|--------|
| Static Analysis (Clang-Tidy) | See section above |
| Static Analysis (Cppcheck) | See section above |
| Test Results | 49/49 passed |
```

## GR Validation Integration

The error detection system integrates with the GR validation tests:

| Test | Expected Value |
|------|-------------|
| Mercury perihelion | 43.0 arcsec/century |
| Eddington light deflection | 1.75 arcsec |
| Gravitational redshift | Δλ/λ = GM/(c²r) |
| Gravity Probe B | 39 mas/year |
| Nordtvedt parameter | \|ω-1\| < 0.003 |

## Best Practices

1. **Run before every release**: Execute the error report generator before tagging releases
2. **Fix warnings first**: Address all static analysis warnings before merging
3. **Monitor coverage**: Maintain >80% code coverage for core modules
4. **Check sanitizers**: Ensure no memory errors in debug builds
5. **Validate physics**: All GR validation tests must pass

## Troubleshooting

### Clang-Tidy not found
```bash
# Ubuntu/Debian
sudo apt install clang-tidy

# macOS
brew install llvm
```

### Cppcheck not found
```bash
# Ubuntu/Debian
sudo apt install cppcheck

# Windows (Chocolatey)
choco install cppcheck
```

### ASan/UBSan not working
Ensure you're using GCC or Clang (not MSVC) and the compiler supports sanitizers:
```bash
# Check compiler version
gcc --version  # Need GCC 4.8+
clang++ --version  # Need Clang 3.1+
```

### Coverage not generating
Make sure to:
1. Build with `--coverage` flag
2. Run the tests (they generate .gcda files)
3. Run lcov from the build directory