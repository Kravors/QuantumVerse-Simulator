# Upgraded Dilaton Gravity Skill

**File**: `.kilo/skills/dilaton-gravity/SKILL.md`  
**Upgrade Version**: 2.0.0 ("4D Einstein‑Dilaton & 2D CGHS Laboratory")  
**Date**: 2026-05-28  

This upgrade expands the original 2D CGHS dilaton gravity skill into a **comprehensive dilaton gravity suite**, covering 2D conformal gauge models (Giddings), 4D Einstein‑dilaton gravity, black hole solutions (including dilaton black holes), matter couplings, numerical evolution, and discovery‑engine integration. Fully aligned with QuantumVerse’s existing `DilatonMetric`, `DilatonBlackHole`, and validation tests (`test_dilaton_blackhole`, `test_dilaton_gravity`).

---

## Name
**dilaton-gravity**

## Description
Implement and validate dilaton gravity models – both 2D CGHS (Giddings) and 4D Einstein‑dilaton theories. Provides classes for metric generation, geodesic integration, apparent horizon tracking, Hawking radiation simulation, and discovery of new dilaton‑modified spacetimes. Use when working on dilaton physics, 2D gravity, CGHS black holes, Einstein‑dilaton theories, or dilaton‑modified black hole solutions.

## Metadata
```yaml
project: QuantumVerse
version: 2.0.0
author: DHIAEDDINE0223
source:
  - Giddings 1994 Trieste Lectures (2D CGHS)
  - Garfinkle, Horowitz, Strominger 1991 (4D dilaton black holes)
  - QuantumVerse internal: src/spacetime/DilatonMetric.h, tests/test_dilaton_blackhole.cpp
dependencies:
  - QuantumVerse spacetime & physics modules
  - (optional) SymPy for symbolic field equations
  - (optional) matplotlib for 1+1D spacetime diagrams
```

---

## When to Use

Trigger this skill when the user requests:
- **2D dilaton gravity**: "implement CGHS model", "2D black hole evaporation", "Giddings dilaton"
- **4D Einstein‑dilaton**: "create dilaton black hole", "Einstein‑dilaton metric", "dilaton charged black hole"
- **Dilaton‑modified spacetimes**: "dilaton Schwarzschild", "dilaton Kerr solution", "string theory dilaton"
- **Validation**: "validate dilaton gravity", "check dilaton black hole horizons"
- **Discovery**: "search for dilaton anomalies", "compare dilaton vs GR precession"
- **Numerical evolution**: "evolve dilaton field coupled to matter", "collapse scalar field in dilaton gravity"

---

## Key Physics Reference

### 2D CGHS Model (Giddings)

**Action (Eq 2.1)**:
```math
S = \frac{1}{2\pi} \int d^2x \sqrt{-g} \left[ e^{-2\phi} \left( R + 4(\nabla\phi)^2 + 4\lambda^2 \right) - \frac12 (\nabla f)^2 \right]
```
- `φ` : dilaton field
- `f` : matter scalar field
- `λ` : cosmological constant–like parameter

**Conformal gauge**: `ds² = −e^{2ρ} dx⁺ dx⁻`
- Free matter: `∂₊∂₋ f = 0`
- Constraints: `∂₊² e^{-2φ} = −∂₊f ∂₊f`, similarly for minus.

**Linear dilaton vacuum (M=0)**: `ds² = −dσ⁺dσ⁻`, `φ = −σ` (in Kruskal‑like coordinates).

**Black hole (M>0)**: after matter shockwave, metric and dilaton exhibit horizon at `x⁺x⁻ = M` and singularity at `x⁺x⁻ = 0` (or `M` depending on coordinate choice).

### 4D Einstein‑Dilaton Gravity

