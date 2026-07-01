# Upgraded Physics Validation Skill

**File**: `.kilo/skills/validate-physics/SKILL.md`  
**Upgrade Version**: 2.0.0 ("Exhaustive Validation Suite")  
**Date**: 2026-05-28

This upgrade transforms the physics validation skill into a comprehensive, automated verification system covering **General Relativity benchmarks**, **modified gravity tests**, **quantum gravity predictions**, **numerical accuracy checks**, and **discovery engine cross-validation**.

---

## Name
**validate-physics**

## Description
Run scientific validation – GR benchmarks, dilaton gravity checks, quantum gravity tests, numerical convergence, and anomaly cross‑validation. Supports full test suites, single‑test execution, automated report generation, and integration with the discovery engine. Use when the user says "validate physics", "check GR", "run benchmarks", "verify quantum gravity", or "full validation".

## Metadata
```yaml
project: QuantumVerse
version: 2.0.0
author: DHIAEDDINE0223
dependencies: 
  - ctest >= 3.20
  - python >= 3.9 (with numpy, scipy, matplotlib)
  - ONNX Runtime (for neural surrogate validation)
optional: 
  - CUDA (for GPU benchmark)
  - LaTeX (for report generation)
```

---

## When to Use

Trigger this skill when the user requests any of the following:
- **Physics correctness verification**: "validate physics", "check GR", "verify spacetime"
- **Benchmark execution**: "run benchmarks", "performance validation", "numerical accuracy"
- **Quantum gravity checks**: "validate quantum gravity", "test CDT", "spin foam verification"
- **Full test suites**: "full validation", "all tests", "comprehensive check"
- **Discovery cross-check**: "validate anomaly", "confirm discovery", "verify deviation"
- **Regression testing**: "test after change", "compare with baseline"
- **Report generation**: "generate validation report", "export results"

---

## Process

### Step 1: Determine Validation Scope

The skill automatically selects scope based on user command:

| Command Pattern | Scope |
|----------------|-------|
| "full validation", "validate everything" | **Full Suite** (all tests + benchmarks + discovery) |
| "validate GR", "check relativity" | **GR Tests Only** (Mercury, light deflection, redshift, frame‑dragging) |
| "validate quantum gravity", "test QG" | **Quantum Gravity Tests** (CDT, spin foam, causal sets, GFT) |
| "validate dilaton", "test dilaton gravity" | **Modified Gravity Tests** (dilaton, f(R), scalar‑tensor) |
| "validate anomaly", "check discovery" | **Discovery Validation** (cross‑validate anomalies against known GR) |
| "performance", "benchmark" | **Performance Benchmarks** (geodesic speed, neural ODE accuracy) |
| no explicit scope | **Quick Validation** (run essential GR tests + 1 QG test) |

### Step 2: Run Validation Suite

#### 2.1 GR Historical Tests (Full Suite)
```bash
cd f:/syyyy/build
ctest -R "validation_gr|test_mercury|test_light_deflection|test_gravitational_redshift|test_frame_dragging" --output-on-failure -V
```

**Individual GR Tests**:
```bash
# Mercury perihelion precession (43"/century)
ctest -R test_mercury_precession -V

# Light deflection by Sun (1.75 arcsec)
ctest -R test_light_deflection -V

# Gravitational redshift (Pound–Rebka)
ctest -R test_gravitational_redshift -V

# Frame‑dragging (Gravity Probe B, 39 mas/yr)
ctest -R test_frame_dragging -V

# Lunar laser ranging (active/passive mass equivalence)
ctest -R test_lunar_laser_ranging -V

# Binary pulsar timing (PSR 1913+16)
ctest -R test_binary_pulsar -V
```

#### 2.2 Modified Gravity Tests
```bash
# Dilaton gravity – black hole solutions
ctest -R test_dilaton_blackhole -V

# f(R) gravity – chameleon screening
ctest -R test_fR_gravity -V

# Scalar‑tensor (Brans‑Dicke) – solar system constraints
ctest -R test_scalar_tensor -V

# Massive gravity – Vainshtein screening
ctest -R test_massive_gravity -V
```

