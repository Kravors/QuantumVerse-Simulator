# NEXT STEPS — QuantumVerse Spacetime GPS + Experiment Runner

Status of the work completed in this session, and the concrete items remaining before the feature is production-ready.

## Completed (this session)

| Area | What landed | Files |
|------|-------------|-------|
| Parameter Sweep controls | Export CSV button wired to `exportSweepResults("csv")`, gated by `hasResults` | `src/main.qml`, `src/discovery/DiscoveryPanelManager.{h,cpp}` |
| RL ONNX message | Softened to "offline — Load model…"; Load button added | `src/main_qml.cpp`, `src/main.qml` |
| Spacetime GPS | `SpacetimeGPS : QAbstractListModel` registry + telemetry feed (pos/vel/spin/spherical/redshift/proper-time), throttled 10 Hz, `pin()`/`focusOn()` | `src/ui4d/SpacetimeGPS.{h,cpp}`, `src/main_qml.cpp`, `src/qmlglviewport.{h,cpp}` |
| GPS panel | QML list + detail table + trail canvas (right dock) | `src/GPSPanel.qml`, `src/resources.qrc` |
| Experiments framework | `Experiment` base + `ExperimentRunner` (worker-thread via QThreadPool), dynamic param form, results canvas/log | `src/experiment/Experiment.{h,cpp}`, `src/experiment/ExperimentRunner.{h,cpp}`, `src/ExperimentParamForm.qml`, `src/ExperimentsPanel.qml` |
| Experiments (3 full) | MassSweep, SpinSweep, LensingDeflection | `src/experiment/experiments/{MassSweep,SpinSweep,LensingDeflectionExperiment}.{h,cpp}` |
| Experiments (7 more) | ChargeSweep, MercuryPrecession, FrameDragging (full); RedshiftProfile, AnomalyScan, TheoryComparison, QuantumGeometry (skeletons) | `src/experiment/experiments/*.{h,cpp}` |
| Metric epoch | `MetricTensor::epoch()` + `bumpEpoch()` on every setter; `KerrMetric::setMass`/`setSpin` call it | `src/spacetime/MetricTensor.h`, `src/spacetime/KerrMetric.{h,cpp}` |
| History UI | `ExperimentRunner::showHistoryEntry(id)` + History section in panel | `src/experiment/ExperimentRunner.{h,cpp}`, `src/ExperimentsPanel.qml` |
| Tests | 7 new ctest tests, all green; existing SpacetimeGps/ExperimentRunner/UiWiring still green | `tests/test_metric_epoch.cpp`, `test_charge_sweep.cpp`, `test_mercury_precession.cpp`, `test_frame_dragging_experiment.cpp`, + prior `test_spacetime_gps.cpp`, `test_experiment_runner.cpp` |

Build: `quantumverse_qml` + all 8 test targets build clean with zero warnings. Headless `--frames 2 --software-rendering` loads all QML panels. `ctest` all green.

## Immediate next steps (this week)

### 1. Expand the 4 skeleton experiments to full implementations

Each skeleton currently returns an `ExperimentResult` with at least one scalar/curve point, so the UI shows something instead of a blank canvas. Expand them to real physics:

| Experiment | What's missing | Real computation to add |
|------------|---------------|-------------------------|
| `RedshiftProfile` | Currently computes `z(r) = 1/sqrt(-g_tt) - 1` for Schwarzschild — already functional | Extend to Kerr at θ=0; add reference curve for flat space |
| `AnomalyScan` | Grid scan flags Kretschmann outliers vs median; MAD threshold | Wire real `DiscoveryEngine::detectAnomaly(location, metric)` instead of inline `curvatureScalars()`; add confidence field |
| `TheoryComparison` | Compares Schwarzschild vs Kerr vs Minkowski K at one probe | Wire registered `TheoryManager` plugins (FRLGravity, BransDicke, LQG, TeVeS, EinsteinAether, Horndeski, YukawaFifthForce); one curve per theory |
| `QuantumGeometry` | CDT spectral dimension vs diffusion scale | Already functional via `CDTEngine::computeSpectralDimension(mu)`; add SpinFoam/GFT/CausalSet comparison curves |

Files: `src/experiment/experiments/<name>.{h,cpp}`

No QML changes needed — `ExperimentRunner::parametersFor(id)` already drives `ExperimentParamForm.qml` dynamically from the `type` field.

### 2. Experiment result persistence

`ExperimentRunner::history()` is in-memory only (bounded to last 200). Add optional persistence:

- On `exportLast("csv"|"json")`, also write a `summary.json` next to the export with id, name, timestamp, scalars.
- `showHistoryEntry(id)` should fall back to reading a persisted summary if the in-memory entry was evicted.

Files: `src/experiment/ExperimentRunner.cpp` (add a `loadHistoryEntryFromDisk(id)` helper), `src/main_qml.cpp` (point the runner at `data/experiments/`).

