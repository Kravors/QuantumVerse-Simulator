# QuantumVerse Workspace Rules

These rules apply to **every mode and every prompt** in this workspace.
They capture the essential standards, conventions, and discipline required
to develop the QuantumVerse Simulator correctly.

---

## 1. Language & Communication

- **Primary language**: English for all code, comments, and documentation.
- **Responses**: Match the user's language. English is the default.

---

## 2. Code Standards (C++17)

### Mandatory
- **Use C++17** exclusively. `std::optional`, `std::variant`, `if constexpr` are preferred.
- **`#pragma once`** in every header file.
- **Smart pointers only**: `std::unique_ptr`, `std::shared_ptr`. **NO** raw `new`/`delete`.
- **RAII** for all resource management.
- **`constexpr`** where possible; **`noexcept`** on non‑throwing functions.
- **Doxygen** on all public symbols: `@brief`, `@param`, `@return`, `@note`.
- **Header‑first development**: write the `.h` to define the interface before the `.cpp`.

### Project Structure
- New code goes under `src/<module>/` where `<module>` is one of:
  `spacetime`, `physics`, `rendering`, `ui4d`, `discovery`, `quantumgravity`, `data`, `ml`, `math`.
- Tests go in `tests/`. Every new feature must have at least one test.
- Examples go in `examples/`.

---

## 3. Physics Rules

- **Lorentz signature**: `(−, +, +, +)` for 4D spacetime; `(−, +)` for 2D dilaton models.
- **Reuse existing machinery**: always use `MetricTensor` for Christoffel/Riemann/Ricci/Kretschmann.
  Do not re‑invent tensor calculus.
- **Geodesics**: use `GeodesicIntegrator` (adaptive RK4) or validate any custom integrator.
- **Conservation laws**: always test energy and angular momentum conservation in stationary spacetimes.
- **New modified gravity**: add as a `TheoryPlugin` subclass and register with `TheoryManager`.
- **Anomalies**: link to curvature invariants (Kretschmann, Ricci, Weyl) and flag anything >5σ.

---

## 4. Validation Discipline

Before claiming any new result or "discovery":
1. **Build and test** the full suite (`/build-and-test`).
2. **Validate physics** (`/validate-physics`): Mercury precession, light deflection, redshift,
   frame‑dragging, neutron star TOV.
3. **Reject** any result that fails the historical tests.

### Mandatory Validation Targets
| Test | Expected |
|------|----------|
| Mercury perihelion | 43.0 arcsec/century |
| Eddington light deflection | 1.75 arcsec |
| Gravitational redshift | Δλ/λ = GM/(c²r) |
| Gravity Probe B | 39 mas/year |
| Nordtvedt parameter | \|ω‑1\| < 0.003 |

---

## 5. Giddings‑Lecture Derived Rules

When implementing 2D dilaton gravity or Hawking radiation from the 1994 Trieste lectures:
- **Conformal gauge**: ds² = −e²ᵖ dx⁺ dx⁻; all equations must be expressed in this gauge first.
- **Linear dilaton vacuum** (M=0): ds² = −dσ⁺dσ⁻, φ = −σ. Check R = 0.
- **Black hole** (M>0): horizon at x⁺x⁻ = M, e²ᵠ|horizon = 1/M.
- **Hawking flux**: asymptotic 〈:T₋₋:〉 = 1/48 (2D units, λ=1).
- **Temperature**: T = 1/(2π) (2D), T = κ/(2π) = 1/(8πM) (4D Schwarzschild).
- **Entropy**: S = A/4.
- **RST endpoint**: semiclassical approximation fails at σ⁻_NS (Eq 5.26).

## 6. Build & Test Rules

- Always reconfigure CMake before building when new files are added.
- Never commit code that hasn't passed `ctest --output-on-failure`.
- Always check for compiler warnings; aim for zero warnings.

---

## 7. Discovery Safety

- No "new physics" claim may be made without passing the GR validation suite.
- Discovery logs must record the parameter values, observed vs expected curvature,
  and whether the anomaly survives cross‑check.
- Before accepting an anomaly, rule out numerical artifacts or coordinate singularities.

---

## 8. Git Hygiene

- Commit messages in English; short summary, then body.
- Reference task/issue numbers when applicable (e.g., `Task 2.1: add ONNX inference`).
- Do not commit build artifacts, `.onnx` models, or large `.h5` datasets.

---

## 9. 4D UI & Rendering

- Always use the two‑slice projection model: 4D → 3D → 2D.
- New discovery tools must appear in the DiscoveryPanel with an anomaly timeline.
- Maintain linkability: an event selected in one viewport must highlight in all others.