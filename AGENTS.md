# QuantumVerse Agent Instructions

## Project Overview

**QuantumVerse** is a production-ready, 4D spacetime cognition laboratory that enables users to navigate, visualize, and discover new physics in a four-dimensional Lorentzian manifold. It combines **general relativity**, **quantum gravity engines**, **AI-driven discovery**, and **immersive 4D visualization** into a single interactive platform.

Key capabilities:
- True 4D navigation with 6 rotation planes (SO(4))
- Real-time Einstein field equations with validated GR predictions
- Quantum gravity engines: CDT, Spin Foam (LQG), Group Field Theory, Causal Sets
- AI-driven discovery: symbolic regression, anomaly detection, autonomous hypothesis generation
- Multi-messenger pipeline: LIGO (GW), IceCube (neutrinos), TESS (exoplanets), Fermi GBM (GRBs)
- Differentiable physics with gradient-based optimization
- Holographic duality research (AdS/CFT)
- VR support via OpenXR
- Qt 6.11 QML UI (`quantumverse_qml`)

## Repository Structure

```
src/
├── spacetime/         # 4D events, metrics, curvature invariants
├── physics/           # Geodesics, singularities, Hawking radiation, GR raytracing
├── rendering/         # OpenGL 4.5 curvature and celestial rendering
├── ui4d/              # 4D UI coordinator, QML viewport, Planck microscope
├── math/              # Vector4D, Matrix4x4, AutoDiff, SO(4) rotations
├── discovery/         # Discovery instruments, TheoryManager, TheoryDiscoveryAgent,
│                      # SymbolicMath, HolographicDualityLab, FindingsModel
├── quantumgravity/    # CDT, Spin Foam, GFT, Causal Sets
├── ml/                # FeatureVector, AnomalyDetector, AnomalyMonitor,
│                      # TrainingDataCollector, GeodesicNeuralODE, MetricGNN,
│                      # CurvatureNormalizingFlow, DifferentiableSimulator
├── scenario/          # Scenario, ScenarioManager
├── data/              # Multi-messenger adapters (LIGO, IceCube, Fermi, Swift, TESS)
├── config/            # Configuration loaders
├── net/               # Networking utilities
├── audio/             # Spatial audio
├── vr/                # VR multi-user server/client
└── utils/             # Shared utilities
```

## Development Workflow

1. Write failing test first
2. Implement minimal code to pass
3. Run sanitizers when available: `cmake -DQUANTUMVERSE_USE_ASAN=ON`
4. Run verification and differential tests
5. Commit with clear message referencing tests

## Build & Test

### Prerequisites
- Windows 10/11, MSVC 2022, CMake 3.25+
- Qt 6.11.1 (msvc2022_64)
- Optional: ONNX Runtime 1.27.0, CUDA 12.x, Python 3.10+ (ML training only)

### CI vs. local Qt version
CI uses Qt 6.8.3 (mirror availability). Local dev uses 6.11.1.
QML behavior may differ; verify any QML change against both if it
touches layout, popup behavior, or parser-level features.

