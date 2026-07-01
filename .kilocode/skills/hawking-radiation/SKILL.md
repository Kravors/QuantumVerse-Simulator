# Upgraded Hawking Radiation Skill

**File**: `.kilo/skills/hawking-radiation/SKILL.md`  
**Upgrade Version**: 2.0.0 (“Advanced Quantum Black Hole Evaporation”)  
**Date**: 2026-05-28  

This upgrade transforms the original Hawking radiation skill into a **comprehensive toolkit for computing and simulating black hole evaporation** in both 2D and 4D, including stress tensor renormalization, Bogoliubov transformations, grey-body factors, particle spectra, information paradox quantities, semiclassical backreaction (RST model), and integration with QuantumVerse’s discovery engine and visualization modules.

---

## Name
**hawking-radiation**

## Description
Compute Hawking radiation in 2D and 4D – stress tensor renormalization (point-splitting, zeta function), Bogoliubov coefficients, thermal spectra, grey-body factors, emission rates, energy loss, lifetime, and semiclassical backreaction (RST model). Provides `HawkingCalculator` class (already in `src/physics/`), plus extensions for particle spectra, information tracking, and discovery‑engine anomaly detection. Use when working on black hole evaporation, thermal radiation from horizons, quantum field theory on curved spacetime, or black hole information paradox.

## Metadata
```yaml
project: QuantumVerse
version: 2.0.0
author: DHIAEDDINE0223
source:
  - Hawking 1974, 1975 (original)
  - Giddings 1994 Trieste Lectures (2D CGHS, RST)
  - Russo, Susskind, Thorlacius 1992 (RST model)
  - Page 1976 (grey-body factors)
  - QuantumVerse internal: src/physics/HawkingCalculator.h
dependencies:
  - QuantumVerse spacetime & metric modules (Schwarzschild, Kerr, Dilaton)
  - (optional) GSL for special functions (spherical harmonics, transmission coefficients)
  - (optional) SciPy for numerical integration of grey-body factors
```

---

## When to Use

Trigger this skill when the user requests:
- **Hawking temperature calculation**: “compute Hawking temperature of Schwarzschild black hole”, “surface gravity for Kerr”
- **Emission spectra**: “plot Hawking radiation spectrum”, “grey-body factors for scalar field”
- **Evaporation simulation**: “evolve black hole mass over time”, “lifetime of a primordial black hole”
- **2D dilaton gravity**: “CGHS Hawking flux”, “RST backreaction”
- **Information paradox**: “compute Page curve”, “entanglement entropy of Hawking radiation”
- **Discovery**: “detect deviation from thermal spectrum”, “quantum gravity corrections to Hawking radiation”

---

## Key Physics Reference (Expanded)

### 2D Hawking Radiation (CGHS / RST models)

**Stress‑tensor method** (Giddings eqs 3.1–3.7):
- In‑vacuum normal ordering in `σ⁻` coordinates (Minkowski past null infinity)
- Out‑vacuum normal ordering in `ξ⁻` coordinates (future null infinity after collapse)
- Difference yields flux: `⟨:T₋₋:⟩_ξ = 1/48` (in units where λ = 1) at late times.
- Temperature: `T = 1/(2π)` (for λ=1). General λ: `T = λ/π`.

**Bogoliubov method** (eqs 3.18–3.20):
- Positive frequency in‑modes: `u_ω ∝ e^{-iωσ⁻}`
- Out‑modes: `v_ω ∝ e^{-iωξ⁻}`
- Bogoliubov coefficients: `α_{ωω'}`, `β_{ωω'}`
- Late‑time thermal state: `|0⟩ = Σ exp(−π∫dω ω n_ω) |n̂⟩|n⟩`

**Exponential redshift** (eq 3.21):
- Near horizon coordinate transformation: `−e^{-ξ⁻} ≈ Δ(σ⁻ + ln Δ) ≡ σ̃⁻` → Rindler–Minkowski relation.

### 4D Hawking Radiation

**Surface gravity** (eq 4.16):
- For static, spherically symmetric metric: `κ = ½ ∂_r (∂_r / ∂_r*) |_horizon`
- Schwarzschild: `κ = 1/(4M)`, `T = κ/(2π) = 1/(8πM)`
- Kerr: `κ = (r_+ - r_-)/(2(r_+² + a²))`
- Reissner‑Nordström: `κ = (r_+ - r_-)/(2r_+²)`