### 3. PDF export of experiment results

Currently `exportLast` supports csv/json only. Add PDF via `PaperGenerator` (already used for the discovery paper pipeline at `Documents\QuantumVerse\output\discovery_paper.pdf`).

Files: `src/experiment/ExperimentRunner.cpp::exportLast()` — add `"pdf"` branch calling `PaperGenerator`.

## Medium-term (next 2–3 weeks)

### 4. In-place mass mutation refactor + grid recompute unification

The epoch fix (step above) makes in-place mass mutation safe, but `AGENTS.md` flags a second half that's still deferred:

> a grid that reads a stale metric pointer and a sweep that reallocates one per sample are two views of the same lifetime problem.

Work remaining:

1. Change the mass sweep path in `QmlGlViewport` / `runParameterSweep` to call `metric->setMass(M)` in place instead of rebuilding `m_probeMetric` per sample (invalidating the probe cache via `setProbeMetric()`).
2. In `setProbeMetric()`, add `m_curvatureRenderer->recomputeGridZRange(*m_probeMetric)` + `m_curvatureRenderer->markGridDirty()` + `update()` so the grid deformation follows the live mass.
3. Add a regression test asserting `gridZRange` changes after `setMass()` (the v7.2 changelog shows it regressed from `-6.4..-0.0708` back to `0..0` at one point).

Files: `src/qmlglviewport.cpp` (`setProbeMetric`, `updateSimulation`), `src/rendering/CurvatureRenderer.cpp` (`recomputeGridZRange`), `tests/test_spacetime_gps.cpp` (add cache-invalidates-on-mutation test).

### 5. Skeleton experiments → validation against the GR suite

Per `AGENTS.md` validation discipline, before claiming any new GR result:

- Mercury precession experiment is **analytic** (validates the formula, not the integrator) — documented in header. The integrator is validated separately in `test_validation.cpp`.
- Frame-dragging uses the correct SI formula; default M=1M☉/a*=0.9/r=7e6m → `2.23422e+10` mas/yr pinned as regression. The 39 mas/yr scalar is a GP-B reference label only (GP-B assumes M=Earth, different J).

If any skeleton is switched to numerical geodesic integration later, add a convergence test (halve step count, assert answer changes < 0.1 arcsec).

## Verification & test checklist (before commit)

Run these commands:

```bash
# Reconfigure (required after CMakeLists changes)
cmake F:\syyyy -B F:\syyyy\build_v71 -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_BUILD_TESTS=ON

# Build everything
cmake --build F:\syyyy\build_v71 --parallel

# Run the full core suite (canaries excluded per AGENTS.md)
ctest -C Release --output-on-failure

# Headless smoke test
build\Release\quantumverse_qml.exe --headless --frames 3 --software-rendering
```

Canaries (non-blocking per AGENTS.md): `PerformanceGateTest`, `QMLPerformanceBaseline`, `ViewportStateTest`, `ViewportContentTest`, `VisualRegressionTest`, `AnimationTimingTest`, `GLStrictAuditTest`.

**Assertions must survive NDEBUG** — all new tests use `QV_CHECK`/`QV_CHECK_NEAR` from `tests/test_assert.h`, never bare `assert()`.

## Commit readiness

When ready to commit, use this message shape:

```
feat(experiments,gps,metrics): finish deferred experiment work + metric epoch

Experiments:
- ChargeSweep (RN horizons analytic), MercuryPrecession (GR formula),
  FrameDragging (Lense-Thirring, reference case documented)
- Skeletons for RedshiftProfile, AnomalyScan, TheoryComparison,
  QuantumGeometry — each returns at least one scalar so the UI has
  something to show; marked TODO for expansion
- ExperimentRunner::showHistoryEntry() + History section in
  ExperimentsPanel.qml

Metric epoch:
- MetricTensor owns m_epoch, bumped by setMass/setSpin/setCharge
- KerrMetric/SchwarzschildMetric setMass call bumpEpoch (in-place
  mutation is now safe; rebuild-per-sample path retired)

Tests:
- MetricEpoch, ChargeSweep, MercuryPrecession, FrameDraggingExperiment
- All existing (SpacetimeGps, ExperimentRunner, UiWiring) still green
```

Do **not** commit build artifacts (`.pyd`, `.onnx`, `graphics_api.txt`) — they were reverted in this session.

## Still genuinely deferred (beyond this feature)

| Item | Why deferred |
|------|--------------|
| PDF export of experiment results | csv/json only; `PaperGenerator` exists but not wired |
| Cross-session history persistence | `history()` is in-memory only |
| In-place mass mutation + grid recompute | Epoch done; grid recompute still needs wiring |
| Experiment expansion beyond 10 | 4 skeletons remain |
| Qt 6.11 `0xC0000409` canary crash | Per `AGENTS.md`, a Qt runtime/dependency issue, not project code |
