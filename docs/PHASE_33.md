# docs/PHASE_33.md — Phase 33: Closed (not feasible as scoped)

Status: **CLOSED — not feasible as scoped**. See "Scope decision invalidated" and "Criterion retired" below.

## Provenance

The acceptance criterion below originated in a `project.md` memory record (not a repo file). That record contained two literal placeholders — `X%` and `N steps` — with no numeric values defined anywhere in memory or the repository. During this session those placeholders were resolved:

- **N = 20** (matches the existing `AdjointGradientOptimizerTest` fixture at `tests/discovery/test_adjoint_gradient_optimizer.cpp:22`; N=10 was measured and the FD path increases chi2 over 10 steps, so N=10 fails). N=20 and LR=0.1 are now frozen in this spec — they are not re-derivable from the existing test; if that test changes, Phase 33 does not move.
- **Fallback clause dropped** — the `>= X%` fallback was redundant with "final < initial" once "monotonic" was relaxed to "final < initial"; with N=20 the FD decrease is ~0.12%, so any achievable X would test nothing.

This file is now the authoritative source. Do not re-derive from memory records.

## Acceptance criterion (RETIRED — unfulfillable by existing architecture)

The criterion below was the original memory-record acceptance criterion. It is **retired** — findings 4 and 5 prove it cannot be satisfied by wiring `AdjointGeodesicIntegrator` into `evaluateTheory()`. The criterion remains here only as a historical record of what was attempted and why it fails.


At the fixed point `{mass_param=500.0, coupling=0.9}`, `TheoryType::BRANS_DICKE`, LR=0.1:

1. **Gradient-match**: `||g_adj - g_fd||_2 / max(||g_fd||_2, 1e-12) < 1e-4`, where `g_adj = computeAdjointGradient(params)` and `g_fd = computeFDGradient(params)`.
2. **chi2 trajectory**: `chi2 at step 20 < chi2 at step 0`, measured through `optimizeWithGradient()` for `TheoryType::BRANS_DICKE` **after wiring** (i.e., the production path calls `computeAdjointGradient()`).
3. **Adjoint-path-invoked**: `computeAdjointGradient()` must have actually executed the `AdjointGeodesicIntegrator` branch — asserted via invocation counter, NOT inferred from output matching (rel_err ≈ 0 is trivially satisfiable by FD comparing to FD, which is the current behavior: diagnostic measured 4.1e-7 rel_err between `computeAdjointGradient()` and an external FD computation because `computeAdjointGradient()` *is* FD today).

**No fallback clause.** `X` is moot.

If post-wiring `rel_err >= 1e-4`, chi2 does not decrease over 20 steps, or gradients are non-finite, that is a **finding** — the adjoint gradient is wrong or the wiring is wrong. Do not silently relax the threshold to make the test pass.

## Call graph (confirmed)

`optimizeWithGradient()` (`src/discovery/TheoryDiscoveryAgent.cpp:1017`) calls `computeAdjointGradient()` at `:1046`. Wiring `computeAdjointGradient()` for Brans-Dicke therefore changes production behavior and is observable through the chi2 trajectory criterion. This is not test-only code.

`adjointBranchInvocationCount()` is a public static member of `TheoryDiscoveryAgent` (`src/discovery/TheoryDiscoveryAgent.h`), backed by `static std::atomic<int> s_adjointBranchInvocations` defined in the `.cpp`.

## Deliverable (real code, not a one-line un-gate)

Zero callers of `AdjointGeodesicIntegrator` exist in `src/discovery/` (grep confirmed). Phase 33 wiring is:

1. **Extract FD loop** from `computeAdjointGradient()` (`src/discovery/TheoryDiscoveryAgent.cpp:983`, FD loop at `:999-1011`, Brans-Dicke skip comment at `:993-996`) into a new `computeFDGradient()`. Pure refactor — `AdjointGradientOptimizerTest` must still pass after this step alone. This is the safe checkpoint.
2. **Rewrite `computeAdjointGradient()`** to use `AdjointGeodesicIntegrator` (generalized constructor at `src/physics/AdjointGeodesicIntegrator.h:220`) for **Brans-Dicke only**; keep FD fallback for other theory types.

   **Risk**: The original `:993-996` comment explicitly skips the adjoint path for Brans-Dicke. If un-gating produces `rel_err >= 1e-4` or non-finite gradients, the original skip was load-bearing — that is a Phase 33 finding and the wiring needs the integrator extended, not the test relaxed.
