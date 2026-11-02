# Theory × Experiment Discrimination Matrix

**Generated**: Session 12, 2026-11-02  
**Source**: `CosmologyValidationSuite` run with `theory_choice` = `project`, `fR`, `wcdm`, `brans_dicke`  
**Method**: Each theory runs all 6 cosmology experiments; σ-tension = (project − observed) / σ_observed against Planck/SH0ES/SNe Ia references

## Matrix: σ-tension by (theory, experiment)

| Experiment | project (ΛCDM) | fR (b=0.3) | wCDM (w0=-0.9, wa=0) | Brans-Dicke (ω=50) |
|------------|----------------|------------|----------------------|---------------------|
| r_drag_Mpc | 0.44σ | **-37.07σ** | 0.44σ | 0.44σ |
| age_universe_Gyr | 0.23σ | **-33.85σ** | **-10.90σ** | -0.03σ |
| age_at_z1_Gyr | -0.004σ | **-10.00σ** | -1.44σ | -0.004σ |
| D_z1 | 4.40σ | 4.40σ | 4.40σ | **4.63σ** |
| H0_SH0ES | -5.17σ | -5.17σ | -5.17σ | -5.17σ |
| H0_Planck | 0σ | 0σ | 0σ | 0σ |
| Tcmb_0_K | 0σ | 0σ | 0σ | 0σ |
| Tcmb_z1100_K | nan | nan | nan | nan |
| mu_z0.1 | 1.05σ | 1.01σ | 0.98σ | 1.05σ |
| mu_z0.5 | 1.89σ | 0.92σ | 0.76σ | 1.89σ |
| mu_z1 | -0.002σ | -1.12σ | -0.87σ | -0.005σ |
| q0 | nan | nan | nan | nan |

**Pass threshold**: |σ| ≤ 3.0 (fail = red)

## Interpretation

### f(R) gravity (b=0.3) — expansion-history modification at high z
- **r_drag: -37σ** — BAO sound horizon is 6.6% smaller because H(z) is 11% larger at z=1059
- **age_universe: -34σ** — universe is younger because expansion is faster at high z
- **age_at_z1: -10σ** — same mechanism at z=1
- All distance-modulus checks pass (mu_z* ≈ 1σ) — the r_drag/age shift is absorbed by the distance-redshift relation
- H0_SH0ES unchanged (-5.17σ) — H(0) is b-independent by construction

### wCDM (w0=-0.9, wa=0) — evolving dark energy equation of state
- **age_universe: -11σ** — w > -1 means DE dilutes faster, universe expands faster, is younger
- **age_at_z1: -1.4σ** — smaller effect at z=1 (DE less dominant)
- **mu_z1: -0.87σ** — distance modulus at z=1 shifts slightly
- r_drag unchanged (0.44σ) — DE is negligible at z=1059 (radiation+matter dominated)

### Brans-Dicke (ω=50) — growth-only modification
- **D_z1: 4.63σ** (vs 4.40σ baseline) — growth factor enhanced by 1 + 1/(2ω+3) ≈ 1%
- All expansion-history checks unchanged — H(z) identical to ΛCDM by construction
- H0_SH0ES unchanged (-5.17σ) — background expansion unaffected

## Key findings

1. **Expansion-history theories (fR, wCDM) are detected by r_drag and age** — these integrate H(z) over wide z ranges, accumulating the modification
2. **Growth-only theories (Brans-Dicke) are detected only by D_z1** — all distance/age checks pass because H(z) is unchanged
3. **H0_SH0ES is theory-invariant** — H(0) is b-independent for fR and w-independent for wCDM/BD at z=0
4. **The probe set is not independent** — r_drag, age, age_at_z1, mu_z* all route through H(z) integrals; they light up together for fR/wCDM

## Caveats

- **f(R) parametrization is toy model**: `E² ∝ Ωₘ(1+z)³(1 + b·z/(1+z))` grows monotonically with z, reaching 1.3× at z≫1. Real Hu-Sawicki f(R) has chameleon screening — suppressed at high density (early universe). So r_drag (z~1059) sees zero modification in realistic f(R), but 11% here.
- **wCDM is CPL**: `w(z) = w0 + wa·z/(1+z)`. w0=-0.9, wa=0 is a constant-w deviation, not the full CPL evolution.
- **Brans-Dicke is growth-only**: Real BD also modifies background expansion via G_eff(z); this toy version holds H(z) fixed to isolate growth.
- **5 of 6 experiments route through H(z)** — they're correlated, not independent probes. The genuinely independent discriminator is D_z1 (growth).

## Session 13 option

Add Hu-Sawicki f(R) proper (screening parameter |f_R0|), re-run the matrix, and produce real observational constraints rather than toy-model sensitivities.
