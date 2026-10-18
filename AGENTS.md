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

### Qt 6.11 QML parser quirks

- `function` declarations are not hoisted. Declare them before any visual
  items that reference them via bindings, `onClicked`, etc. Symptom: parse
  errors ("Cannot assign to non-existent property") or silent runtime failures.
- Avoid `parent: ApplicationWindow.overlay` inside `Popup` components —
  Qt 6.11 may reject this at parse time. Use explicit anchors instead.
- Two `RowLayout`s directly inside a `ColumnLayout` inside a `Popup` may
  fail to parse — wrap each `RowLayout` in an `Item` container.

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

- **Lensing shader was non-functional until commit `00fd554` — never compiled.**
  `loadLensingFragmentSource()` used `fs::current_path()` (wrong directory), so
  `m_lensingProgram` stayed 0; the shader also had pre-existing GLSL errors
  (`atan2` undefined, `delta` undefined). All prior lensing behavior claims were
  the fallback path, not the shader. Fix: `__FILE__`-relative path, `atan2`→`atan`,
  removed undefined `delta`, added `u_bhPos` uniform so ray origin is
  `u_cameraPos - u_bhPos` (BH at world origin → ring at screen center). Verified:
  ring at screen center within ~2px across cam-dist 20/40/80; Test 23b still
  passes (CPU `computeVolumetricDiskEmissivity()` unchanged); steady-state ~28
  FPS in Debug at 1024 raySteps.

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

`stepSimulation` has two direct callers outside `updateSimulation`:
- `UI4D::updateSolarSystemPositions()` (`src/ui4d/UI4D.cpp:600`) — called for backward compatibility
- `QmlGlViewport::render()` (`src/qmlglviewport.cpp:2424`) — steps every frame

`updateSimulation()` is the single intended funnel. Before routing any new code through
`stepSimulation` directly, confirm none of these three paths double-step the same frame.

> NOTE: the viewport call at `qmlglviewport.cpp:2424` runs every frame alongside the QML `simTimer`
> in interactive mode. Confirm it doesn't double-step with the timer before adding any feature that
> depends on absolute sim-time per frame — that's exactly the class of bug `d3962e0` eliminated.

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

**Resolved:** `GeodesicIntegrator` produces correct deflection at b=6M (~1.71 vs 1.719) and b=12M (0.4485 vs 0.4532). The integrator is correct at both impact parameters; neither discrepancy was integrator-side.
