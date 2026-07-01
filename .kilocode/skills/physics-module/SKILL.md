# Upgraded Physics Module Creation Skill

**File**: `.kilo/skills/physics-module/SKILL.md`  
**Upgrade Version**: 3.0.0 ("Full‑Stack Physics Engineering")  
**Date**: 2026-05-28  

This upgrade transforms the basic module scaffolding into a **professional physics‑module engineering suite** supporting differentiable physics, neural surrogates, quantum gravity engines, discovery integration, automatic benchmarking, and cross‑language bindings.

---

## Name
**physics-module**

## Description
Scaffold a new C++17 physics module for QuantumVerse – header, implementation, and unit test – with Doxygen, RAII, and smart pointers. Supports advanced module types: **differentiable**, **neural surrogate**, **quantum gravity**, **discovery instrument**, and **theory plugin**. Automatically generates benchmarks, Python bindings (pybind11), ONNX export stubs, and validation hooks. Use when creating new simulation units like `DilatonMetric`, `HawkingCalculator`, `EvaporationIntegrator`, `NeuralGeodesicSurrogate`, or `SpinFoamVertex`.

## Metadata
```yaml
project: QuantumVerse
version: 3.0.0
author: DHIAEDDINE0223
dependencies:
  - CMake >= 3.20
  - catch2 (or google-test) for unit tests
  - (optional) pybind11 for Python bindings
  - (optional) ONNX Runtime for neural exports
  - (optional) CUDA for GPU kernels
```

---

## When to Use

Trigger this skill when the user asks to:
- Create a new physics component: "create module", "new physics class", "scaffold DilatonMetric"
- Add a differentiable module: "add differentiable geodesic integrator"
- Create a neural surrogate: "scaffold a Neural ODE model"
- Add a quantum gravity engine: "create a new spin foam amplitude calculator"
- Build a discovery instrument: "add anomaly detection module"
- Extend the theory plugin system: "register new modified gravity theory"
- Generate Python bindings for an existing module: "add pybind11 wrapper for MetricTensor"

---

## Step 1: Gather Enhanced Module Information

Ask the user for:

| Field | Options / Example | Required |
|-------|-------------------|----------|
| **Module name** (CamelCase) | `DilatonMetric`, `NeuralGeodesicSurrogate` | ✅ |
| **Submodule directory** | `spacetime`, `physics`, `rendering`, `ui4d`, `discovery`, `quantumgravity`, `data`, `ml`, `math`, `exotic`, `topological`, `holography` | ✅ |
| **Brief description** | Used for Doxygen `@brief` | ✅ |
| **Module type** | `standard`, `differentiable`, `neural`, `quantum`, `discovery`, `theory_plugin` | ✅ |
| **Dependencies** | Existing headers (e.g., `MetricTensor.h`, `GeodesicIntegrator.h`) | Optional |
| **Header‑only or .cpp?** | `header-only`, `separate` | Optional |
| **Generate Python bindings?** | `yes` / `no` | Optional |
| **Generate ONNX export stub?** | `yes` / `no` (for neural modules) | Optional |
| **Generate CUDA kernel stub?** | `yes` / `no` (for performance‑critical modules) | Optional |
| **Auto‑register with DiscoveryEngine?** | `yes` / `no` (for anomaly detectors) | Optional |
| **Add to TheoryManager?** | `yes` / `no` (for modified gravity plugins) | Optional |
| **Include benchmark test?** | `yes` / `no` | Optional |

---

## Step 2: Create Advanced Header File

Path: `src/<submodule>/<ModuleName>.h`

The header template now includes:
- Full Doxygen comments (module, class, methods, parameters, return values, exceptions)
- PIMPL idiom with `std::unique_ptr`
- Deleted copy constructors, explicit move constructors
- Support for Eigen vectors/matrices (if needed)
- Conditional `#ifdef` for CUDA/neural code
- Optional `__host__ __device__` annotations
- Optional `DIFFERENTIABLE` macro for auto‑diff registration

### Example Header (Differentiable Module)

