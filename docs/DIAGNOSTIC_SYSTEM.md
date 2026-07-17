# QuantumVerse Diagnostic System

## Overview

The QuantumVerse Diagnostic System provides comprehensive error detection and reporting capabilities for the C++17/OpenGL physics simulation codebase.

## CMake Build Options

| Option | Description |
|--------|-------------|
| `QUANTUMVERSE_USE_ASAN` | Enable AddressSanitizer for memory leak detection |
| `QUANTUMVERSE_USE_UBSAN` | Enable UndefinedBehaviorSanitizer |
| `QUANTUMVERSE_USE_FUZZER` | Build fuzzing targets (requires Clang) |
| `QUANTUMVERSE_ENABLE_TRACING` | Enable performance tracing instrumentation |
| `QUANTUMVERSE_RR_RECORD` | Enable time-travel debugging support (rr on Linux, WinDbg TTD on Windows) |
| `QUANTUMVERSE_USE_COVERAGE` | Enable code coverage instrumentation |

## Components

### 1. Static Analysis

**Tools:**
- **Clang-Tidy**: Modern C++ linter with bug pattern detection
- **Cppcheck**: Portability and undefined behavior analysis

**Usage:**
```bash
# Configure with compile commands
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Run clang-tidy
run-clang-tidy -p build src/

# Run cppcheck
cppcheck --enable=all --inconclusive src/
```

### 2. Dynamic Analysis (Sanitizers)

**AddressSanitizer (ASan)** detects:
- Use-after-free
- Buffer overflows
- Memory leaks

**UndefinedBehaviorSanitizer (UBSan)** detects:
- Integer overflow
- Null pointer dereference
- Alignment issues

**Usage:**
```bash
cmake -B build_san -DQUANTUMVERSE_USE_ASAN=ON -DQUANTUMVERSE_USE_UBSAN=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build_san
ctest --test-dir build_san --output-on-failure
```

### 3. Performance Baseline Test

Located in [`tests/test_performance_baseline.cpp`](tests/test_performance_baseline.cpp), measures 7 core physics operations:

| Operation | Description |
|-----------|-------------|
| `Vector4D::lorentzianDot` | Lorentzian inner product (100k calls) |
| `Vector4D::normEuclidean` | Euclidean norm calculation (100k calls) |
| `Matrix4x4::multiply` | 4x4 matrix multiplication (10k calls) |
| `Event4D::intervalSquared` | Spacetime interval calculation (100k calls) |
| `AutoDiff::arithmetic` | Automatic differentiation operations (100k calls) |
| `MetricTensor::schwarzschild` | Schwarzschild metric computation (10k calls) |
| `GeodesicIntegrator::integrate` | Geodesic integration (100 calls) |

**Usage:**
```bash
# Build and run
cmake -B build -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_BUILD_TESTS=ON
cmake --build build
./build/test_performance_baseline

# Output format: PIPELINE: name calls=N avg=Nus total=Nus
```

### 4. Regression Guard & Baseline Management

The [`scripts/analyze_traces.py`](scripts/analyze_traces.py) script compares performance against a golden baseline:

```bash
# Compare against baseline (5% threshold)
python scripts/analyze_traces.py performance.log report.html --compare-baseline --threshold 5

# Update baseline
python scripts/analyze_traces.py performance.log --save-baseline --baseline-path tests/baselines/performance_baseline.json

# Read from stdin
./test_performance_baseline | python scripts/analyze_traces.py - report.html --compare-baseline
```

**Baseline file:** [`tests/baselines/performance_baseline.json`](tests/baselines/performance_baseline.json)

**Nightly updates:** The `.github/workflows/nightly-baseline-update.yml` workflow automatically updates the baseline daily.

### 5. Tracing System

Located in [`src/diagnostics/Trace.h`](src/diagnostics/Trace.h), provides:

- **Scoped tracing**: `TRACE_SCOPE("function_name")` for entry/exit timing
- **Event tracing**: `TRACE_EVENT("event_name")` for one-shot events
- **Correlation IDs**: Track data flow across modules with `CORRELATION_SET(id)`
- **Pipeline metrics**: `PipelineStage` for throughput measurement

