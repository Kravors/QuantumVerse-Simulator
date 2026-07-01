
---

### 6. `.kilo/skills/discovery-sweep/SKILL.md`

```markdown
---
name: discovery-sweep
description: Configure and run parameter sweeps through the Discovery Engine to search for new physics. Use when the user wants to explore parameter spaces, run automated searches, or find anomalies.
metadata:
  project: QuantumVerse
  phase: 2
---

# Discovery Sweep

## When to Use
When the user asks to run a parameter sweep, search for new physics, or configure discovery exploration.

## Process

### 1. Gather Parameters
Ask user for:
1. **Metric type**: Schwarzschild, Kerr, RN, dilaton, or custom
2. **Parameter ranges**: mass, spin, charge—min/max/steps
3. **Anomaly threshold**: default 5σ
4. **Output format**: CSV, JSON, HDF5
5. **Max runtime**: in minutes

### 2. Generate sweep_config.yaml
```yaml
sweep:
  name: "<descriptive name>"
metric:
  type: <schwarzschild|kerr|rn|dilaton>
parameters:
  - name: mass
    min: <value>
    max: <value>
    steps: <count>
  - name: spin
    ...
anomaly:
  threshold_sigma: 5.0
  monitors:
    - kretschmann
    - ricci_scalar
    - weyl_scalar
    - energy_condition
    - causal_structure
output:
  format: <csv|json|hdf5>
  path: <output_path>