3. **Adjoint-path-invoked assertion**: `computeAdjointGradient()` increments `static std::atomic<int> s_adjointBranchInvocations` only on the `AdjointGeodesicIntegrator` branch. Test reads `adjointBranchInvocationCount()` before and after `computeAdjointGradient({500,0.9})`, asserts `after - before >= 1`. No reset function needed. Also `qDebug()` under `PHASE33_ADJOINT_DEBUG=1` for debug parity with the `LENSING_DEBUG` precedent at `src/rendering/GravitationalLensing.cpp:641-643` and `GRID_DEBUG` at `src/rendering/CurvatureRenderer.cpp:1083`.

## Test location

**New file**: `tests/discovery/test_adjoint_phase33.cpp` (do not extend `test_adjoint_gradient_optimizer.cpp` — that file tests the legacy FD path; keep them separate).

Register in `CMakeLists.txt` near the other Phase 33 entries (`test_adjoint_gradient_optimizer` at `:2192`, `test_generalized_adjoint` at `:2236`), with comment `# Phase 33.1: adjoint wiring gate`.

## Non-goals (scope confirmed clean)

Does **not** touch lensing, camera, RK4, or `rs = 2GM/c²`. No collision with the six settled fixes from the lensing session.

## Findings (step 2 pre-flight)

### Finding 1 — BD has no AD overrides (DiscoveryEngine.cpp:459-463)
`BransDickePlugin::computeChristoffel` returns all zeros; `computeMetric` ignores `omega` (`(void)parameters` at DiscoveryEngine.cpp:446). BD as implemented is not a viable AD target.

### Finding 2 — `omega` is alive but not in the metric (TheoryDiscoveryAgent.cpp:880-884)
`omega` enters `c_gw` only via `computeGW170817Chi2`, at TheoryDiscoveryAgent.cpp:883 (`ratio = (1 + omega/phi0) / (1 + 2*omega/phi0)`). `phi0` enters both `computeMetric` (`g[0][0] = -factor * phi0`, DiscoveryEngine.cpp:452) and `computeGW170817Chi2`.

### Finding 3 — FD gradient at `{500.0, 0.9}` is effectively 1-component (diagnostic 2)
`grad[omega] = 0.00127`, `grad[phi0] = -51.96`. Gradient-match criterion at rel_err < 1e-4 is dominated by the phi0 component; omega contributes negligibly.

### Finding 4 — `AdjointGeodesicIntegrator` does not compute `d(chi2)/d(params)` (decisive)
The integrator (`src/physics/AdjointGeodesicIntegrator.h`) computes:
- `d(final_state)/d(params)` via `integrate()` / `computeStateGradient()` (lines 235-410)
- `d(Kretschmann)/d(params)` via `computeCurvatureGradient()` (lines 354-390)

`computeAdjointGradient()` needs `d(chi2)/d(params)` through `evaluateTheory()` → `computeObservationalChi2()` + `computeGW170817Chi2()` (TheoryDiscoveryAgent.cpp:345-347). These chi2 functions sum residuals over observational data (Pantheon, BAO, CMB, GW170817) — **not** geodesic state or curvature. There is **no composition layer** mapping the integrator's output to `d(chi2)/d(params)`.

**Implication**: Wiring `AdjointGeodesicIntegrator` directly into `computeAdjointGradient()` produces gradients that do not match FD chi2 gradients, even for a metric with full AD support (only KerrMetric has it — KerrMetric.h:87-92). Finding 4 changes Phase 33's shape; the criterion as written cannot be satisfied by the integrator alone.

### Finding 5 — Neither chi2 function consumes its `metric` argument (most decisive)
`computeObservationalChi2(const MetricTensor& metric, ...)` at TheoryDiscoveryAgent.cpp:734: **`(void)metric` at line 738** — the metric argument is entirely unused. Chi2 is computed from:
- Pantheon+ SNe Ia: `luminosityDistanceMpc(z, H0, omegaM, omegaLambda)` — standard ΛCDM, not the metric
- BAO: `computeBAOChi2(params)` → `baoEffectiveDistanceMpc(z, H0, omegaM, omegaLambda)` — ΛCDM
- CMB: `computeCMBShiftChi2(params)` → `comovingDistanceLCDM(Z_REC, H0, omegaM, omegaLambda)` — ΛCDM
- BBN: `computeBBNChi2(params)` → scalar functions of omegaB only