### Build
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_BUILD_TESTS=ON
cmake --build build --parallel
```

### Run Tests
```bash
ctest -C Release --output-on-failure
```

### Assertions Must Survive NDEBUG

**Tests must be built in Debug, or use `QV_CHECK`/`QV_CHECK_NEAR` from
`tests/test_assert.h`.** `assert()` is compiled out under Release `/DNDEBUG`
and silently passes — a Release build of the test suite asserts nothing. Every
"all green" measured with `assert()` was vacuous until 2026-09-11, when the
assert→QV_CHECK sweep (commit `d381c97`) revealed 9 pre-existing failures that
had been invisible for months. Prefer `QV_CHECK`/`QV_CHECK_NEAR` over `assert()`
in all test code so assertions stay live under any build type.

### Test file pattern is `main()` + `QV_CHECK`, not `TEST()`

Test files use `int main(int argc, char** argv)` with `QV_CHECK` /
`QV_CHECK_NEAR` from `tests/test_assert.h`. There is **no** `TEST()`
macro — a test file beginning with `TEST(...)` will not compile.

Reference implementations:
- `tests/test_cosmology_experiments.cpp` — multiple checks per file
- `tests/test_lensing_time_delay.cpp` — 4 checks (pins + non-vacuity)

Both survive `-DNDEBUG`; the `assert()` → `QV_CHECK` rule above applies
to all assertions inside the `main()` body.

### Tests must be provably non-vacuous

A test that passes because the field it asserts on was never set is
worse than no test — it silently marks regressions as green. Two
concrete cases caught in `test_cosmology_experiments`:

1. `out.scalars[...]` was commented out in `recordCheck`. Tests
   asserting `scalars["x_pass"] == true` compared a default-
   initialized `QVariant(false)` against `bool` — vacuous pass.
2. Negative providers (`BrokenH0`) overrode accessors (`H0()`) but
   the computation used the private member (`m_H0`) directly. The
   "broken" provider was not broken from the computation's
   perspective — vacuous pass.

For every positive test, add a complementary negative test using a
provider/builder that is *actually* broken at the data level, and
assert the specific field is `false`/off. If the negative test
passes, the positive test is vacuous.

### Comparison tests must exercise the relevant parameter regime

A "reference vs project" comparison test at one z-value only validates
that regime. A test at z=1 does **not** validate H(z) at z=1000 —
radiation Ωr(1+z)⁴ is ~0.046% of E² at z=1 but ~31% at z_drag≈1060, so
a missing-radiation bug in H(z) is invisible at low z and catastrophic
at the drag epoch.

When adding a comparison test, choose z-values where the physics being
compared is actually relevant:
- z ≈ 1: growth factor, distance modulus (matter+dark energy dominated)
- z ≈ 100–1000: sound horizon integration, drag epoch (radiation matters)
- z ≈ 1100: recombination (radiation dominates)

The drag epoch (~1059) and recombination (~1100) are separate regimes
from z≈1 growth — each needs its own test point.

Corollary: when a test asserts on a field with a non-trivial getter,
grep the implementation for both `m_field` and `field()` usage. If
they can diverge, the negative test is testing the wrong layer.
Prefer a constructor-injected `Params` struct over settable members
so the divergence is structurally impossible, not just avoided by
convention.

### Run Headless
```bash
build\Release\quantumverse_qml.exe --headless --frames 3 --metric schwarzschild
```

#### Software Rendering (no GPU required)
In headless CI environments or machines without a GPU, use `--software-rendering`
to select Qt's software raster backend instead of OpenGL. This runs the full QML
pipeline (UI controls, Popups, toolbutton bindings, PropertyChanges) but skips
GL-dependent rendering (CelestialBodyRenderer, lensing overlay, curvature renderer).
```bash
build\Debug\quantumverse_qml.exe --software-rendering --frames 1 --metric schwarzschild
```
The flag implicitly sets `--frames 1` if no `--frames` is specified.

### Smoke test pixel assertions

`tests/smoke/test_runtime.cpp` is the authoritative visual regression surface.
Each visual fix should include a pixel assertion in the same commit. The
assertion vocabulary matches the feature type:

| Feature type | Assertion style |
|--------------|-----------------|
| Colored line/wireframe | Color threshold, count > N |
| Colored body | Region average in expected hue range |
| Text | Compare region against a baseline screenshot (hash or SSIM) |
| Gradient | Region has variance > threshold (not flat) |
| Layout | Region is empty (no overflow pixels), or log flag is 0 |

**Prefer log-level assertions when the screenshot captures only a subregion.**
The headless `--screenshot` path writes the viewport FBO, not the full window.
Pixel checks against that image cannot see the QML toolbar. For layout
overflow, assert on the QML-computed `toolbar_overflow` flag in `[REALITY]`
dumps instead. `QQuickWindow::grabWindow()` is unavailable in the offscreen
platform used by `--headless`, so full-window screenshots require a real
display and cannot be used for CI.

Toolbar overflow (`toolbar_overflow`) was verified at 1200px (the
`minimumWidth` floor) during Session 2 — `toolbar_overflow= 0` in [REALITY]
dumps at width 1200. Default width remains 1280. `test_runtime.cpp`'s
`testToolbarOverflowAssertion` covers the default width; the 1200px floor is
covered by the `minimumWidth: 1200` declaration in `main.qml:25`. Add a
regression test that runs at `--window-width 1200` if that CLI flag is ever
added.

### CI Exclusions
Some rendering and performance tests are non-blocking canaries in CI:
- `PerformanceGateTest`
- `QMLPerformanceBaseline`
- `ViewportStateTest`
- `ViewportContentTest`
- `VisualRegressionTest`
- `AnimationTimingTest`
- `GLStrictAuditTest`

Core scientific tests run in primary workflows and must remain green.

### Workflow parity

Multiple workflows run ctest and must maintain the same canary exclusion
list: `ci.yml`, `build-and-test.yml`, `ci-cd.yml`, `cross-platform-test.yml`,
`pr-diagnostics.yml`. When adding or removing a canary, update all of them.
A drifted exclusion list causes main to go red while other workflows pass,
which is hard to diagnose from the failing job alone.

Consider extracting the exclusion list into a shared file (e.g.,
`.github/workflows/_canary_exclusions.txt`) and `cat`-ing it into each
ctest invocation.

### Qt 6 backend switching on Windows
`QT_OPENGL=angle` is a no-op on Qt 6.2+ — ANGLE was removed.
Use `QSG_RHI_BACKEND` instead: `d3d11`, `opengl`, `vulkan`,
`software`. Setting `QT_OPENGL` in a Qt 6 project silently does
nothing and makes backend comparisons misleading.

### MinGW compiler must match Qt's shipped MinGW

Qt 6.8.3 mingw_64 ships with MinGW 13.1.0. Building against it with a
different GCC (e.g. MinGW 16.1.0) produces cross-TU `std::string` /
`std::function` / `QStringList` layout mismatches. Symptom: heap
corruption (`0xC0000374`, `STATUS_HEAP_CORRUPTION`) at process exit,
or crash-on-`QStringList`-destruction inside code that compiles and
runs standalone. The corruption is detected at `free()`, but the write
happened earlier — `gflags /p /enable ... /full` (PageHeap) surfaces
the actual offending write.

Fix: use `C:\Qt\Tools\mingw1310_64\bin\g++.exe`, not
`C:\ProgramData\mingw64\mingw64\bin\g++.exe`. When a test crashes with
`0xC0000374` and "passes standalone", check compiler-vs-Qt MinGW
version before any other hypothesis.

### MinGW 13 on Windows — additional quirks

Three behaviors differ from Linux/GCC expectations:

1. **`std::filesystem::create_directories()` hangs on recursive creation.**
   Symptom: first call to `create_directories("data/sessions/<uuid>/")`
   blocks indefinitely (no CPU, no I/O). Workaround: use a recursive
   `CreateDirectoryA()` helper instead (see `SessionRecorder`). This is
   a MinGW 13 filesystem implementation issue, not a Windows API issue
   — the same call works from MSVC and from Linux GCC 13.

2. **`std::mutex` in a setup-path method deadlocks.** Symptom: first
   `lock()` inside `start()` blocks forever even on a single-threaded
   caller. Workaround: remove the mutex from methods called exactly once
   at init (`start()`, `open()`, `connect()`). Guard only genuinely
   concurrent paths.

3. **`std::function` layout matches Qt's expectation only when the
   compiler matches Qt's MinGW.** (Covered by the existing rule above —
   repeated here because the DLL boundary and the in-process layout are
   the same root cause.)

These three together mean: MinGW 13 works, but not by default. Grep for
`std::filesystem` and `std::mutex` before claiming a "portable" feature
is done.

### GL canary tests bypass backend selection
`test_rendering_diagnostic`, `test_viewport_content_test`, and
`test_viewport_state_test` create `QOpenGLContext` directly instead
of going through QQuickWindow. This means:
- `QT_OPENGL=*` and `QSG_RHI_BACKEND=*` env vars have no effect
- `QT_QUICK_BACKEND=software` has no effect
- The tests will always exercise the desktop GL driver

To make them backend-switchable, mirror `main_qml.cpp`'s
`--software-rendering` flag: gate `QOpenGLContext` creation on a
CLI arg and skip GL init when not requested. Then the RHI/backend
comparison becomes meaningful.

> TODO(canary): the three `0xC0000409` canaries (`test_rendering_diagnostic`,
> `test_viewport_content_test`, `test_viewport_state_test`) are
> `STATUS_STACK_BUFFER_OVERRUN` — the `/GS` cookie firing, i.e. something wrote
> past a stack buffer — **not** plain stack overflow (`0xC00000FD`). Backend
> switching produced identical failures, but these tests create a
> `QOpenGLContext` and use the OpenGL path directly, so the test is not
> conclusive for driver attribution. WinDbg analysis of `non_asan_crash.dmp`
> resolved the attribution: the non-ASan build crashes in
> `Qt6Core!QtPrivate::sizedFree` during
> `QGuiApplicationPrivate::init`/`createPlatformIntegration`, before any test
> rendering code. `FAILURE_BUCKET_ID: FAIL_FAST_FATAL_APP_EXIT_c0000409_Qt6Core.dll!Unknown`.
> This is a Qt 6.11.1 runtime/dependency crash, not project memory safety and
> not a graphics-driver fault. The ASan build additionally shows an ASan runtime
> initialization fault (`e0736170` in `clang_rt_asan_dynamic-x86_64.dll!EnlightenVSDebugger`),
> which is separate from the source canary behavior.
>
> RESOLVED: all three "timeout" tests are slow-but-passing, not hangs.
> Root cause: `test_gradient_optimizer` and `test_theory_discovery_agent`
> both enable `multi_objective_mode_`, making each `evaluateTheory()` call
> trigger `updateParetoArchive()` inside the optimization loop — the cost
> grows super-linearly with iterations. Reduced iteration count in the
> gradient optimizer test; the theory discovery suite completes in ~43s
> with unbuffered markers. `test_adjoint_gradient_optimizer` passes at
> ~384s; timeout bumped to 900s for CI margin.
>
> Underlying performance issue: the production `optimizeWithGradient()`
> loop has the same cost profile. Worth a follow-up issue if users hit
> iteration counts > 20. Not a test-fix blocker.
>
> ### Test-suite speed bounded by curvature computation (profiled 2026-10-03)
>
> Profiling `TheoryDiscoveryAgent::evaluateTheory()` showed ~350ms/call,
> of which ~335ms is `CurvatureCalculator::computeAll()` (Christoffel →
> Riemann → Ricci → Kretschmann). `luminosityDistanceMpc` is ~4% and
> Pareto archive updates are microseconds. Fixing test-suite speed
> requires either intra-call metric memoization in `computeAll()` or
> replacing central-FD gradients with SPSA (2 eval calls per step
> regardless of parameter count, vs 2N). Pareto optimization and
> dL caching are not the bottleneck. Current test times are 44–74s
> per theory-discovery test — acceptable, not pathological.

### qFatal exit codes on Windows
Qt's qFatal / `__fastfail` produces different exit codes depending on build type:
- Release: `0xC0000409` (`STATUS_STACK_BUFFER_OVERRUN` via `__fastfail`)
- Debug:   `0x80000003` (`STATUS_BREAKPOINT` via `__debugbreak`)

**These codes are not specific to Qt init failure.** `0xC0000409` is the generic
`STATUS_STACK_BUFFER_OVERRUN` — the `/GS` cookie firing. In a Qt-linked test it is
*often* `QGuiApplicationPrivate::init` / `QtPrivate::sizedFree`, but that is an
observation about a subset of failures, not a definition. Do not treat `0xC0000409`
as proof of a Qt init problem:

- A test that does **not** link Qt (e.g. `test_full_exact_solutions`,
  `test_extreme_regimes`) cannot be failing in Qt init. Those crash after the test
  body prints its final `[PASS]`, which points at static destructors, `atexit`
  hooks, CRT exit, or a `/GS` / runtime mismatch between the test exe and the
  libraries it links (`dilaton` is `INTERFACE`, so it inherits the consumer's
  flags — verify with `dumpbin /headers ... | findstr GS` on the exe and the
  object files that feed it).
- Diagnosis recipe: append `std::cout.flush(); std::_exit(0);` to `main()`.
  Crash gone  → static destructor / atexit. Still crashes → CRT exit or link
  mismatch.

Do not treat `0x80000003` as a distinct bug.

### SchwarzschildMetric::evaluate() r = 0 contract
`MetricTensor::evaluate()` and `curvatureScalars()` agree that the coordinate
origin is a curvature pole, but they return different things by design:
- `curvatureScalars()` returns `valid = false` — the probe is reported invalid.
- `evaluate()` returns the zero-initialized flat-space array — a NaN-free,
  Inf-free placeholder so downstream numerics do not propagate NaN/Inf.

**Do not reinterpret this as "r = 0 is Minkowski".** The zero array is a
sentinel, not a physical metric; it exists only to satisfy fuzz contracts that
reject NaN/Inf output. If a caller needs a real metric at the singularity it
must check `curvatureScalars().valid` first.

### Changing `evaluate()` semantics requires updating all callers

`MetricTensor::evaluate()` has a documented contract: the returned
matrix is a NaN-free, Inf-free placeholder inside the horizon (see
"SchwarzschildMetric::evaluate() r = 0 contract"). Any change to
*when* the placeholder fires, or *what* it contains, requires
updating every test that calls `evaluate()` near or inside the
horizon.

Concrete case (2026-11): a change to return `diag(-1, 1, 1, 1)`
inside the horizon broke `NumericalConvergenceTest`, which still
expected `g_tt = 0` at `r = r_s` (the old behavior). The test was
correct for the old code, wrong for the new code, and nobody
re-ran it before pushing.

Rule: when you change a public API's return contract, grep for all
callers of that API and update their assertions in the same commit.
`git grep 'evaluate(' tests/` should show you the full surface.

### Qt 6.11 QML parser quirks

- `function` declarations are not hoisted. Declare them before any visual
  items that reference them via bindings, `onClicked`, etc. Symptom: parse
  errors ("Cannot assign to non-existent property") or silent runtime failures.
- Avoid `parent: ApplicationWindow.overlay` inside `Popup` components —
  Qt 6.11 may reject this at parse time. Use explicit anchors instead.
- Two `RowLayout`s directly inside a `ColumnLayout` inside a `Popup` may
  fail to parse — wrap each `RowLayout` in an `Item` container.

### Every `Pane` in a dark-themed app needs an explicit `background`

`Pane` under Qt Quick Controls Basic renders a light-grey background
(`#f6f6f6`-ish) by default. In a dark-themed application, this appears
as visible grey boxes wherever a Pane's internal `RowLayout`s don't
stretch to fill the pane width — and the source is invisible in `grep`
because the color value lives in Qt's internal style sheet, not in
`main.qml`.

