# docs/PHASE_33.md — Phase 33: Adjoint Wiring into TheoryDiscoveryAgent

## Provenance

The acceptance criterion below originated in a `project.md` memory record (not a repo file). That record contained two literal placeholders — `X%` and `N steps` — with no numeric values defined anywhere in memory or the repository. During this session those placeholders were resolved:

- **N = 20** (matches the existing `AdjointGradientOptimizerTest` fixture at `tests/discovery/test_adjoint_gradient_optimizer.cpp:22`; N=10 was measured and the FD path increases chi2 over 10 steps, so N=10 fails). N=20 and LR=0.1 are now frozen in this spec — they are not re-derivable from the existing test; if that test changes, Phase 33 does not move.
- **Fallback clause dropped** — the `>= X%` fallback was redundant with "final < initial" once "monotonic" was relaxed to "final < initial"; with N=20 the FD decrease is ~0.12%, so any achievable X would test nothing.

This file is now the authoritative source. Do not re-derive from memory records.

## Acceptance criterion (commitments, not targets)

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
