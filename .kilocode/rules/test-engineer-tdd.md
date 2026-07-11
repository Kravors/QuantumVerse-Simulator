# TDD Workflow for QuantumVerse

**Version**: 0.1.1  
**Last Updated**: 2026-06-27  
**Project**: QuantumVerse Simulator

When implementing a new physics component:

1. **Write the test first** in `tests/unit/test_new_component.cpp`
2. **Run the test** - it should fail
3. **Implement minimal code** to pass the test
4. **Refactor** while keeping tests green
5. **Add integration tests** for interactions
6. **Run validation tests** against analytical solutions
7. **Benchmark** performance
8. **Commit** with clear message referencing the test

## Physics-Specific Guidelines

### GR Validation Tests

All physics components must pass the following validation tests:

| Test | Expected Value | Tolerance |
|------|---------------|-----------|
| Mercury perihelion | 43.0 arcsec/century | ±0.1 |
| Eddington light deflection | 1.75 arcsec | ±0.01 |
| Gravitational redshift | Δλ/λ = GM/(c²r) | ±1e-6 |
| Frame dragging (Gravity Probe B) | 39 mas/year | ±1 mas/year |
| Nordtvedt parameter | \|ω-1\| < 0.003 | ±0.001 |

### Test File Organization

```
tests/
├── unit/                    # Unit tests for individual components
│   └── test_new_component.cpp
├── integration/             # Cross-module integration tests
├── validation/              # GR validation tests
├── performance/             # Performance benchmarks
└── physics/                 # Physics accuracy tests
```

### Example TDD Cycle

```cpp
// 1. Write failing test
TEST(MetricTensorTest, SchwarzschildExterior) {
    MetricTensor metric = MetricTensor::schwarzschild(1.0, 10.0, 0.0, 0.0);
    EXPECT_NEAR(metric.determinant(), -1.0, 1e-10);
}

// 2. Run test (should fail)
// 3. Implement minimal code
// 4. Run test (should pass)
// 5. Add edge cases
// 6. Add integration test
```