Rule: any `Pane` in this app must set `background: Rectangle { ... }`
with a dark color, or use the shared `SectionPane` component. `grep -c
"Pane {" src/*.qml` should equal `grep -c "background:" src/*.qml` for
panes that intentionally render a background.

Concrete case (2026-11): the `Parameter Sweep` pane showed "two grey
UI elements on the right side" that were really the default Pane
background showing through gaps in the layout. Nobody could find them
by grepping for `color:` or `Rectangle` because the color was implicit.

### `Layout.fillWidth` only works inside `Layout` parents

`Layout.fillWidth: true` on a child of a `ScrollView`, `Flickable`,
`Item`, or any non-Layout container has no effect — the container's
implicit width is the max of children's implicit widths, so the child
sits at its natural size regardless.

For a `ColumnLayout` inside a `ScrollView`:
    ScrollView {
        contentWidth: availableWidth
        ColumnLayout { width: availableWidth }
    }

Concrete case (2026-11): right panel panes were 240px wide inside a
380px ScrollView because `Layout.fillWidth` was inert. Pane backgrounds
stopped at 240px; inner RowLayouts extended past them; the ScrollView's
default light-grey background showed through the gap — looked like
"empty grey boxes to the right of the panel" but was actually the
missing width propagation.

### DiscoveryPanelManager moc note