```cpp
#pragma once

/**
 * @file <ModuleName>.h
 * @brief <brief description>
 * @ingroup <submodule>
 */

#include <cstddef>
#include <memory>
#include <vector>
#include <Eigen/Core>

#ifdef QUANTUMVERSE_USE_CUDA
#include <cuda_runtime.h>
#endif

namespace QuantumVerse {

/**
 * @class <ModuleName>
 * @brief <brief description>
 * 
 * @details Longer description. This module implements <physics concept>.
 *          It satisfies RAII, uses smart pointers, and is thread‑safe
 *          (unless noted otherwise).
 * 
 * @note Instances of this class are move‑only.
 * @see RelatedClass
 */
class <ModuleName> {
public:
    /// Default constructor – initializes with default parameters.
    <ModuleName>();

    /// Parameterized constructor.
    explicit <ModuleName>(double parameter);

    /// Destructor – releases all resources.
    ~<ModuleName>();

    // Move semantics
    <ModuleName>(<ModuleName>&&) noexcept;
    <ModuleName>& operator=(<ModuleName>&&) noexcept;

    // Copy disabled
    <ModuleName>(const <ModuleName>&) = delete;
    <ModuleName>& operator=(const <ModuleName>&) = delete;

    /**
     * @brief Compute the main physical quantity.
     * @param input Input vector (e.g., event coordinates, metric parameters)
     * @return Output quantity (e.g., curvature scalar, geodesic endpoint)
     * @throws std::invalid_argument if input is invalid
     */
    [[nodiscard]] double compute(const Eigen::Vector4d& input) const;

    /**
     * @brief Compute gradient with respect to parameters (for differentiable modules).
     * @return Jacobian matrix (∂output/∂params)
     */
    [[nodiscard]] Eigen::MatrixXd gradient() const;

    /**
     * @brief Get current internal state (for debugging/serialization).
     */
    [[nodiscard]] nlohmann::json getState() const;

    /**
     * @brief Set internal state from JSON.
     */
    void setState(const nlohmann::json& state);

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace QuantumVerse
```

---

## Step 3: Create Implementation with Advanced Features

Path: `src/<submodule>/<ModuleName>.cpp`

The implementation now includes:
- PIMPL struct definition with all member variables
- RAII for GPU memory (if CUDA enabled)
- Automatic parameter validation
- Logging via project‑wide logger (spdlog or custom)
- Optional auto‑registration with `TheoryManager` or `DiscoveryEngine`

### Example Implementation Skeleton (CUDA + Differentiable)

```cpp
#include "<ModuleName>.h"
#include "spacetime/MetricTensor.h"
#include "logging/Logger.h"

#ifdef QUANTUMVERSE_USE_CUDA
#include "cuda/<ModuleName>_kernels.cuh"
#endif

namespace QuantumVerse {

struct <ModuleName>::Impl {
    double param_{0.0};
    mutable Eigen::MatrixXd cachedGradient_;
    
    Impl() = default;
    explicit Impl(double p) : param_(p) {}
    
    double computeImpl(const Eigen::Vector4d& input) const {
        // Core physics computation
        double result = 0.0;
        // ...
        return result;
    }
    
    Eigen::MatrixXd gradientImpl() const {
        // Differentiable calculation (if enabled)
        return Eigen::MatrixXd::Zero(1, 1);
    }
};

<ModuleName>::<ModuleName>() : pImpl(std::make_unique<Impl>()) {
    LOG_INFO("<ModuleName> default constructed");
}

<ModuleName>::<ModuleName>(double parameter) 
    : pImpl(std::make_unique<Impl>(parameter)) {
    LOG_INFO("<ModuleName> constructed with param = {}", parameter);
}

<ModuleName>::~<ModuleName>() = default;

<ModuleName>::<ModuleName>(<ModuleName>&&) noexcept = default;
<ModuleName>& <ModuleName>::operator=(<ModuleName>&&) noexcept = default;

double <ModuleName>::compute(const Eigen::Vector4d& input) const {
    if (input.hasNaN()) {
        throw std::invalid_argument("Input contains NaN");
    }
    return pImpl->computeImpl(input);
}

Eigen::MatrixXd <ModuleName>::gradient() const {
    return pImpl->gradientImpl();
}

nlohmann::json <ModuleName>::getState() const {
    return {{"param", pImpl->param_}};
}

void <ModuleName>::setState(const nlohmann::json& state) {
    pImpl->param_ = state.value("param", 0.0);
}

} // namespace QuantumVerse
```

---

## Step 4: Generate Unit Test with Catch2 (or GoogleTest)

Path: `tests/test_<snake_case>.cpp`

Test template includes:
- Basic construction
- Parameter validation
- Numerical correctness (against analytic solution or tolerance)
- Performance benchmark (if requested)

