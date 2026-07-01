# QuantumVerse Invariants Documentation

This document describes the critical invariants that must be maintained throughout the codebase to ensure correctness and safety.

## 1. Physics Invariants

### 1.1 Metric Tensor Invariants

| Invariant | Description | Location |
|-----------|-------------|----------|
| Lorentz Signature | All valid metrics must have signature (-, +, +, +) | [`src/spacetime/MetricTensor.h`](src/spacetime/MetricTensor.h) |
| Symmetry | Metric tensor must be symmetric: g[μ][ν] = g[ν][μ] | [`MetricTensor::isLorentzian()`](src/spacetime/MetricTensor.h) |
| Determinant | Non-singular metrics must have non-zero determinant | [`MetricTensor::determinant()`](src/spacetime/MetricTensor.h) |
| Positive Definite Spatial | For Schwarzschild/Kerr, g_rr > 0 outside horizon | [`MetricTensor::schwarzschild()`](src/spacetime/MetricTensor.h) |

### 1.2 Geodesic Invariants

| Invariant | Description | Location |
|-----------|-------------|----------|
| Energy Conservation | E = -g_tt * dt/dλ must be constant along geodesics | [`src/physics/GeodesicIntegrator.h`](src/physics/GeodesicIntegrator.h) |
| Angular Momentum | L = g_φφ * dφ/dλ must be constant for stationary spacetimes | [`src/physics/GeodesicIntegrator.h`](src/physics/GeodesicIntegrator.h) |
| Timelike Condition | g_μν * u^μ * u^ν < 0 for physical particles | [`src/physics/GeodesicIntegrator.h`](src/physics/GeodesicIntegrator.h) |

### 1.3 Curvature Invariants

| Invariant | Description | Location |
|-----------|-------------|----------|
| Kretschmann Scalar | R_μνρσ * R^μνρσ ≥ 0 everywhere | [`src/physics/CurvatureCalculator.h`](src/physics/CurvatureCalculator.h) |
| Ricci Scalar | R = g^μν * R_μν (contraction) | [`src/physics/CurvatureCalculator.h`](src/physics/CurvatureCalculator.h) |
| Bianchi Identity | ∇_μ * G^μν = 0 (contracted Bianchi identity) | [`src/physics/CurvatureCalculator.h`](src/physics/CurvatureCalculator.h) |

## 2. Memory Safety Invariants

### 2.1 Smart Pointer Invariants

| Invariant | Description | Location |
|-----------|-------------|----------|
| No Raw new/delete | All allocations use `std::unique_ptr` or `std::shared_ptr` | All source files |
| RAII Compliance | All resources follow RAII pattern | All source files |
| No Dangling Pointers | All pointers are validated before use | [`src/utils/CrashHandler.h`](src/utils/CrashHandler.h) |

### 2.2 Buffer Safety Invariants

| Invariant | Description | Location |
|-----------|-------------|----------|
| Array Bounds | All array access checked against size | [`src/math/Vector4D.h`](src/math/Vector4D.h) |
| Null Checks | All pointers checked for null before dereference | All source files |
| String Safety | No buffer overflows in string operations | All source files |

## 3. Concurrency Invariants

### 3.1 Thread Safety

| Invariant | Description | Location |
|-----------|-------------|----------|
| Atomic Operations | Shared state uses `std::atomic` | [`src/vr/MultiUserServer.h`](src/vr/MultiUserServer.h) |
| Mutex Protection | All shared data protected by mutex | [`src/vr/MultiUserServer.h`](src/vr/MultiUserServer.h) |
| No Race Conditions | ThreadSanitizer clean | All multi-threaded code |

## 4. Rendering Invariants

### 4.1 OpenGL/DirectX Safety

| Invariant | Description | Location |
|-----------|-------------|----------|
| FBO Complete | Framebuffer completeness checked before use | [`src/rendering/CurvatureRenderer.h`](src/rendering/CurvatureRenderer.h) |
| Shader Valid | Shader compilation status checked | [`src/rendering/CurvatureRenderer.h`](src/rendering/CurvatureRenderer.h) |
| Context Current | OpenGL context current before GL calls | [`src/rendering/GLDebug.h`](src/rendering/GLDebug.h) |

## 5. Input Validation Invariants

### 5.1 Parameter Ranges

| Invariant | Description | Location |
|-----------|-------------|----------|
| Mass > 0 | Black hole mass must be positive | [`src/spacetime/MetricTensor.h`](src/spacetime/MetricTensor.h) |
| Spin ≤ Mass | Kerr spin parameter: |a| ≤ M | [`src/spacetime/MetricTensor.h`](src/spacetime/MetricTensor.h) |
| r > 2M | Outside event horizon for Schwarzschild | [`src/spacetime/MetricTensor.h`](src/spacetime/MetricTensor.h) |

## 6. Security Invariants

### 6.1 Input Sanitization

| Invariant | Description | Location |
|-----------|-------------|----------|
| No Injection | All inputs sanitized against injection attacks | [`src/utils/ConfigManager.h`](src/utils/ConfigManager.h) |
| Path Traversal | File paths validated against traversal attacks | [`src/data/SpacetimeExporter.h`](src/data/SpacetimeExporter.h) |
| Integer Overflow | All arithmetic checked for overflow | All source files |

## 7. Testing Invariants

### 7.1 Test Coverage

| Invariant | Description | Location |
|-----------|-------------|----------|
| Line Coverage | ≥ 95% line coverage for all modules | [`cmake/CoverageCheck.cmake`](cmake/CoverageCheck.cmake) |
| Branch Coverage | ≥ 90% branch coverage for all modules | [`cmake/CoverageCheck.cmake`](cmake/CoverageCheck.cmake) |
| Mutation Score | ≥ 80% mutation score for core modules | [`.mull.yml`](.mull.yml) |

## 8. Validation Tests

The following validation tests must pass before any release:

| Test | Expected Value | Tolerance |
|------|---------------|-----------|
| Mercury Perihelion | 43.0 arcsec/century | ±0.1 |
| Eddington Light Deflection | 1.75 arcsec | ±0.01 |
| Gravitational Redshift | Δλ/λ = GM/(c²r) | ±1e-6 |
| Gravity Probe B | 39 mas/year | ±1 mas/year |
| Nordtvedt Parameter | \|ω-1\| < 0.003 | ±0.001 |

## 9. Code Review Checklist

Before merging any PR, verify:

- [ ] All public symbols have Doxygen documentation
- [ ] All new code uses smart pointers (no raw `new`/`delete`)
- [ ] All new code is `noexcept` where appropriate
- [ ] All new code uses `constexpr` where appropriate
- [ ] All invariants are maintained
- [ ] All tests pass with sanitizers enabled
- [ ] All new code has unit tests
- [ ] All new code has property-based tests (if applicable)
- [ ] All new code has integration tests (if applicable)
- [ ] All new code has performance benchmarks (if applicable)