`Q_OBJECT` is inside `#ifdef QUANTUMVERSE_USE_QML` in
`DiscoveryPanelManager.h`. Builds without that define produce a benign
AutoMoc note: `No relevant classes found. No output generated.` This is
a build configuration artifact, not a defect. Do not chase.

## Testing & Verification

### Test Categories
- Unit tests: `tests/test_*.cpp`
- Integration tests: `tests/test_integration*.cpp`
- Validation tests: GR benchmarks (Mercury precession, light deflection, redshift, frame dragging, Nordtvedt)
- Discovery tests: `tests/discovery/test_*.cpp` with 6+ TDD checks per instrument
- ML tests: anomaly detector, monitor, training data collector
- VR tests: `tests/unit/test_shared_session.cpp`, `test_signaling_client.cpp`, `test_multi_user_server.cpp` — headless, no OpenXR runtime required

### Physics Invariants Monitored
1. MetricTensor symmetry: `g[μ][ν] = g[ν][μ]`
2. Lorentzian signature: `(-,+,+,+)`
3. Negative determinant: `det(g) < 0`
4. Energy conservation along geodesics
5. Angular momentum conservation in central potentials
6. Timelike separation for physical trajectories

### Adding New Verification Tests
1. Add verification functions to `src/verification/` or the relevant module test file
2. Register test in `CMakeLists.txt`
3. Run `ctest -R <Name>Test --output-on-failure`

## Discovery Instruments

The project includes discovery instruments for multi-messenger astrophysics and theory exploration:

| Domain | Instruments |
|--------|-------------|
| **GW Detection** | `MergingBinaryInspiralAnalyzer`, `BosonStarCollisionPredictor`, `GWRingdownScanner` |
| **BH Physics** | `GWEchoHunter`, `ECORingdownAnalyzer`, `KerrNoHairViolationAnalyzer`, `HawkingRadiationDetector`, `GravitationalWaveMemoryAnalyzer` |
| **Multi-Messenger** | `NeutrinoBurstAnalyzer`, `KilonovaAfterglowScanner`, `FastRadioBurstAnalyzer` |
| **Cosmology** | `CMBLensingScanner`, `CosmicShearScanner`, `PTAScanner`, `RecombinationConstantVariationImager` |
| **Dark Matter** | `UltralightDMWaveInterferometer`, `DarkMatterAnnihilationAnalyzer`, `PBHMicrolensingScanner` |
| **Theory** | `TheoryDiscoveryAgent`, `HolographicDualityLab`, `SymbolicMath` |

Each instrument follows the `DiscoveryInstrument` base class pattern with:
- `analyze()` method taking `MetricTensor&`, `Event4D&`, `vector<Event4D>&`
- `getParameterRanges()` for QML dashboard integration
- TDD-verified tests in `tests/discovery/`
- Registration in `src/discovery/DiscoveryEngine.cpp` or `src/main_qml.cpp`

