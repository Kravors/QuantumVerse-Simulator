# QuantumVerse Developer Guide

## Project Structure

```
src/
├── spacetime/      # Event4D, MetricTensor, spacetime math
├── physics/        # GeodesicIntegrator, CurvatureCalculator
├── quantumgravity/ # CDT, Spin Foam, GFT, Causal Sets
├── discovery/      # AI discovery engines, anomaly detection
├── rendering/      # OpenGL rendering, ImGui UI
├── math/           # Vector4D, Matrix4x4, AutoDiff
└── data/           # LIGOAdapter, external data integration

tests/
├── unit/           # Unit tests
├── validation/     # GR validation tests
└── verification/   # CBMC harnesses

docs/
├── validation/     # Validation reports
└── user_manual/    # User documentation
```

## Build Instructions

```bash
# Linux/macOS
cmake -B build -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_BUILD_TESTS=ON
cmake --build build --parallel $(nproc)

# Windows
cmake -B build -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_BUILD_TESTS=ON
cmake --build build --config Release --parallel
```

## Running Tests

```bash
cd build
ctest --output-on-failure              # All tests
ctest -R Validation --output-on-failure  # Physics validation
ctest -R PropertyBased --output-on-failure  # Property tests
```

## Code Style

- C++17 standard
- `#pragma once` in headers
- Smart pointers only (`std::unique_ptr`, `std::shared_ptr`)
- RAII for resource management
- Doxygen comments on public symbols

## Static Analysis

```bash
# Run cppcheck
./scripts/run_cppcheck.sh

# Run clang-tidy (if available)
./scripts/run_clang_tidy.sh
```

## Adding New Tests

1. Create `tests/test_my_feature.cpp`
2. Add to `CMakeLists.txt`:
   ```cmake
   add_executable(test_my_feature tests/test_my_feature.cpp)
   target_link_libraries(test_my_feature PRIVATE dilaton)
   add_test(NAME MyFeatureTest COMMAND test_my_feature)
   ```
3. Run `ctest -R MyFeature`

## Diagnostic Layers

Run the full diagnostic pipeline:
```bash
./scripts/run_all_diagnostics.bat   # Windows
./scripts/run_all_diagnostics.sh    # Linux/macOS
```

This runs all 12 layers: pre-commit, static analysis, unit tests, integration, performance, memory, coverage, fuzzing, mutation, stress, security, and AI learning.