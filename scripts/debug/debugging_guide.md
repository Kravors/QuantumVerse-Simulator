# QuantumVerse Debugging Guide

This guide covers debugging techniques, tools, and workflows for the QuantumVerse Simulator.

## 1. Quick Start: Building with Sanitizers

### Windows (MSVC) - Debug Build

```powershell
# Debug build without sanitizers (recommended for MSVC)
cmake -B build_debug -DCMAKE_BUILD_TYPE=Debug -DQUANTUMVERSE_BUILD_TESTS=ON
cmake --build build_debug --config Debug --parallel
ctest --output-on-failure -C Debug
```

### Windows (Clang) - Sanitizers

```powershell
# Debug build with AddressSanitizer (static linking)
cmake -B build_asan -DCMAKE_BUILD_TYPE=Debug -DQUANTUMVERSE_USE_ASAN=ON -DQUANTUMVERSE_ASAN_STATIC=ON -DQUANTUMVERSE_BUILD_TESTS=ON
cmake --build build_asan --config Debug --parallel
ctest --output-on-failure -C Debug
```

### Linux/macOS

```bash
# Debug build with AddressSanitizer
cmake -B build_asan -DCMAKE_BUILD_TYPE=Debug -DQUANTUMVERSE_USE_ASAN=ON -DQUANTUMVERSE_ASAN_STATIC=ON -DQUANTUMVERSE_BUILD_TESTS=ON
cmake --build build_asan --parallel
ctest --output-on-failure
```

## 2. Available Sanitizers

| Sanitizer | Detects | CMake Option |
|-----------|---------|--------------|
| AddressSanitizer | Memory corruption, use-after-free, buffer overflows | `QUANTUMVERSE_USE_ASAN` |
| UndefinedBehaviorSanitizer | Integer overflow, null deref, alignment issues | `QUANTUMVERSE_USE_UBSAN` |
| ThreadSanitizer | Data races | `QUANTUMVERSE_USE_TSAN` |
| MemorySanitizer | Uninitialized reads | `QUANTUMVERSE_USE_MSAN` |

## 3. Running Tests

```bash
# Run all tests
cd build_debug && ctest --output-on-failure -C Debug

# Run specific test
ctest -R test_mercury_precession --output-on-failure -C Debug

# Run with verbose output
ctest -R test_geodesic -V -C Debug
```

## 4. Debugging Tools

### GDB/LLDB (Linux/macOS)

```bash
# Run test under debugger
gdb --args build_asan/tests/test_geodesic

# With sanitizer
ASAN_OPTIONS=detect_leaks=1 ./build_asan/tests/test_geodesic
```

### Visual Studio Debugger (Windows)

1. Open the test executable in VS
2. Set breakpoints in the source code
3. Run with diagnostic tools enabled

### Time-Travel Debugging

**Linux (rr):**
```bash
rr record ./build_asan/tests/test_geodesic
rr replay
```

**Windows (WinDbg TTD):**
```
.cdbcmd /c "reload /f /l /t build_asan/tests/test_geodesic"
```

## 5. Static Analysis

```bash
# Clang-Tidy
run-clang-tidy -p build_debug src/

# Clang Static Analyzer
scan-build cmake -B build_debug -DCMAKE_BUILD_TYPE=Debug
scan-build --status-bugs cmake --build build_debug
```

## 6. Profiling

Use Tracy profiler (already integrated):
```cpp
TRACE_SCOPE("MyFunction");
TRACE_EVENT("key", value);
```

## 7. Common Issues

### ASan Tests Failing on Windows

**Problem:** Tests fail with exit code `0xC0000135` (DLL_NOT_FOUND) when running under ctest with MSVC's ASan.

**Root Cause:** MSVC's AddressSanitizer requires Visual Studio runtime DLLs that are not available in the test environment.

**Solutions:**

1. **Use MSVC without ASan** (recommended for Windows development):
   ```powershell
   cmake -B build_debug -DCMAKE_BUILD_TYPE=Debug -DQUANTUMVERSE_BUILD_TESTS=ON
   cmake --build build_debug --config Debug
   ctest --output-on-failure -C Debug
   ```

2. **Use Clang compiler on Windows** with static linking:
   ```powershell
   cmake -B build_asan -DCMAKE_BUILD_TYPE=Debug -DQUANTUMVERSE_USE_ASAN=ON -DQUANTUMVERSE_ASAN_STATIC=ON -DQUANTUMVERSE_BUILD_TESTS=ON
   cmake --build build_asan --config Debug
   ```

3. **Run tests directly** (bypass ctest):
   ```powershell
   .\build_asan\Debug\test_dilaton.exe
   ```

### UBSan not detecting issues

Enable stack trace:
```bash
UBSAN_OPTIONS=print_stacktrace=1 ./test_geodesic
```