### Recent Analyzer Improvements
`MergingBinaryInspiralAnalyzer` includes:
- Median + 3σ outlier rejection on the frequency track
- Two-stage coarse-to-fine grid search centered on the best coarse estimate
- Improved SNR recovery from ~3 → ~770 for marginal signals
- Edge-case tests covering boundary chirp masses, off-grid recovery, short tracks, noise rejection, and seed consistency

### Adding New Instruments
1. Create `src/discovery/<Name>.h/.cpp` following existing patterns
2. Create `tests/discovery/test_<name>.cpp` with 6+ TDD checks
3. Add to test target in `CMakeLists.txt`
4. Register in discovery engine or QML entry point
5. Build and run: `ctest -R <Name>Test --output-on-failure`

## ML / Differentiable Physics

The `src/ml/` module provides C++ runtime inference and differentiable physics:
- `FeatureVector` / `AnomalyDetector` / `AnomalyMonitor`: real-time anomaly scoring
- `TrainingDataCollector`: telemetry logging for retraining
- `GeodesicNeuralODE`: Neural ODE surrogate for geodesic prediction
- `MetricGNN`: graph neural network for metric learning
- `CurvatureNormalizingFlow`: density estimation on curvature space
- `DifferentiableSimulator`: gradient-based spacetime optimization

Python training scripts live in `python/` and exported ONNX models are loaded at runtime.

## CI/CD

Primary workflows:
- PR Diagnostics
- Cross-Platform Sanitization Pipeline
- CI
- Windows Validation
- Advanced Error Discovery Pipeline

Nightly workflows (`Nightly Baseline Update`, `Nightly Deep Scan`) are allowed to fail without blocking mainline development. If they fail, check for environment-specific issues such as missing secrets or hardware differences before investigating code changes.

### Checking workflow status: `--json conclusion` vs `--log-failed`

`gh run view <id> --log-failed` shows any log line that *looks* like a
failure — including informational output from steps that passed, canary
steps with `continue-on-error: true`, and CTest lines emitted under
`--output-on-failure` even when the test passed.

`gh run view <id> --json conclusion,jobs --jq '.jobs[].steps[] | select(.conclusion=="failure") | .name'`
returns the ground truth: the *step names* that actually failed.

For release verification, use the JSON form. For debugging a red
workflow, `--log-failed` is useful context but not the verdict.

This distinction caused three rounds of "the run failed / no it didn't"
on v7.4.0's CI/CD run.

### Windows App Execution Alias trap

Windows App Execution Alias stubs (e.g. `%LOCALAPPDATA%\Microsoft\WindowsApps\python3.exe`) launch the Microsoft Store instead of a real interpreter and cause `std::system`/`_popen` calls to stall for minutes. Always reject paths containing `WindowsApps`; see `SymbolicMath::resolveExecutablePath` as the reference implementation.

## Known Issues

- Windows COM exception `0x8001010D` may appear in headless/CI output; it is benign and caught by the VEH.
- Some rendering/performance tests are non-blocking canaries because they depend on GPU/driver state in CI.
- The inspiral analyzer’s `fitTaylorF2()` uses a fixed frequency model; extremely low-frequency or very short tracks may yield low-confidence detections.
- ONNX Runtime and CUDA acceleration are optional; runtime inference falls back to CPU if unavailable.
- **Probe cache assumes a static metric.** `QmlGlViewport::m_probeCache`
  (`qmlglviewport.h:973`, `qmlglviewport.cpp:2358`) keys on
  `(metric.get() pointer, position)`. This is safe only because
  `m_probeMetric` is a static analytic Kerr/Schwarzschild metric with no
  mutators (`KerrMetric.h:35-36` exposes `mass()`/`spin()` getters only);
  constructed once at `main_qml.cpp:513/518`, handed to the viewport at
  `:1086`. If a time-dependent N-body metric is ever wired into
  `m_probeMetric`, the pointer stays stable while body positions evolve and the
  cache returns **stale curvature scalars**. Fix in that case: add a
  `uint64_t m_metricEpoch` to the metric owner, bump it whenever underlying
  state changes, and include the epoch in the cache key. Do not wire a live
  N-body metric into `m_probeMetric` without this. Cache is invalidated in
  `setProbeMetric()` and `clearProbe()`.

- **Quantum geometry FOV mismatch.** `QmlGlRenderer::renderQuantumGeometry()`
  (`qmlglviewport.cpp:1770`) built a `Camera4D` with
  `fov = 2.0 * std::atan(1.0 / m_camera4DAdapter->distance())`. At the headless
  default `cam_d=150` that FOV is **0.76°** — visible width ~2 units. The CDT
  geometry lives in `[0,100]³`, ~86 units from the camera, so almost no points
  fell inside the frustum (4 magenta pixels out of 1000). The rest of the scene
  (CurvatureRenderer, CelestialBodyRenderer) uses a fixed 45° FOV
  (`qmlglviewport.cpp:222,434,835,3032`). Fix: use `M_PI / 4.0` in the QG path
  to match. Verified: 4 → 1297 magenta pixels, `RuntimeSmokeTest` 7/7 green.
  Do not reintroduce the distance-derived FOV here — it only made sense for a
  probe microscope, not the main viewport.

- **Redshift `n/a` at simTime ≈ 51.76 (2026-10-07) — not reproducible.** One
  historical log showed `gtt=+43.26` at a fixed probe position, implying an
  ~1500 M⊙ mass mutation after asteroid injection. 5000-frame headless orbits
  with injection show `gtt=-0.7046 branch=1` throughout. Instrumentation
  (`[MASS]`, `[REDSHIFT-DEBUG]`, `[PROBE]`) is in place to catch a recurrence.