```cpp
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include "<ModuleName>.h"
#include <Eigen/Core>

using namespace QuantumVerse;

TEST_CASE("<ModuleName> construction", "[<ModuleName>]") {
    REQUIRE_NOTHROW(<ModuleName>());
    REQUIRE_NOTHROW(<ModuleName>(1.0));
}

TEST_CASE("<ModuleName> compute correctness", "[<ModuleName>]") {
    <ModuleName> module(2.0);
    Eigen::Vector4d input(1.0, 0.0, 0.0, 0.0);
    double result = module.compute(input);
    // Known analytic value: <value>
    REQUIRE(result == Approx(3.14159).epsilon(1e-6));
}

TEST_CASE("<ModuleName> gradient", "[<ModuleName>][differentiable]") {
    <ModuleName> module(2.0);
    auto grad = module.gradient();
    // Check finite differences
    // ...
}

TEST_CASE("<ModuleName> serialization", "[<ModuleName>]") {
    <ModuleName> original(3.14);
    auto state = original.getState();
    <ModuleName> restored;
    restored.setState(state);
    REQUIRE(original.compute(Eigen::Vector4d::Zero()) == 
            Approx(restored.compute(Eigen::Vector4d::Zero())));
}

#ifdef QUANTUMVERSE_BENCHMARK
#include <benchmark/benchmark.h>

static void BM_<ModuleName>Compute(benchmark::State& state) {
    <ModuleName> module;
    Eigen::Vector4d input = Eigen::Vector4d::Random();
    for (auto _ : state) {
        benchmark::DoNotOptimize(module.compute(input));
    }
}
BENCHMARK(BM_<ModuleName>Compute);
#endif
```

---

## Step 5: Generate Python Bindings (Optional)

If user selected `--pybind`, create `src/bindings/py_<ModuleName>.cpp`:

```cpp
#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include "<ModuleName>.h"

namespace py = pybind11;

PYBIND11_MODULE(<module_name>, m) {
    py::class_<QuantumVerse::<ModuleName>>(m, "<ModuleName>")
        .def(py::init<>())
        .def(py::init<double>())
        .def("compute", &QuantumVerse::<ModuleName>::compute)
        .def("gradient", &QuantumVerse::<ModuleName>::gradient)
        .def_property("state", 
            &QuantumVerse::<ModuleName>::getState,
            &QuantumVerse::<ModuleName>::setState);
}
```

Add to `CMakeLists.txt`:
```cmake
if(QUANTUMVERSE_BUILD_PYTHON_BINDINGS)
    pybind11_add_module(py_<module_name> src/bindings/py_<ModuleName>.cpp)
    target_link_libraries(py_<module_name> PRIVATE <module_name>)
endif()
```

---

## Step 6: Generate ONNX Export Stub (for Neural Modules)

If module type is `neural`, create `src/ml/export_<ModuleName>_onnx.py`:

```python
import torch
import torch.nn as nn

class <ModuleName>Torch(nn.Module):
    def __init__(self, input_dim=4, hidden_dim=128, output_dim=1):
        super().__init__()
        self.net = nn.Sequential(
            nn.Linear(input_dim, hidden_dim),
            nn.ReLU(),
            nn.Linear(hidden_dim, hidden_dim),
            nn.ReLU(),
            nn.Linear(hidden_dim, output_dim)
        )
    
    def forward(self, x):
        return self.net(x)

# Export to ONNX
model = <ModuleName>Torch()
dummy_input = torch.randn(1, 4)
torch.onnx.export(model, dummy_input, "models/<module_name>.onnx",
                  input_names=["input"], output_names=["output"],
                  dynamic_axes={"input": {0: "batch_size"}})
print("ONNX model saved to models/<module_name>.onnx")
```

Add C++ inference loader in `<ModuleName>.cpp`:
```cpp
#ifdef QUANTUMVERSE_USE_ONNX
#include <onnxruntime_cxx_api.h>
// Load model in Impl constructor
#endif
```

---

## Step 7: Auto‑Register with DiscoveryEngine (Optional)

If user selects `--register-discovery`, add registration code in the `.cpp` file:

```cpp
#include "discovery/DiscoveryEngine.h"

namespace {
    // Static initializer (called before main)
    struct AutoRegister {
        AutoRegister() {
            DiscoveryEngine::instance().registerModule(
                "<ModuleName>",
                []() -> std::unique_ptr<AnomalyDetector> {
                    return std::make_unique<<ModuleName>>();
                }
            );
        }
    } autoRegister;
}
```

---

## Step 8: Update CMakeLists.txt

Append to `src/<submodule>/CMakeLists.txt`:

```cmake
# <ModuleName> library
add_library(<module_name> STATIC
    src/<submodule>/<ModuleName>.cpp
)

target_link_libraries(<module_name>
    PUBLIC quantumverse_core
    PRIVATE Eigen3::Eigen
)

if(QUANTUMVERSE_USE_CUDA)
    target_compile_definitions(<module_name> PRIVATE QUANTUMVERSE_USE_CUDA)
    target_link_libraries(<module_name> PRIVATE cudart)
endif()

# Unit test
add_executable(test_<module_name> tests/test_<snake_case>.cpp)
target_link_libraries(test_<module_name> PRIVATE <module_name> Catch2::Catch2)
add_test(NAME test_<module_name> COMMAND test_<module_name>)

# Optional benchmark
if(QUANTUMVERSE_BUILD_BENCHMARKS)
    add_executable(bench_<module_name> tests/bench_<snake_case>.cpp)
    target_link_libraries(bench_<module_name> PRIVATE <module_name> benchmark::benchmark)
endif()
```

---

## Step 9: Build and Validate

After scaffolding, the skill automatically runs:

```bash
cd f:/syyyy/build
cmake .. -DQUANTUMVERSE_BUILD_TESTS=ON -DQUANTUMVERSE_BUILD_BENCHMARKS=ON
cmake --build . --config Release --parallel
ctest -R test_<module_name> --output-on-failure -V
```

If benchmarks are enabled:
```bash
./tests/bench_<module_name> --benchmark_format=json > bench_results.json
```

---

## Step 10: Generate Documentation

The skill optionally runs Doxygen on the new header:

```bash
doxygen -doxygen/doxygen.conf
```

Opens the generated HTML documentation for `<ModuleName>`.

---

## Customization Options Summary

| Flag (User Prompt) | Effect |
|--------------------|--------|
| `--type differentiable` | Adds `gradient()` method, automatic finite‑difference test |
| `--type neural` | Generates PyTorch stub, ONNX export script, ONNX Runtime loader |
| `--type quantum` | Includes headers from `quantumgravity/`, adds Monte Carlo seed support |
| `--type discovery` | Implements `AnomalyDetector` interface, auto‑registers with DiscoveryEngine |
| `--type theory_plugin` | Derives from `TheoryPlugin`, adds to `TheoryManager` registry |
| `--pybind` | Creates Python bindings with pybind11 |
| `--cuda` | Adds CUDA kernel `.cu` file and host‑side wrappers |
| `--benchmark` | Generates Google Benchmark test |
| `--register-discovery` | Static registration code for discovery engine |
| `--register-theory` | Static registration for theory manager |
| `--header-only` | Places implementation inside header, no .cpp file |

---

## Complete Example Session

```
User: "Create a differentiable scalar‑tensor gravity module named DilatonMetric in spacetime, with pybind11 bindings and discovery registration"

Assistant (via physics-module skill):
1. Gathering info...
2. Creating src/spacetime/DilatonMetric.h (with Doxygen, PIMPL, gradient method)
3. Creating src/spacetime/DilatonMetric.cpp (impl, auto‑register discovery)
4. Creating tests/test_dilaton_metric.cpp (Catch2, finite‑difference gradient check)
5. Creating src/bindings/py_DilatonMetric.cpp (pybind11)
6. Updating CMakeLists.txt in spacetime, bindings, and tests
7. Building and running tests...
   → test_dilaton_metric passed (12 assertions)
   → gradient finite‑difference relative error: 2.3e-7
8. Generating Doxygen documentation.
9. Python binding compiled: import dilaton_metric; dm = dilaton_metric.DilatonMetric(1.5)
10. Discovery registration confirmed: "DilatonMetric" now available in anomaly detector list.
```

---

## Return Codes

| Exit Code | Meaning |
|-----------|---------|
| 0 | Module created, built, and all tests passed |
| 1 | Invalid module name or submodule |
| 2 | Build failed (compilation errors) |
| 3 | Unit test failures |
| 4 | Missing dependency (e.g., pybind11 not found but requested) |
| 5 | Doxygen generation failed |

---

## Extensibility

The skill can be extended by adding new module type templates in `.kilo/skills/physics-module/templates/`. Each template is a directory containing `header.h.tmpl`, `impl.cpp.tmpl`, `test.cpp.tmpl`, and `CMakeLists.txt.tmpl`. The skill substitutes placeholders like `{{MODULE_NAME}}`, `{{SUBMODULE}}`, etc.

---

**Upgrade Summary**:
- Added support for 6 module types (standard, differentiable, neural, quantum, discovery, theory_plugin)
- Optional Python bindings, ONNX export, CUDA kernels
- Auto‑registration with `DiscoveryEngine` and `TheoryManager`
- Built‑in benchmarking and serialization
- Comprehensive error handling and logging
- Doxygen documentation generation

This transforms a simple file generator into a **full‑stack physics engineering assistant**.