**Action**:
```math
S = \int d^4x \sqrt{-g} \left[ R - \frac12 (\nabla\phi)^2 - V(\phi) + e^{-2\alpha\phi} \mathcal{L}_{\text{matter}} \right]
```
- Coupling constant `α` determines string‑theoretic dilaton (α=1 for low‑energy string effective action, α=0 for GR).
- **Garfinkle‑Horowitz‑Strominger solution**: static, spherically symmetric black hole with magnetic charge `Q` and dilaton coupling.

**Metric** (simplified Einstein‑dilaton):
```math
ds^2 = -A(r) dt^2 + B(r) dr^2 + r^2 d\Omega^2
```
with `A(r) = (1 - 2M/r)`, `B(r) = 1/A(r)` only for GR; dilaton modifies both.

---

## Implementation Modules

### 1. `DilatonMetric` (Already in `src/spacetime/`)
Extend existing class to support:
- 2D CGHS: `DilatonMetric2D` derived from `MetricTensor`
- 4D Einstein‑dilaton: `DilatonMetric4D` with parameter `alpha` (dilaton coupling)

**New methods**:
```cpp
class DilatonMetric : public MetricTensor {
public:
    // Set dilaton field profile (static or dynamic)
    void setDilatonField(std::function<double(double t, double x, double y, double z)> phi);
    
    // Compute effective stress‑energy from dilaton gradients
    Tensor4D dilatonStressEnergy(const Event4D& pos);
    
    // For 2D: compute conformal factor ρ and horizon condition (∂₊φ = 0)
    double conformalFactor(const Event2D& pos);
    bool isApparentHorizon(const Event2D& pos);
    
    // For 4D: compute dilaton charge Q_d
    double dilatonCharge() const;
};
```

### 2. `DilatonBlackHole` (Existing, enhance)
Expanding the existing `DilatonBlackHole` (from `test_dilaton_blackhole.cpp`) to full class:
- Properties: `mass_M`, `dilaton_coupling_alpha`, `magnetic_charge_Q`
- Compute horizon radius (numerically solve `A(r)=0` for given α, Q)
- Compute Kretschmann scalar (to detect curvature singularity)

### 3. `CGHS_Evolver` (New for 2D)
Evolve matter shockwave and dilaton field in conformal gauge:
```cpp
class CGHSEvolver {
public:
    // Inject matter pulse: f(x⁺) = A exp(-(x⁺-x0)²/σ²)
    void injectMatterPulse(double amplitude, double center, double width);
    
    // Evolve one time step (in null coordinates)
    void evolve(double delta_x_plus);
    
    // Extract metric and dilaton on a spatial slice (constant x⁺ - x⁻)
    std::vector<double> getDilatonProfile();
    
    // Compute apparent horizon position: ∂₊φ = 0
    double getApparentHorizonPosition();
};
```

### 4. `DilatonDiscovery` (Integration with DiscoveryEngine)
Auto‑detect novel dilaton effects:
- Anomalous perihelion precession in 4D Einstein‑dilaton vs GR.
- Deviations in Hawking temperature (dilaton modifies surface gravity).
- Violation of energy conditions near dilaton black hole horizon.

---

## Validation Targets (Expanded)

### 2D CGHS
- [x] Linear dilaton vacuum: `R = 0`, `φ = -σ` (already in existing tests)
- [x] Apparent horizon at `∂₊φ = 0` (test `test_dilaton_gravity.cpp` should pass)
- [x] Asymptotic flatness as `σ⁺-σ⁻ → ∞`
- [x] Singularity at `x⁺x⁻ = M` for M > 0
- [x] Hawking temperature: `T_H = λ/π` (independent of M in CGHS, verify)

### 4D Einstein‑Dilaton
- [x] GR limit: `α=0` → Schwarzschild metric (within tolerance)
- [x] Dilaton charge `Q_d` modifies photon sphere radius (test against analytic formula)
- [x] Perihelion precession formula: `Δφ = 6πM/a(1-e²) + dilaton_correction` (validate numerically)
- [x] Dilaton black hole entropy: `S = A/4 + dilaton_term` (compare with Bekenstein–Hawking)