**Grey‑body factors**:
- Effective potential `V(r*)` for each partial wave ℓ and spin (scalar, Dirac, vector, tensor).
- Transmission coefficient `Γ_{ωℓ}` through angular momentum barrier.
- Emission rate: `dE/dt = Σ_{ℓ} ∫ (dω/2π) ω Γ_{ωℓ} / (e^{ω/T} - 1)`

**Mass loss rate** (eq 4.24):
- `dM/dt = -α / M²` where `α` depends on particle content and grey‑body factors.
- Lifetime: `τ ∼ (M/m_pl)³ t_pl`

### Semiclassical Backreaction (RST Model)

**Conformal anomaly** (eq 5.10): `⟨T⟩ = R/24` (in 2D)
**RST counterterm** (eq 5.15): `e^{-2φ} → e^{-2φ} + κρ/2`, where `κ = Nℏ/12`
**Key equation** (eq 5.23):
```
e^{-2φ} + (κ/2)φ = M + e^{σ⁺}(e^{σ⁻} - Δ) - (κ/4)(σ⁺ - σ⁻)
```
**Critical point** `φ_cr = -½ ln(κ/4)`. When RHS falls below `e^{-2φ_cr}`, semiclassical approximation fails – horizon meets singularity.
**Naked singularity time** (eq 5.26):
```
σ⁻_NS = -ln(Δ/(1 - e^{-4M/κ}))
```

---

## Implementation Modules (QuantumVerse)

### 1. `HawkingCalculator` Class (Existing – Enhance)
Location: `src/physics/HawkingCalculator.h`

**Current methods**:
- `computeSurfaceGravity(MetricTensor& metric, Event4D horizonPoint)` – returns κ
- `computeTemperature(double kappa)` – returns T

**New methods to add**:
```cpp
class HawkingCalculator {
public:
    // Grey‑body factor for a given ℓ, ω, black hole parameters
    double transmissionCoefficient(int ell, double omega, double M, double a=0.0, double Q=0.0);
    
    // Differential emission rate d²E/(dω dt) for particle species (0=scalar, 1=Dirac, 2=vector, 3=tensor)
    double differentialEmissionRate(double omega, int spin, int ell, double M, double a, double Q);
    
    // Total power loss dM/dt (integrated over all ℓ, ω)
    double powerLoss(double M, double a, double Q, double cutoff_omega_max = INFINITY);
    
    // Lifetime from initial mass M0 to final mass Mf (default Mf = 0)
    double lifetime(double M0, double Mf = 0.0, double a=0.0, double Q=0.0);
    
    // For 2D CGHS: compute asymptotic flux (should be 1/48 in units where λ=1)
    double flux2D(double lambda);
    
    // RST model: evolution of dilaton φ at horizon
    double rstHorizonDilaton(double sigma_minus, double M, double kappa, double Delta);
    
    // Page curve: entanglement entropy of Hawking radiation as function of time
    double pageEntropy(double elapsedTime, double initialMass);
};
```

### 2. Grey‑body Integrator (Numerical)
Implement in `src/physics/GreyBodyIntegrator.cpp`:
- Solves radial wave equation for each ℓ, ω using numerical integration (e.g., Runge‑Kutta) from horizon to infinity.
- Computes transmission coefficient via asymptotic matching (WKB or exact).
- Caches results for efficiency.

### 3. Particle Spectra Generator
Class `HawkingSpectrum`:
- Generates synthetic spectra for scalar, photon, neutrino, graviton emission.
- Outputs to `std::vector<double>` for plotting or integration.

### 4. Information‑Paradox Tracker
Class `PageCurveTracker`:
- Tracks evaporated mass, emitted entropy, and remaining Bekenstein‑Hawking entropy.
- Computes Page time when `S_rad = S_BH/2`.

---

## Usage Examples

### Basic 4D Schwarzschild
```cpp
#include "physics/HawkingCalculator.h"

HawkingCalculator hc;
double M = 5.0; // solar masses
double kappa = hc.computeSurfaceGravitySchwarzschild(M);
double T = hc.computeTemperature(kappa); // T ≈ 1/(8πM) in Planck units
double power = hc.powerLoss(M); // ~ 10^-5 (Planck units)
double lifetime = hc.lifetime(M); // ~ 10^66 years for stellar mass
```