#### 2.3 Quantum Gravity Tests
```bash
# Causal Dynamical Triangulations (CDT) – spectral dimension
ctest -R test_cdt -V

# Spin foam (LQG) – vertex amplitude
ctest -R test_spin_foam -V

# Group Field Theory – condensate phase
ctest -R test_gft -V

# Causal sets – dimension estimator
ctest -R test_causal_set -V

# Regular black holes (Hayward, Bardeen, LQG)
ctest -R test_regular_black_holes -V
```

#### 2.4 Numerical Accuracy Benchmarks
```bash
# Conservation of energy‑momentum (geodesic integrator)
ctest -R test_energy_conservation -V

# Convergence test (adaptive step vs. fixed step)
ctest -R test_convergence -V

# Symplectic property of integrator
ctest -R test_symplectic -V

# Curvature invariants (Kretschmann vs. analytic)
ctest -R test_curvature_invariants -V
```

#### 2.5 Neural Surrogate Validation (if ONNX models available)
```bash
# Geodesic Neural ODE – accuracy vs. RK4 (<1% error)
ctest -R test_neural_ode_accuracy -V

# Metric GNN – prediction error (<1%)
ctest -R test_metric_gnn_accuracy -V

# Normalizing flow – anomaly detection ROC‑AUC (>0.95)
ctest -R test_anomaly_flow_accuracy -V
```

#### 2.6 Discovery Engine Cross‑Validation
```bash
# Reproduce existing discovery (e.g., GW170817)
ctest -R test_reproduce_discovery -V

# Verify anomaly is not a numerical artifact
python f:/syyyy/scripts/validate_anomaly.py --anomaly-id <id> --confidence-threshold 0.95

# Cross‑validate against independent solver (e.g., Mathematica)
ctest -R test_cross_validate_metric -V
```

#### 2.7 Performance Benchmarks
```bash
# Geodesic integration speed (target <10 ms per 1000 steps)
ctest -R benchmark_geodesic_speed -V

# Metric evaluation throughput (target >10⁶ evaluations/s)
ctest -R benchmark_metric_eval -V

# Full scene FPS (target >45 FPS on reference GPU)
ctest -R benchmark_fps -V

# Neural ODE inference latency (target <1 ms)
ctest -R benchmark_neural_ode -V
```

#### 2.8 Regression & Consistency Checks
```bash
# Compare current results with baseline (stored in `data/baseline/`)
ctest -R regression_compare -V

# Random seed reproducibility (same seed → same result)
ctest -R test_reproducibility -V

# Determinism (multiple runs with same config yield identical outputs)
ctest -R test_determinism -V
```

### Step 3: Interpret Results

The skill automatically parses test outputs and classifies results:

| Outcome | Interpretation | User Guidance |
|---------|---------------|----------------|
| All GR tests pass within 1% tolerance | **GR validated** – spacetime physics correct | Proceed to advanced simulations |
| GR tests pass but neural ODE fails | **Surrogate needs retraining** – fallback to RK4 | Run `python scripts/train_neural_ode.py` |
| Quantum gravity tests fail | **QG engine misconfigured** – check parameters | Verify Monte Carlo seeds, theory plugin version |
| Regression mismatch | **Code change altered physics** – review recent commits | Run `git bisect` to locate change |
| Anomaly validated | **Potential new physics** – log to discovery panel | Trigger automated hypothesis generation |

### Step 4: Generate Validation Report

After suite completion, the skill creates a structured report:

#### 4.1 Console Summary
```
╔═══════════════════════════════════════════════════════════════╗
║             PHYSICS VALIDATION REPORT                         ║
╚═══════════════════════════════════════════════════════════════╝

✔ GR Historical Tests: 6/6 passed (100%)
   - Mercury precession: 42.98"/century (tolerance: 0.1%)
   - Light deflection: 1.75" (tolerance: 0.5%)
   - Gravitational redshift: 2.46e-15 (Pound-Rebka: 2.46e-15)

✔ Modified Gravity: 4/4 passed (100%)
   - Dilaton black hole: analytic match to 1e-6

⚠ Quantum Gravity: 3/5 passed (60%)
   - CDT: spectral dimension 2.03±0.12 (expected: 2.0–2.2) → PASS
   - Spin foam: vertex amplitude NaN → FAIL (fix amplitude normalization)
   - Causal sets: Hausdorff dimension 3.87±0.23 → PASS

✔ Neural Surrogates: 2/3 passed (67%)
   - Neural ODE error: 0.8% (<1%) → PASS
   - Metric GNN error: 2.3% (<1%) → FAIL (retrain with more data)

✔ Performance: 3/3 passed (100%)
   - Geodesic speed: 4.2 ms/1000 steps (target <10 ms)
   - FPS: 52 (target >45)

📊 Overall Status: 18/21 tests passed (86%) – MINOR ISSUES
   → Action required: Retrain Metric GNN, fix spin foam amplitude
   → Report saved to: f:/syyyy/validation_reports/2026-05-28_14-30-00.json
```

#### 4.2 Detailed JSON Report
The skill automatically writes a machine‑readable report:
```json
{
  "timestamp": "2026-05-28T14:30:00Z",
  "commit_hash": "a3f2c91",
  "suite": "full",
  "results": {
    "gr_tests": {"passed": 6, "failed": 0, "skipped": 0},
    "modified_gravity": {"passed": 4, "failed": 0, "skipped": 0},
    "quantum_gravity": {"passed": 3, "failed": 2, "skipped": 0},
    "neural_surrogates": {"passed": 2, "failed": 1, "skipped": 0},
    "performance": {"passed": 3, "failed": 0, "skipped": 0}
  },
  "failures": [
    {"test": "test_spin_foam_amplitude", "error": "NaN in vertex amplitude", "suggested_fix": "normalize SU(2) Clebsch-Gordan"},
    {"test": "test_metric_gnn_accuracy", "error": "2.3% > 1%", "suggested_fix": "increase training epochs or data"}
  ],
  "performance_metrics": {
    "geodesic_ms_per_1000": 4.2,
    "fps": 52,
    "neural_ode_latency_us": 890
  },
  "report_path": "f:/syyyy/validation_reports/2026-05-28_14-30-00.json"
}
```

#### 4.3 LaTeX Report (Optional)
If LaTeX is installed, the skill generates a publication‑ready PDF report:
```bash
python f:/syyyy/scripts/generate_validation_report.py --input validation_results.json --output validation_report.tex --compile
```

### Step 5: Integration with Discovery Engine

When a validation test passes but a discovery anomaly exists, the skill performs **cross‑validation**:

1. **Confirm the anomaly is not a numerical artifact** – rerun with higher precision
2. **Check against alternative metric solvers** – compare with analytical or independent code
3. **Run parameter sensitivity** – vary tolerance and observe stability
4. **Log to discovery panel** with confidence score

```bash
# Example: Validate a user‑reported anomaly
validate-physics --validate-anomaly --anomaly-id "ANOM-2026-05-27-001" --check-against-gr
```

The skill outputs:
```
🔍 Cross‑validating anomaly ANOM-2026-05-27-001:
   → GR prediction: Mercury precession 42.98"/century
   → Observed (anomaly): 47.2"/century
   → Difference: 4.22"/century (9.8% deviation)
   → Numerical convergence test: PASS (tolerance 1e-6)
   → Alternative solver (Mathematica): 47.1"/century (match)
   → Confidence: 99.7% (p=0.003)
   → Verdict: CONFIRMED – potential new physics
   → Action: Trigger RL agent to search for modified gravity parameters
```

---

## Custom Commands

