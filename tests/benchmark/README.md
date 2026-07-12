# QML Performance Baseline

`qml_performance_baseline` is a headless smoke/throughput test for the QML
render path (`quantumverse_qml` + `QmlGlViewport`). The contract logic lives
in `perf_gate.h` and is exercised directly by `tests/test_performance_gate.cpp`.

## What it does

1. Launches `quantumverse_qml --headless --frames 50`.
2. The application runs a repeating `QTimer` (16 ms interval) that counts
   frames and, once the target is reached, writes timing statistics to
   `headless_performance.log` and exits cleanly.
3. The test parses the log line
   `Average frame time: <avg> ms, Max: <max> ms, ...` and asserts the
   measured average and maximum per-frame times stay under thresholds.

## Important: headless measures event-loop throughput, not GPU performance

In headless/`QT_QPA_PLATFORM=offscreen` mode there is **no real OpenGL
context** (`renderGL()` early-returns before `glad` initializes), so this
test does **not** measure GPU frame time. It measures the Qt event-loop /
scene-graph tick cadence under the offscreen platform. Typical results are
`avg ~16 ms`, `max ~30 ms` (≈60 FPS), dominated by per-tick event-loop
overhead rather than rendering work.

## Thresholds

| Metric | Threshold | Rationale |
|--------|-----------|-----------|
| Average frame time | `< 60 ms` | Wide margin over the observed ~16 ms baseline to catch gross regressions (e.g. a runaway timer or blocking work on the UI thread). |
| Maximum frame time  | `< 80 ms` | Catches occasional long ticks (GC, disk flush, QML compile) without being flaky. |

These are CI baselines, not performance contracts. If the headless path is
later wired to a real GL context, tighten the thresholds to reflect true
render cost.

### Configuring the thresholds

Thresholds are overridable **without recompiling**:

- **CMake cache variables** (compile-time defaults):
  ```bash
  cmake -B build -DQUANTUMVERSE_USE_QT=ON \
    -DCTEST_PERFORMANCE_THRESHOLD=60 \
    -DCTEST_PERFORMANCE_MAX_THRESHOLD=80
  ```
- **Environment variables** (override at run time, take precedence):
  ```bash
  set QUANTUMVERSE_PERF_AVG_THRESHOLD_MS=50
  set QUANTUMVERSE_PERF_MAX_THRESHOLD_MS=70
  ```

Both the baseline test and `tests/test_performance_gate.cpp` read the same
defaults via `perf_gate::thresholdFromEnv`.

## Running

```bash
cd build_qml
set PATH=%PATH%;F:\qt\6.11.1\msvc2022_64\bin
set QT_QPA_PLATFORM=offscreen
ctest -C Release -R "Performance" --output-on-failure
```

The `Performance` label selects both `QMLPerformanceBaseline` (end-to-end)
and `PerformanceGateTest` (gate-contract unit test). Exit code `0` = pass.
The run should complete in a few seconds.