`computeGW170817Chi2(const MetricTensor& metric, ...)` at TheoryDiscoveryAgent.cpp:868: **`(void)metric` at line 872** — also entirely unused. Chi2 is `pow(expected_delay - observed_delay, 2) / sigma²` where `expected_delay = observed_delay * (C_LIGHT / c_gw)` and `c_gw` is a scalar algebraic function of theory params (line 883 for BD).

**Implication**: Neither chi2 function consumes the metric tensor, geodesic state, or curvature — the quantities the integrator differentiates. The chain `metric → geodesic → curvature → chi2` **does not exist** in `evaluateTheory()`. Even with full AD through `computeMetric` and `AdjointGeodesicIntegrator`, the AD output feeds into nothing in the chi2 computation. The premise that "adjoint geodesic integrator → chi2 gradient" is architecturally broken at both levels: the integrator computes the wrong quantity (Finding 4), and chi2 doesn't consume its output at all (Finding 5).

**Root architectural fact**: `evaluateTheory()` calls `plugin->computeMetric(test_location_, param_map)` (line 312) then passes `metric` to chi2 functions that ignore it (`(void)metric` at 738 and 872). The metric is computed but never used for chi2. chi2 is a function of scalar cosmological + theory params only.

## Scope decision invalidated (Correction 3)

The memory-record scope decision (`phase33_scope`) stated:

> "Phase 33 wires AdjointGeodesicIntegrator into TheoryDiscoveryAgent::evaluateTheory() for χ²-based parameter optimization"

This scope decision is **invalidated** by findings 4–5:

- **Finding 4**: `AdjointGeodesicIntegrator` computes `d(state)/d(params)` and `d(Kretschmann)/d(params)`, not `d(chi2)/d(params)`. The integrator's output is not the quantity the criterion needs.
- **Finding 5**: Neither chi2 function consumes its `metric` argument (`(void)metric` at TheoryDiscoveryAgent.cpp:738 and :872). The chi2 computation is a function of scalar cosmological + theory params only — it never touches the metric, geodesic state, or curvature that the integrator differentiates.

The premise that "adjoint geodesic integrator → chi2 gradient" is architecturally broken. `AdjointGeodesicIntegrator` cannot be wired into `evaluateTheory()` for chi2-based optimization because nothing on the chi2 path consumes it. Any future phase that needs chi2 adjoints must start from scratch with AD-differentiable chi2 functions (option D=E below), not from the integrator.

## Criterion retired

The criterion `rel_err(adjoint, FD) < 1e-4 on chi2 gradient at {500,0.9}` plus `chi2_final < chi2_initial` over 20 steps is **retired**. It cannot be satisfied by the existing `AdjointGeodesicIntegrator`. It is satisfiable only by option D=E (multi-week chi2 AD rewrite), which is deferred as a future phase.

## Re-scope proposal (deferred)

If chi2-based adjoint parameter optimization becomes a near-term requirement, the work is:

> **Make the chi2 chain AD-differentiable and expose reverse-mode gradient through `evaluateTheory()`.**

This requires making AD-aware: `luminosityDistanceMpc`, `comovingDistanceLCDM`, `distanceModulus`, `extractCosmologicalParams`, `computeBAOChi2`, `computeCMBShiftChi2`, `computeBBNChi2`, `computeGW170817Chi2`, and their call chain. Sized at **weeks**, not days — a multi-week project, not a one-line wiring change.

This becomes Phase-NEW when product confirms it's needed downstream. Phase 33 does not start this work.

## Orthogonal capability retained

`AdjointGeodesicIntegrator` is unaffected and remains available for its actual capabilities:
- `d(final_state)/d(params)` via `integrate()` / `computeStateGradient()` (lines 235-410)
- `d(Kretschmann)/d(params)` via `computeCurvatureGradient()` (lines 354-390)

`test_adjoint.cpp` Tests 4–6 verify these at unit level. The capability is orthogonal to chi2 optimization and remains available for future curvature- or state-gradient work. Nothing in this close removes or degrades it.

## Non-goals (scope confirmed clean)

Does **not** touch lensing, camera, RK4, or `rs = 2GM/c²`. No collision with the six settled fixes from the lensing session.