| User Command | Skill Action |
|--------------|--------------|
| `validate-physics --quick` | Run essential GR tests only (Mercury, light deflection, redshift) |
| `validate-physics --full` | Run all tests (GR, modified gravity, QG, neural, performance) |
| `validate-physics --gr` | Run GR historical tests only |
| `validate-physics --quantum` | Run quantum gravity tests only |
| `validate-physics --benchmark` | Run performance benchmarks only |
| `validate-physics --regression --baseline <tag>` | Compare against tagged baseline (e.g., v2.0.0) |
| `validate-physics --report --format latex` | Generate LaTeX report |
| `validate-physics --watch` | Continuous validation – rerun tests on file changes (monitor mode) |
| `validate-physics --validate-anomaly --id <id>` | Cross‑validate a discovery anomaly |
| `validate-physics --help` | Show full usage |

---

## Dependencies & Configuration

### Required
- **CMake** ≥ 3.20 with `CTest` enabled
- **Python 3.9+** with packages: `numpy`, `scipy`, `matplotlib`, `jinja2` (for reports)
- **ONNX Runtime** (for neural surrogate validation) – optional, tests skip if absent

### Environment Variables
```bash
export QUANTUMVERSE_VALIDATION_TOLERANCE=0.01   # 1% tolerance for GR tests
export QUANTUMVERSE_BASELINE_DIR=f:/syyyy/data/baseline
export QUANTUMVERSE_REPORT_DIR=f:/syyyy/validation_reports
```

### Configuration File (`validate_physics_config.yaml`)
```yaml
# Optional custom configuration
tests:
  gr:
    mercury_tolerance_arcsec: 0.05
    light_deflection_tolerance_arcsec: 0.02
  quantum_gravity:
    cdt_spectral_dimension_tolerance: 0.2
    spin_foam_test_skip_if_no_gpu: true
performance:
  target_fps: 45
  geodesic_speed_ms_per_1000: 10.0
reporting:
  auto_generate: true
  format: "both"  # console, json, latex
  notify_on_failure: true  # popup or sound
```

---

## Return Codes

| Exit Code | Meaning |
|-----------|---------|
| 0 | All tests passed within tolerance |
| 1 | One or more tests failed (validation error) |
| 2 | Performance below target |
| 3 | Missing dependencies (e.g., ONNX Runtime not found) |
| 4 | Regression detected (deviation from baseline) |
| 5 | Anomaly confirmed (new physics candidate) – special code for discovery engine |

---

## Example Usage Session

```bash
$ validate-physics --full

[INFO] Running full validation suite...
[INFO] GR tests: 6/6 passed
[INFO] Modified gravity: 4/4 passed
[WARN] Quantum gravity: spin foam test failed (NaN amplitude)
[INFO] Neural surrogates: 2/3 passed (Metric GNN retraining needed)
[INFO] Performance: 52 FPS (target 45) – PASS
[INFO] Report saved to f:/syyyy/validation_reports/2026-05-28_14-30-00.json

Do you want to generate a LaTeX report? (y/n): y
[INFO] LaTeX report generated: validation_report.pdf

Do you want to open the report? (y/n): y
[INFO] Opening validation_report.pdf
```

---

## Extensibility

The skill supports **plugin validators**:
- Add custom validation scripts to `f:/syyyy/scripts/validators/`
- Each validator must implement `run()` and `parse_output()` methods
- The skill automatically discovers and executes `.py` or `.sh` files in that directory

Example custom validator (`scripts/validators/custom_dark_matter.py`):
```python
def run():
    # Simulate a custom test (e.g., dark matter halo profile)
    return {"passed": True, "metric": "NFW profile χ² = 1.2"}
```

---

**Upgrade Summary**:
- Added 8 validation categories (GR, modified gravity, QG, numerical, neural, performance, regression, discovery)
- Structured JSON + LaTeX reporting
- Integration with discovery engine anomaly validation
- Continuous watch mode
- Plugin architecture for custom tests
- Clear user guidance and exit codes

This transforms a simple `ctest` wrapper into a professional‑grade physics validation framework.