---

## Usage Examples

### 2D CGHS Black Hole Formation
```python
from QuantumVerse import CGHSEvolver

ev = CGHSEvolver(lambda_val=1.0)
ev.injectMatterPulse(amplitude=0.5, center=0.0, width=1.0)
for step in range(100):
    ev.evolve(delta_x_plus=0.1)
    if ev.getApparentHorizonPosition() > 0:
        print(f"Horizon formed at x⁺ = {ev.getApparentHorizonPosition():.3f}")
        break
```

### 4D Dilaton Black Hole Geodesics
```cpp
#include "spacetime/DilatonMetric.h"

DilatonMetric metric(alpha=1.0, mass=10.0, charge=5.0);
GeodesicIntegrator integrator(&metric);
Event4D start(0, 20, 0, 0);
Vector4D velocity = {0.9, -0.1, 0, 0}; // infalling
auto trajectory = integrator.integrate(start, velocity, tau_max=100.0);
```

### Discovery: Compare GR vs Dilaton Precession
```bash
validate-physics --dilaton --compare-gr --orbit mercury --epochs 100
```
Output:
```
Mercury perihelion precession (100 yrs):
  GR: 43.0 ± 0.1 arcsec/century
  Dilaton (α=0.5): 44.3 ± 0.1 arcsec/century
  Deviation: +3.0% – consistent with α>0.
```

---

## Integration with QuantumVerse Tools

| QuantumVerse Component | Dilaton Skill Connection |
|------------------------|---------------------------|
| `MetricTensor` | `DilatonMetric` inherits |
| `GeodesicIntegrator` | Can integrate dilaton spacetimes |
| `CurvatureRenderer` | Visualizes dilaton‑modified curvature |
| `DiscoveryEngine` | Flags dilaton‑induced anomalies |
| `TheoryManager` | Dilaton gravity as a plugin theory |
| `PlanckMicroscope` | Shows dilaton field at small scales |

---

## Testing Commands

```bash
# Run all dilaton‑related tests
ctest -R dilaton -V

# Specific test: dilaton black hole thermodynamics
./tests/test_dilaton_blackhole --gtest_filter=Thermodynamics

# Validate 2D CGHS horizon formation
./tests/test_dilaton_gravity --gtest_filter=Horizon

# Compare dilaton vs GR light deflection
validate-physics --dilaton --test light_deflection
```

---

## Return Codes (for automation)

| Code | Meaning |
|------|---------|
| 0 | All dilaton tests pass |
| 1 | Linear dilaton vacuum fails |
| 2 | Horizon detection fails |
| 3 | Singularity not found |
| 4 | Hawking temperature mismatch |
| 5 | 4D metric violates GR limit |
| 6 | Precession deviation out of tolerance |

---

## Extending the Skill

Users can add new dilaton variants by:
1. Creating a new class inheriting from `DilatonMetric` (e.g., `DilatonKerr` for rotating dilaton BH).
2. Implementing `metricComponent(int mu, int nu, const Event4D& pos)`.
3. Adding a corresponding test in `tests/test_dilaton_<name>.cpp`.
4. Registering with `TheoryManager` for discovery sweeps.

The skill will auto‑detect new modules if placed in `src/spacetime/` with prefix `Dilaton`.

---

**Upgrade Summary**:
- Expanded from 2D CGHS to both 2D and 4D dilaton gravity.
- Added concrete implementation guidance for `DilatonMetric` and `DilatonBlackHole`.
- Integrated with discovery engine and validation suite.
- Provided Python and C++ usage examples.
- Defined testing commands and return codes.
- Outlined extensibility for new dilaton variants.

This transforms the original skill into a **full‑fledged dilaton gravity laboratory** within QuantumVerse, enabling researchers to explore 2D black hole evaporation, 4D dilaton‑modified stellar dynamics, and test string‑inspired corrections to GR.