**Example:**
```cpp
void GeodesicIntegrator::integrate() {
    TRACE_SCOPE("GeodesicIntegrator::integrate");
    CORRELATION_SET("geodesic-12345");
    
    // ... integration code ...
    
    TRACE_EVENT("step_completed");
}
```

### 6. Data Integrity

Located in [`src/diagnostics/DataIntegrity.h`](src/diagnostics/DataIntegrity.h), provides:

- **CRC32 checksums** for detecting silent data corruption
- **Integrity verification** for MetricTensor and Event4D

**Example:**
```cpp
uint32_t checksum = computeMetricChecksum(metric);
// Later...
if (!verifyMetricIntegrity(metric, checksum)) {
    ERROR_LOG("Metric data corrupted!");
}
```

### 7. API Contract Testing

Located in [`tests/fuzz_api_contract.cpp`](tests/fuzz_api_contract.cpp), validates:

- Pre/post conditions
- Exception handling
- Edge cases (NaN, Inf, zero values)
- Symmetry invariants

**Build with libFuzzer:**
```bash
cmake -B build -DQUANTUMVERSE_USE_FUZZER=ON -DCMAKE_CXX_COMPILER=clang++
cmake --build build --target fuzz_api_contract
./build/fuzz_api_contract -max_total_time=300
```

### 8. Time-Travel Debugging

**Linux (rr):**
```bash
cmake -B build -DQUANTUMVERSE_RR_RECORD=ON
rr record ./build/quantumverse_qml
rr replay
```

**Windows (WinDbg TTD):**
```cmd
# Use tttracer.exe from WinDbg preview
tttracer.exe -launch cmake --build build --config Release
# Replay in WinDbg with trace loaded
```

### 9. Automated Diagnostic Script

Run [`scripts/diagnose_all.py`](scripts/diagnose_all.py) for a complete analysis:

```bash
python scripts/diagnose_all.py
# Or quick mode:
python scripts/diagnose_all.py --quick
```

Generates `diagnostic_report.html` with all results.

## CI Workflows

### error-detection-reporting.yml

Triggers on push/PR to main/develop branches. Runs:
- Static analysis (Clang-Tidy, Cppcheck)
- Sanitizer tests (ASan, UBSan)
- Performance baseline check
- Code coverage
- Fuzz testing
- Windows build & packaging

### nightly-baseline-update.yml

Runs daily to update `performance_baseline.json` and opens a PR if changes are detected.

## Packaging & Deployment

**CPack configuration** in [`CMakeLists.txt`](CMakeLists.txt):

```bash
# Build release
cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Package
cd build && cpack -C Release
# Generates: QuantumVerse-0.1.1-win64.zip and QuantumVerse-0.1.1.exe (NSIS installer)
```

## Test Status

| Phase | Status | Tests |
|-------|--------|-------|
| Phase 1 - Stabilise Test Suite | ✅ Complete | 62/62 passing |
| Phase 2 - Dynamic Analysis | ✅ Complete | Sanitizer job in CI |
| Phase 3 - Strategic Instrumentation | ✅ Complete | Tracing option added |
| Phase 4 - Performance Regression Guard | ✅ Complete | Baseline comparison working |
| Phase 5 - Deep-Dive Debugging | ✅ Complete | RR record option added |

## Performance Impact

- **Release builds**: No tracing overhead (macros can be disabled)
- **Debug builds**: ~5-10% overhead for full tracing
- **Sanitized builds**: 2-3x slowdown (expected for memory checking)

## Troubleshooting

### "report_runtime_error.cpp not found"

This is a Visual Studio debugger message when stepping into system code. Disable source server support in VS options.

### ASan DLL not found

On Windows, copy `clang_rt.asan_dynamic-x86_64.dll` to the build directory or use `QUANTUMVERSE_ASAN_STATIC=ON`.

### Performance regression detected

If a legitimate change causes >5% slowdown:
1. Run with `--save-baseline` to update the golden reference
2. The nightly workflow will automatically propose the update via PR