### Emission Spectrum for a Given ℓ
```python
from QuantumVerse import HawkingCalculator
import matplotlib.pyplot as plt

hc = HawkingCalculator()
M = 1.0
omega = np.linspace(0.01, 1.0, 500)
spectrum = [hc.differentialEmissionRate(w, spin=0, ell=0, M=M) for w in omega]
plt.loglog(omega, spectrum)
plt.xlabel('ω (Planck units)')
plt.ylabel('d²E/dωdt')
plt.title('Hawking radiation spectrum (scalar, s-wave)')
plt.show()
```

### 2D RST Backreaction
```cpp
#include "physics/HawkingCalculator.h"

HawkingCalculator hc;
double kappa = 1.0; // example
double M = 0.5;
double Delta = 0.1; // shockwave parameter
double sigma_minus = -10.0; // early time
double phi = hc.rstHorizonDilaton(sigma_minus, M, kappa, Delta);
while (phi > -0.5 * log(kappa/4)) {
    sigma_minus += 0.1;
    phi = hc.rstHorizonDilaton(sigma_minus, M, kappa, Delta);
}
std::cout << "Naked singularity forms at σ⁻ = " << sigma_minus << std::endl;
```

### Page Curve Simulation (C++ or Python)
```python
pt = PageCurveTracker(initial_mass=100.0, particle_content='SM')
for t in range(0, 10000):
    pt.evolve(dt=1.0)
    if pt.get_radiated_entropy() > pt.get_bekenstein_hawking_entropy() / 2:
        print(f"Page time reached at t = {pt.get_time()} Planck units")
        break
```

---

## Validation Targets (Expanded)

| Quantity | Expected Value | Tolerance | Test |
|----------|---------------|-----------|------|
| 2D CGHS asymptotic flux | 1/48 (λ=1) | 1e-6 | `test_hawking_2d_flux` |
| Schwarzschild temperature | 1/(8πM) | 1e-6 | `test_hawking_temperature` |
| Schwarzschild power loss | 2.0×10⁻⁵ / M² (Planck units, scalar only) | 10% | `test_hawking_power` |
| Grey‑body factor ℓ=0 scalar | 1 (at high ω) | 1e-2 | `test_grey_body_high_energy` |
| Grey‑body factor ℓ=1 scalar | <1 | monotonic | `test_grey_body_low_energy` |
| Page time (Schwarzschild, pure gravitons) | ~ M³ (Planck units) | order of magnitude | `test_page_time` |

---

## Integration with Discovery Engine

The skill registers `HawkingAnomalyDetector` with `DiscoveryEngine`:
- Compares observed emission (from simulation or mock data) with thermal spectrum.
- Flags deviations: `Δ = |observed_rate - thermal_rate| / thermal_rate > threshold`
- Can trigger RL agent to explore modified gravity or quantum corrections.

Example discovery output:
```
⚠️ Hawking radiation anomaly: observed power loss 2.3×10⁻⁴, thermal expects 1.8×10⁻⁴ → +28% deviation.
Hypothesis: quantum gravity corrections (Planck-scale modifications to dispersion relation).
```

---

## Testing Commands

```bash
# Run all Hawking radiation tests
ctest -R hawking -V

# Specific test: 2D flux
./tests/test_hawking_calculator --gtest_filter=TwoDFlux

# Compare numerical vs analytic grey-body for ℓ=0
./tests/test_grey_body --ell 0 --method numerical

# Page curve simulation test (must reach Page time within 1e6 steps)
./tests/test_page_curve --max-steps 100000
```

---

## Return Codes

| Code | Meaning |
|------|---------|
| 0 | All Hawking tests pass |
| 1 | Temperature mismatch > 1% |
| 2 | Grey‑body factor integration fails |
| 3 | Power loss out of range |
| 4 | 2D flux incorrect |
| 5 | Information tracking (Page curve) fails |

---

## Extensibility

Users can add new black hole metrics (e.g., dilaton, Gauss‑Bonnet) by:
1. Subclassing `HawkingCalculator` and overriding `computeSurfaceGravity()`.
2. Implementing grey‑body integration for the new metric’s effective potential.
3. Adding corresponding tests.

The skill also supports user‑provided particle content via `setParticleSpecies(std::vector<int> spins, std::vector<int> degeneracies)`.

---

**Upgrade Summary**:
- Added grey‑body factors and emission spectra.
- Provided full Page curve and information paradox tracking.
- Integrated with QuantumVerse discovery engine.
- Extended 2D RST model implementation.
- Included Python examples for plotting.
- Defined comprehensive testing and validation.

This elevates the skill to a **world‑class computational Hawking radiation toolkit**, suitable for both pedagogical and research use within QuantumVerse.