- **Lensing shader camera convention fix (2026-10-19):** The half-frame split
  in Kerr black hole shadow rendering was caused by a camera/basis mismatch
  between the lensing shader and the QML viewport camera, not by `u_bhPos`
  (which was correctly wired: `setGridBlackHoleCenter` → `setBlackHolePosition`
  → `setUniformVec3("u_bhPos", ...)`) nor by RK4 (which is a valid integrator
  for the Kerr orbit equation). Three issues were found and fixed:

  1. **Theta convention:** `--cam-el 0` means "on the equator", which is θ=π/2
     in the shader's spherical coordinate system (θ measured from +Y pole).
     Was passing `cliCamElevationDeg * π/180` directly (0 → pole). Fixed to
     `(90.0 - cliCamElevationDeg) * π/180` so el=0 → θ=π/2 (equator).

  2. **Camera world position:** The lensing shader previously reconstructed
     `u_cameraPos` from spherical coordinates around the world origin
     (`d·(sinθcosφ, cosθ, sinθsinφ)`), while the QML viewport camera orbits
     around the target (`target + d·offset`). When `target != origin`, this
     caused an off-center shadow. Fixed by feeding the QML camera's actual
     world position via `setLensingCameraPos()` (computed as
     `target + (d·cos(el)·sin(az), d·sin(el), d·cos(el)·cos(az))`,
     matching `Camera4DAdapter::cameraPosition4D()`), and using
     `hasCameraPos=true` to bypass spherical reconstruction.

  3. **Camera direction/basis:** `u_cameraDir` was previously
     `-normalize(cameraPos)` (looking at world origin), which degenerated to
     `(0,0,-1)` when camera is at origin and pointed the wrong way at other
     positions. Fixed to `normalize(bhPos - cameraPos)` (looking at the BH),
     with `u_cameraUp` built from an orthonormal basis (`right = cross(forward,
     worldUp)`, `up = cross(right, forward)`). The GLSL ray basis
     (`lensing_fragment.glsl:465-468`) consumes `u_cameraDir`/`u_cameraUp`
     directly — no `theta`/`phi` in the basis block.

  Debug logs added:
  - `[LENS]` in `GravitationalLensing.cpp:updateUniforms()` prints
    `u_cameraPos`, `u_bhPos`, `distance`, `theta`, `phi`, `hasCameraPos`,
    `u_cameraDir` — verifies uniform values on this machine without GL.
  - `[LENS-VERIFY]` in `main_qml.cpp` compares `camera4DAdapter->cameraPosition4D()`
    (real QML controller state) against the lensing camera position —
    bit-for-bit match confirms no circular recompute.

  GPU capture gate (machine with display): run
  `--cam-az 90 --cam-el 0 --cam-dist 40 --cam-tx 40 --screenshot` and watch
  for green pixels (step budget), off-center shadow (position/aim), or
  persistent half-split (basis/stale shader). `[LENS]` logs are CPU-side
  uniform values; driver-side uniform upload or stale shader program remain
  the only unclosed gap.

- **Schwarzschild curvatureScalars() rs bug (2026-10-19):** `curvatureScalars()`
  computed the Schwarzschild radius as `rs = 2GM/c⁴` instead of `2GM/c²`, making
  `rs ≈ 1e-14 m` for 1 M☉ instead of `≈ 2952 m`. The `r ≤ rs → invalid` check
  never fired, and the `r_eps = 0.01·rs` floor clamp was effectively zero, so
  neither safety net protected against `K → ∞` at `r → 0`. Fixed `rs` to use
  `c²` (not `c⁴`), keeping both the `r ≤ rs → invalid` check and the `r_eps`
  floor clamp active. Restored `r ≤ rs → valid=false` inside the horizon and
  added `r_eps = 0.01·rs` floor clamp just outside it.

- **Mass sweep clamping (2026-10-19):** `setBlackHoleMass()` now clamps to
  `[0.1, 10.0] M☉` matching the UI sweep range. Values outside this range
  (e.g. from a stale tour script or uninitialized sweep state) are clamped
  before reaching the metric, preventing `K → ∞` from extreme mass values.

- **TX/TY/TZ camera basis fix (2026-10-19):** `Camera4DAdapter::rotateInPlane()`
  for boost planes (TX/TY/TZ) previously used `coshR * offset[axis]` with
  `sinhR` computed but unused (`(void)sinhR`), and only scaled one component.
  With `tx=ty=tz=0`, this produced a zero offset — dragging did nothing.
  Replaced with a call to the existing `rotateInPlane()` implementation which
  correctly applies the Lorentz boost mixing time and space components, and
  updates `azimuth`/`elevation` when `spatialLen > 0`. Verified that
  `viewMatrix()` remains affine (no NaN/Inf) after rotation.

## Configuration Options

| Option | Default | Description |
|--------|---------|-------------|
| `QUANTUMVERSE_USE_ASAN` | OFF | AddressSanitizer |
| `QUANTUMVERSE_USE_TSAN` | OFF | ThreadSanitizer |
| `QUANTUMVERSE_USE_UBSAN` | OFF | UndefinedBehaviorSanitizer |
| `QUANTUMVERSE_USE_FUZZER` | OFF | libFuzzer targets |
| `QUANTUMVERSE_USE_BOOST_MULTIPRECISION` | OFF | Formal verification tests |
| `QUANTUMVERSE_ENFORCE_TIDY_ERRORS` | ON | Treat clang-tidy warnings as errors |
| `QUANTUMVERSE_USE_COVERAGE` | OFF | Enable code coverage instrumentation |

## Module Layout Convention

New code belongs under `src/<module>/` where `<module>` is one of:
`spacetime`, `physics`, `rendering`, `ui4d`, `discovery`, `quantumgravity`, `data`, `ml`, `math`, `scenario`, `audio`, `vr`, `net`, `utils`, `config`.

