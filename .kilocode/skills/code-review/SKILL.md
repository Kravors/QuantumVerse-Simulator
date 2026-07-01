
---

### 7. `.kilo/skills/code-review/SKILL.md`

```markdown
---
name: code-review
description: Review QuantumVerse code against C++17 standards, Doxygen documentation, RAII/smart pointers, and physics correctness. Use when the user says "review", "check code", or "code quality".
metadata:
  project: QuantumVerse
---

# Code Review

## When to Use
When the user asks to review code, check quality, or verify standards compliance.

## Process

### 1. Identify Scope
Ask user: specific files? All changes since last commit? Entire submodule?

### 2. C++17 Compliance
For each file check:
- No raw `new`/`delete` (smart pointers only)
- `#pragma once` present
- `constexpr` where possible
- `noexcept` on non-throwing functions
- `std::optional`, `std::variant` used appropriately

### 3. Doxygen Documentation
- Every file has `@file` comment
- Every public class has `@brief`
- Every public method has `@param`/`@return`

### 4. Physics Correctness
- Lorentz signature respected: (-,+,+,+) for 4D
- Reuses existing `MetricTensor` for curvature
- Uses `GeodesicIntegrator` or validates custom integrator
- Conservation laws checked (energy, angular momentum)

### 5. Test Coverage
- Each new class has corresponding test
- Edge cases + happy path covered
- Analytical validation where available

### 6. Report
- Violations (critical)
- Warnings
- Physics validation results
- Summary counts