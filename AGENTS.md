# QuantumVerse Agent Instructions

## Error Discovery & Resilience Workflows

### Running Advanced Verification

```bash
# Build with verification tests
cmake -B build -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_BUILD_TESTS=ON
cmake --build build --parallel

# Run CBMC verification tests
cd build && ctest -R CBMC --output-on-failure

# Run differential tests
ctest -R Differential --output-on-failure

# Run symbolic execution tests
ctest -R Symbolic --output-on-failure

# Run headless UI tests
./build/Release/quantumverse_imgui --headless --frames 10

# Run fuzz differential
./build/fuzz_differential -max_total_time=60
```

### Verification Components

| Component | Purpose | Location |
|-----------|---------|----------|
| CBMCVerification | Bounded model checking for critical invariants | `src/verification/CBMCVerification.h` |
| DifferentialTester | Compare multiple implementations | `src/verification/DifferentialTester.h` |
| RuntimeMonitor | Monitor physics invariants at runtime | `src/verification/RuntimeMonitor.h` |
| SymbolicExecutionHarness | Symbolic path exploration | `src/verification/SymbolicExecutionHarness.h` |

### Physics Invariants Monitored

1. **MetricTensor Symmetry**: g[μ][ν] = g[ν][μ]
2. **Lorentzian Signature**: (-,+,+,+) for 4D spacetime
3. **Negative Determinant**: det(g) < 0 for valid metrics
4. **Energy Conservation**: Total energy constant along trajectories
5. **Angular Momentum Conservation**: L conserved in central potentials
6. **Timelike Separation**: ds² < 0 for physical trajectories

### Adding New Verification Tests

1. Add verification functions to `CBMCVerification.h`
2. Add test entry to `runAll()` method
3. Register test in `CMakeLists.txt`:
   ```cmake
   add_executable(test_new_verification tests/test_new_verification.cpp)
   target_link_libraries(test_new_verification PRIVATE dilaton)
   add_test(NAME NewVerificationTest COMMAND test_new_verification)
   ```

### Custom Physics Invariant Checks

The following custom clang-tidy checks are enforced via `-warnings-as-errors=*`:
- `physics-invariant-metric-symmetry`: Ensures `MetricTensor` symmetry (g[μ][ν] = g[ν][μ])
- `physics-invariant-lorentz-signature`: Validates Lorentzian signature (-,+,+,+)
- `physics-invariant-negative-determinant`: Checks det(g) < 0 for valid metrics
- `physics-invariant-energy-conservation`: Verifies energy conservation in geodesics

### CI Pipeline

The advanced error discovery pipeline runs:
- **CBMC**: Bounded model checking (PR + nightly)
- **Differential Testing**: Algorithm comparison (every push)
- **Runtime Monitoring**: Sanitizer + invariant checks (every push)
- **Fuzz Testing**: Hybrid symbolic+fuzzing (PR + 24/7 continuous)
- **Chaos Testing**: Fault injection (weekly)

### Configuration Options

| Option | Default | Description |
|--------|---------|-------------|
| `QUANTUMVERSE_USE_ASAN` | OFF | AddressSanitizer |
| `QUANTUMVERSE_USE_TSAN` | OFF | ThreadSanitizer |
| `QUANTUMVERSE_USE_UBSAN` | OFF | UndefinedBehaviorSanitizer |
| `QUANTUMVERSE_USE_FUZZER` | OFF | libFuzzer targets |
| `QUANTUMVERSE_USE_BOOST_MULTIPRECISION` | OFF | Formal verification tests |
| `QUANTUMVERSE_ENFORCE_TIDY_ERRORS` | ON | Treat clang-tidy warnings as errors |
| `QUANTUMVERSE_USE_COVERAGE` | OFF | Enable code coverage instrumentation |

## Development Workflow

1. Write failing test first
2. Implement minimal code to pass
3. Run sanitizers: `cmake -DQUANTUMVERSE_USE_ASAN=ON`
4. Run verification: `ctest -R CBMC --output-on-failure`
5. Run differential tests: `ctest -R Differential --output-on-failure`
6. Commit with clear message referencing tests

## Success Metrics

| Metric | Target | Measurement |
|--------|--------|-------------|
| Line coverage | ≥ 95% | `lcov`/`gcovr` |
| Branch coverage | ≥ 90% | `lcov`/`gcovr` |
| Mutation score | ≥ 80% | Mull |
| Fuzzing coverage | ≥ 70% of code paths | libFuzzer |
| Security vulnerabilities | 0 critical/high | Semgrep, CodeQL |
| CBMC violations | 0 | CBMC harnesses |

## Phase 0: Headless UI & Integration Tests (COMPLETED)

### Status: ✅ Complete

### Changes Made:
1. **Upgraded UI framework** - Dear ImGui 1.92.8 + ImPlot 0.17
2. **Multi-viewport support** - Docking, viewports, high-DPI scaling
3. **Theme system** - Dark/Light/HighContrast themes in `src/ui4d/ImGuiBackend.cpp`
4. **Plotting integration** - `src/ui4d/PlotPanel.cpp` for curvature/anomaly plots
5. **Headless UI test** - `tests/integration/test_headless_ui.cpp`
6. **Updated CMakeLists.txt** - Built ImGui/ImPlot/GLFW/Glad from third_party
7. **Property-based tests** - `tests/unit/test_property_based.cpp` for Vector4D/Matrix4x4 invariants
8. **UI automation + screenshot** - `tests/ui/test_ui_automation.cpp` with stb_image_write

### Test Results:
```
Test #1: HeadlessUITest ...................   Passed
Test #2: Vector4DTest .....................   Passed
Test #3: Matrix4x4Test ....................   Passed
Test #4: PropertyBasedTest ................   Passed
Test #5: UIAutomationTest .................   Passed
Test #6: MercuryPrecessionTest ............   Passed
Test #7: LightDeflectionTest ..............   Passed
Test #8: GravitationalRedshiftTest .........   Passed
Test #9: FrameDraggingTest ................   Passed
Test #10: CDTTest ..........................   Passed
Test #11: SpinFoamTest .....................   Passed
Test #12: SpacetimeTest ....................   Passed
Test #13: GeodesicTest .....................   Passed
```
All 13 UI tests pass in 1.90 seconds.
Core validation tests: 27 passing.
**Total: 40 passing tests**

### CI Integration:
- Cross-platform matrix (Linux/Windows/macOS) in `.github/workflows/pr-diagnostics.yml`
- Sanitizer builds (ASan/TSan) for nightly runs
- Coverage check with 95% line / 90% branch thresholds
- All validation components complete

### Running Tests:
```bash
# Run all tests
cd build && ctest -C Release --output-on-failure

# Run headless UI specifically
./build/Release/quantumverse_imgui.exe --headless --frames 100

# Run UI automation with screenshot
./build/Release/test_ui_automation.exe --screenshot test_screenshot.png

# Run property-based tests
./build/Release/test_property_based.exe

# Run benchmarks (requires Google Benchmark)
cmake -DQUANTUMVERSE_USE_BENCHMARK=ON ..
./build/Release/benchmark_physics
```

### Running Fuzzing (requires libFuzzer/Clang):
```bash
# Build fuzzer
cmake -DQUANTUMVERSE_USE_FUZZER=ON -DCMAKE_CXX_COMPILER=clang++ ..
./build/fuzz_geodesic_differential -max_total_time=60
```

## Automated Root-Cause Analysis

```bash
# Analyze test failure
python3 scripts/analyze_root_cause.py build/TestResults.log TestName

# Run with git bisect
python3 scripts/analyze_root_cause.py build/TestResults.log TestName --bisect
```