Tests belong in `tests/` and examples in `examples/`.

### Convert-asserts tooling is unsafe

The `d381c97` sweep mangled:
- Scientific literals: `1.2e-7` -> `1.2e, 7` or `1.2e<sub>...</sub>`
- Arrow members: `it->second` -> `it, >second`
- Argument order: `assert(fabs(a-b) < c)` -> `QV_CHECK_NEAR(a-b, c, ...)`
  instead of `QV_CHECK_NEAR(a, b, c)`

Do not re-run it. Any future conversion must be manual or use a
literal-aware parser with tests.

The 12-file repair landed in `c7dd514`.

### Simulation appears frozen (bodies don't move)

If the simulation is advancing but bodies are not moving, check these four
layers in order. Each maps to a distinct root cause fixed in a separate commit:

| Layer | Check | Commit |
|-------|-------|--------|
| Config load | `[DataPaths]` resolves to source tree, not `build/` | `81da060` |
| Physics loop | `QmlGlViewport::updateSimulation()` calls `m_ui4d->stepSimulation()` | `c2fb9f4` |
| Initial conditions | `initializeSolarSystem()` writes velocity into `.y`, not `.z` | `98e80ea` |
| Renderer read | Renderer uses `body.position`, not `body.orbitPoints.back()` | `00e113c` |

The [REALITY] diagnostic dump (emitted every 60 sim frames) shows `bodies=9`,
`gridZRange`, and `simTime`. If bodies=9 but positions are static, the break
is in layer 2, 3, or 4. If bodies<9, the break is in layer 1.

Update 2026-10-14: the frozen-body symptom that motivated this table was
actually the physics clock, fixed in `d3962e0` (single clock on every path,
physics dt = interval x timeScale). The layers above remain valid checks, but
check the clock first: `[N-BODY]` instrumentation in
`UI4D::stepSimulationCPU` logs substep state per call.

### Stepping paths (post-`d3962e0`)

Verified empirically (interactive mode, 5-second run): `simTimer` and
`updateSimulation` → `stepSimulation` fire at identical counts (~1315 each),
`updateSolarSystemPositions` fires 0 times. **No double-step bug exists.**

`stepSimulation` has exactly one active caller path:
- `QmlGlViewport::updateSimulation()` (`src/qmlglviewport.cpp:2411`) — called
  from QML `simTimer` in interactive mode (`main.qml:1569`) and from C++ headless
  loop (`main_qml.cpp:1526`). Inside `updateSimulation`, `stepSimulation(deltaTime)`
  is called at line 2424. One clock per path: interactive uses QML timer,
  headless uses C++ loop, never both.

`UI4D::updateSolarSystemPositions()` (`src/ui4d/UI4D.cpp:597`) is **dead code** —
declared in header but never called from C++ or QML (not Q_INVOKABLE, zero
callers). It calls `stepSimulation(dt, 100)` but is unreachable.

`updateSimulation()` is the single intended funnel. Before routing any new code
through `stepSimulation` directly, confirm none of these three paths double-step
the same frame.

> NOTE: the viewport call at `qmlglviewport.cpp:2424` runs inside
> `updateSimulation()`, not inside `render()`. It is called exclusively from
> QML `simTimer` in interactive mode. No double-step with the timer.

### Two different "up" axes: N-body orbits in XY, annulus disk in XZ

The codebase contains two incompatible plane conventions, and they are
silently in conflict with each other:

| System | Orbital plane | Up axis | Angular momentum |
|--------|--------------|---------|------------------|
| N-body planets (`UI4D::initializeSolarSystem`) | physical XY | physical Z | +Z (`angularMomentumZ` = m(x·vy − y·vx) is correct for this plane) |
| Annulus/disk (`CelestialBodyRenderer` `celestialFragSrc`) | world XZ | world +Y (`OMEGA_AXIS`) | +Y |

The N-body convention comes from `Event4D(t, x, y, z)`:
`initializeSolarSystem()` sets `body.position = Event4D(0, r, 0, 0)` (position
on +X) and `body.velocity = Event4D(0, 0, -v, 0)` — the third constructor
argument lands in `.y`, so the planets orbit in XY with angular momentum along
+Z. The annulus deliberately lives in world XZ: `radial = vWorldPos.xz -
bhCenterWorld.xz`, `v_dir = normalize(cross(OMEGA_AXIS, radial))`, verified
radius range [0.72, 1.80].

Consequences to respect before any feature that depends on "which way is up":
- Comparing planet orientation to disk orientation compares different planes.
- The disk's angular momentum (+Y) and the planets' (+Z) point along different
  world axes; do not add them or feed one into the other's formula.
- A future "show the orbital plane" / "show the disk normal" overlay must pick
  the axis explicitly per system.

Both conventions are load-bearing and verified: the N-body orbits match
analytic two-body to 0.00-0.07% (in XY), and the annulus Doppler work is
verified in XZ. Unifying them is a broad, deferred refactor — do it before,
not during, the next feature that depends on the axis.

### Open: metric rebuilt on every mass change (not blocking)

The mass sweep rebuilds the metric object and swaps the pointer on every
sample (`[MASS] old= ... new= ... probeMetric= ...` /
`[PROBE] metric swap old= ... new= ...`, ~15 swaps per sweep batch).
Correctness is unaffected — the probe cache is invalidated on each swap
(`setProbeMetric()`), and the N-body system never sees the BH — but each
rebuild allocates and invalidates the curvature renderer's cached state, and
`CurvatureRenderer::gridZRange` is only recomputed at startup, so the grid's
Z deformation does not follow the mass. Refactor scheduled: update the mass
in place (with the cache invalidated) instead of rebuilding, and recompute
the grid on metric change. Fix together if they are the same defect — a grid
that reads a stale metric pointer and a sweep that reallocates one per
sample are two views of the same lifetime problem.

