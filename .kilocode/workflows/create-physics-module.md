# Workflow: create-physics-module

Scaffold a new physics module (header, implementation, unit test) for QuantumVerse,
following C++17, Doxygen, RAII and smart‑pointer rules.

## Step 1: Gather Module Information

Ask the user for:

1. **Module name** (CamelCase, e.g. `DilatonMetric`, `HawkingCalculator`).
2. **Submodule directory** (choose from `spacetime`, `physics`, `rendering`, `ui4d`, `discovery`, `quantumgravity`, `data`, `ml`, `math`).
3. **Brief description** (used for Doxygen `@brief`).
4. **Dependencies** (existing headers to include, e.g. `MetricTensor.h`).
5. **Header‑only or .cpp needed?**

Validate:
- Name is a valid C++ identifier.
- Submodule directory exists under `src/`.

## Step 2: Create Header File

Create `src/<sub>/<ModuleName>.h`:

```cpp
#pragma once

/**
 * @file <ModuleName>.h
 * @brief <brief description>
 */

#include <cstddef>
#include <memory>

namespace QuantumVerse {

/**
 * @class <ModuleName>
 * @brief <brief description>
 * @note RAII-compliant, uses smart pointers exclusively.
 */
class <ModuleName> {
public:
    <ModuleName>();
    ~<ModuleName>();

    <ModuleName>(const <ModuleName>&) = delete;
    <ModuleName>& operator=(const <ModuleName>&) = delete;
    <ModuleName>(<ModuleName>&&) noexcept;
    <ModuleName>& operator=(<ModuleName>&&) noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace QuantumVerse