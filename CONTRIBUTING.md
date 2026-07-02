# Contributing to QuantumVerse Simulator

Thank you for your interest in contributing to QuantumVerse Simulator! This document provides guidelines and instructions for contributing.

## Development Workflow

1. Write failing test first
2. Implement minimal code to pass the test
3. Run sanitizers: `cmake -DQUANTUMVERSE_USE_ASAN=ON`
4. Run verification: `ctest -R CBMC --output-on-failure`
5. Run differential tests: `ctest -R Differential --output-on-failure`
6. Commit with clear message referencing tests

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

## Physics Validation Requirements

Before claiming any new result or "discovery":
1. Build and test the full suite
2. Validate physics: Mercury precession, light deflection, redshift, frame-dragging, neutron star TOV
3. Reject any result that fails the historical tests

### Mandatory Validation Targets

| Test | Expected |
|------|----------|
| Mercury perihelion | 43.0 arcsec/century |
| Eddington light deflection | 1.75 arcsec |
| Gravitational redshift | Δλ/λ = GM/(c²r) |
| Gravity Probe B | 39 mas/year |
| Nordtvedt parameter | \|ω-1\| < 0.003 |

## Commit Messages

- Use English for all code, comments, and documentation
- Write clear, descriptive commit messages
- Reference task/issue numbers when applicable (e.g., `Task 2.1: add ONNX inference`)
- Do not commit build artifacts, `.onnx` models, or large `.h5` datasets

## CI Integration

The advanced error discovery pipeline runs:
- **CBMC**: Bounded model checking (PR + nightly)
- **Differential Testing**: Algorithm comparison (every push)
- **Runtime Monitoring**: Sanitizer + invariant checks (every push)
- **Fuzz Testing**: Hybrid symbolic+fuzzing (PR + 24/7 continuous)
- **Chaos Testing**: Fault injection (weekly)

## Code Coverage Requirements

| Metric | Target |
|--------|--------|
| Line coverage | ≥ 95% |
| Branch coverage | ≥ 90% |
| Mutation score | ≥ 80% |

## Questions?

Open an issue or contact the maintainers.