### Orbit-equation RK4 deflection at b=6M — value confirmed correct

`conv_run.cpp` orbit-equation RK4 (Schwarzschild null geodesic, `u'' + u = 3Mu²`):

- b=12M: converged `0.4526` vs Python `solve_ivp` reference `0.453236` — matches within rounding. Commit with this result.
- b=6M: converged `1.7194` vs Python `solve_ivp` reference `1.719458` — exact match. **The code is correct.**

The earlier `~1.47` expected value came from Bozza's strong-deflection limit evaluated at `b/b_crit = 1.15` — outside the SDL's validity range (the `O(x log x)` correction is not negligible at 15% above `b_crit`). Bozza's actual paper table gives `1.6–1.8` at `b/b_crit = 1.15`.

**No code fix needed.** Test 28 in `tests/test_lensing.cpp` was a smoke test (`alpha > 1.65`) that masked the same caller-side formula bug as Test 29; both now use `acos(b/r_start) + acos(b/r_end)` with actual trajectory endpoints. Test 28 passes at b=6M (deflection `~1.71` vs Python `1.719458`), Test 29 passes at b=12M (deflection `0.4485` vs Python `0.4532`). The earlier `1.886` (b=6M) and `0.784` (b=12M) values were produced by subtracting `2*atan(r0/b)`, which assumes `r_end = r_start` but `integrateSimple` runs exactly `steps` RK4 iterations without terminating on escape — `r_end` is whatever the step budget reaches.

### Orbit-equation RK4 deflection at b=6M — value confirmed correct

`conv_run.cpp` orbit-equation RK4 (Schwarzschild null geodesic, `u'' + u = 3Mu²`):

- b=12M: converged `0.4526` vs Python `solve_ivp` reference `0.453236` — matches within rounding. Commit with this result.
- b=6M: converged `1.7194` vs Python `solve_ivp` reference `1.719458` — exact match. **The code is correct.**

The earlier `~1.47` expected value came from Bozza's strong-deflection limit evaluated at `b/b_crit = 1.15` — outside the SDL's validity range (the `O(x log x)` correction is not negligible at 15% above `b_crit`). Bozza's actual paper table gives `1.6–1.8` at `b/b_crit = 1.15`.

**No code fix needed.** Test 28 in `tests/test_lensing.cpp` was a smoke test (`alpha > 1.65`) that masked the same caller-side formula bug as Test 29; both now use `acos(b/r_start) + acos(b/r_end)` with actual trajectory endpoints. Test 28 passes at b=6M (deflection `~1.71` vs Python `1.719458`), Test 29 passes at b=12M (deflection `0.4485` vs Python `0.4532`). The earlier `1.886` (b=6M) and `0.784` (b=12M) values were produced by subtracting `2*atan(r0/b)`, which assumes `r_end = r_start` but `integrateSimple` runs exactly `steps` RK4 iterations without terminating on escape — `r_end` is whatever the step budget reaches.

**Resolved:** `GeodesicIntegrator` produces correct deflection at b=6M (~1.71 vs 1.719) and b=12M (0.4485 vs 0.4532). The integrator is correct at both impact parameters; neither discrepancy was integrator-side.

---

## v7.2 Release (2026-10-26)

### Release Summary

| Item | Value |
|------|-------|
| **Tag** | `v7.2-final` (on `main`) |
| **Release** | Immutable, published 2026-10-26T23:51:52Z |
| **Installer** | `QuantumVerse-7.2-Setup.exe` (~32.4 MB) |
| **Download** | `https://github.com/Kravors/QuantumVerse-Simulator/releases/download/v7.2-final/QuantumVerse-7.2-Setup.exe` |
| **Install location** | `%LOCALAPPDATA%\Programs\QuantumVerse\` (non-admin) |
| **Source commit** | `c8c2d03` |
| **Commit message** | `chore: ignore training_data and *.pyd binaries` |

### v7.2 Changes
- **Grid deformation**: Fixed mass unit slip in vertex dispatch (`kg→solar-mass`), grid now deforms (`gridZRange = -6.4..-0.0708`, was `-1..-1`)
- **Kerr observables**: Verified `K=6.973e-21`, `z=0.1175` against full Kerr formulas at r=1e5m, θ=0, a/M=0.9 (3+ sig figs)
- **Discovery paper pipeline**: scan → abstract → PDF at `Documents\QuantumVerse\output\discovery_paper.pdf`
- **Non-admin installer**: 32.4 MB unsigned NSIS installer at `%LOCALAPPDATA%\Programs\QuantumVerse\`; excludes `.h5/.hdf5/.pth/.ckpt/.onnx` (852 MB removed)
- **Metric label honesty**: ComboBox shows actual type ("Kerr"), spin display shows actual spin (0.9)
- **Physics validator**: All 5 checks pass; red banner eliminated; Kretschmann mass unit double-conversion fixed
- **Probe location**: Moved from r=0 to r=1e5m, θ=0
- **Schwarzschild rs bug**: Fixed `rs = 2GM/c⁴` → `2GM/c²` (was 1e-14 m for 1 M☉, now ≈2952 m)
- **Mass sweep clamping**: `setBlackHoleMass()` clamped to `[0.1, 10.0] M☉`
- **Lensing shader**: Camera convention fix — theta convention, camera world position, camera direction/basis

### Tag Naming Note
The tag `v7.2-shipped` was pushed to GitHub but bound to an immutable release with no assets. A new release `v7.2-final` was created targeting `main` (commit `c8c2d03`) with the installer attached. The source commit is the same; only the tag/release name differs due to GitHub immutability rules preventing reuse of `v7.2-shipped` for a new release.
