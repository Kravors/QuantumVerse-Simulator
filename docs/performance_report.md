# Performance Report — QML Render Path (Phase 2)

**Date:** 2026-07-12
**Scope:** `quantumverse_qml` headless offscreen render path (`QmlGlRenderer`).
**Gate:** `QMLPerformanceBaseline` / `PerformanceGateTest` (label `performance`; see `tests/benchmark/README.md`).

---

## 1. Baseline numbers

Measured by running the gate on this machine (`build_qml/Release`, `QT_QPA_PLATFORM=offscreen`, 50 frames):

| Run | Avg frame time | Max frame time |
|-----|----------------|----------------|
| Cold (first invocation, shaders/GL init) | 16.42 ms | **66.71 ms** |
| Warm (subsequent invocation)             | 15.68 ms | 17.09 ms |

Steady-state throughput is ≈60 FPS (avg ~15–16 ms). The **outlier is `max` on the cold run (66 ms)** — driven by first-frame GL/shader initialization, not steady-state cost. This is exactly what the gate's `max` threshold (80 ms) is designed to catch.

## 2. Environment caveat (important)

On this Windows offscreen build, Qt does **not** get a hardware OpenGL context. The binary strings and runtime confirm a **ANGLE / Microsoft GDI (software) fallback**:

```
WARNING: Desktop OpenGL may not be active! Renderer contains ANGLE/GDI.
Desktop OpenGL not available; fallback to software renderer or ANGLE mode.
GL_RENDERER: ANGLE / Microsoft GDI
```

Consequences:

- `renderGL()` **does** execute (so the gate exercises real draw calls, shader binds, `GL_CHECK`s, etc.), but on a **software rasterizer**.
- Absolute millisecond costs here are **not representative of a real GPU**. Use the numbers **relatively** (before/after an optimisation) rather than as absolute targets.
- On CI Linux offscreen, `renderGL()` may instead early-return before GL init (per `tests/benchmark/README.md`), in which case only Qt event-loop cost is measured. Either way the **gate is a regression guard, not a perf contract.**

## 3. Hot spots (code + observation)

Ranked by expected contribution to frame time.

### H1 — Deferred GL initialization inside `render()`  *(drives the 66 ms max spike)*
`render()` lazily calls `m_celestialBodyRenderer->initializeGL()` on the **first frame** (`src/qmlglviewport.cpp` ~line 232), and `initializeGL()` (GLAD load, shader compile/link, VAO/VBO creation) also runs on first `render()`. On the software fallback this is where the long first tick comes from. GLAD-init fragility strings (`glad initialization failed`, `Failed to get current OpenGL context for GLAD loader`) appear in the binary, confirming init is a real risk area.

### H2 — Shader (re)compilation on GL state change  *(observed)*
A real driver warning was captured on the cold run:

```
[GL PERFORMANCE][API] Program/shader state performance warning:
Vertex shader in program 23 is being recompiled based on GL state.
```

plus shader compile/link/validate error paths present in the source. Recompiling a program mid-frame is a severe stall. Candidate causes: a VAO whose attribute layout doesn't match the bound program, or mixing programs/VAOs such that the driver sees a "new" vertex format.

### H3 — Per-frame file I/O in `render()`  *(steady-state drain)*
```cpp
std::ofstream("renderer_render.log", std::ios::app) << "QmlGlRenderer::render() called at " << ... << std::endl;
```
at the **top of every `render()` call** opens, writes, and closes a file stream each frame. That is ~50 open/append/close cycles per benchmark run — pure overhead unrelated to rendering. Should be removed or gated behind a debug flag.

### H4 — Per-frame `GL_CHECK()` sync stalls  *(steady-state drain)*
```cpp
#define GL_CHECK() do { GLenum err = glad_glGetError(); if (err != GL_NO_ERROR) { ... } } while(0)
```
Called **4× per frame** inside `render()` (after clear, depth-enable, blend-enable, line-smooth-enable) plus in `initializeGL()`. `glGetError()` is a **GPU↔CPU sync point**; 4 per frame is needless in release. Keep it for debug builds only.

### H5 — `renderGeodesics()` per-frame heap allocation + recompute  *(when geodesics enabled)*
Builds `std::vector<std::vector<Event4D>> geodesics`, then per body `std::vector<Event4D> scaledOrbit` (with a `reserve`), then delegates to `CurvatureRenderer::renderGeodesics`. All of this is rebuilt **every frame** even when the solar-system data is unchanged. This dominates when geodesics are shown.

### H6 — Redundant per-frame GL state / uniform lookups  *(minor)*
- `glEnable(GL_DEPTH_TEST)`, `glEnable(GL_BLEND)`, `glBlendFunc(...)`, `glEnable(GL_LINE_SMOOTH)` are set every frame though they rarely change.
- `m_gridShader.setUniformValue("viewMatrix", ...)` / `"projectionMatrix"` (and the geodesics equivalents) use **string-named** uniforms each frame; `QOpenGLShaderProgram`'s string overload re-resolves the location. Caching the `int` locations (or moving to a UBO as suggested for Phase 3) removes this.

## 4. Instrumentation added for Phase 3

`QmlGlRenderer` now has a **guarded, zero-cost-when-off** per-stage timer:

- `PERF_SCOPE("stageName")` wraps each major render stage (`renderGrid`, `renderAxisGizmo`, `renderGeodesics`, `curvatureRender`, `celestialRender`, `renderQuantumGeometry`, `renderOverlay`, `renderProfilingOverlay`).
- Compiled in only with `-DQUANTUMVERSE_PERF_TRACE=ON` (CMake option `QUANTUMVERSE_PERF_TRACE`, default `OFF`); emits `[PERF] <stage>: <us> us` via `fprintf(stderr, ...)`.

On real (hardware-OpenGL) hardware this yields a per-stage microsecond breakdown to guide Phase 3 precisely. It was **not** used for absolute numbers here because the software fallback makes them unrepresentative.

## 5. Reproduce

```bash
cd F:\syyyy
cmake -B build_qml -DQUANTUMVERSE_BUILD_TESTS=ON -DQUANTUMVERSE_USE_QT=ON
cmake --build build_qml --config Release
set QT_QPA_PLATFORM=offscreen
cd build_qml\Release
ctest -C Release -R Performance --output-on-failure

# For a per-stage breakdown on real hardware:
cmake -B build_qml -DQUANTUMVERSE_USE_QT=ON -DQUANTUMVERSE_PERF_TRACE=ON
cmake --build build_qml --config Release
quantumverse_qml.exe --headless --frames 50 2> perf_trace.txt
```

## 6. Phase 3 candidate optimisations (each TDD-paced)

| ID | Target | Expected win | Verification |
|----|--------|---------------|---------------|
| P3-1 | Remove per-frame `renderer_render.log` `ofstream` (H3) | removes per-frame I/O | gate + `PERF_SCOPE` |
| P3-2 | Move lazy GL init out of `render()` into `synchronize()`/one-time init; pre-warm shaders (H1, H2) | removes first-frame `max` spike | gate `max` drop |
| P3-3 | Gate `GL_CHECK()` to debug builds only (H4) | removes 4×/frame sync stalls | gate `avg` drop |
| P3-4 | Cache view/projection matrices; recompute geodesics only when solar data changes (H5) | removes per-frame allocs/recompute | `PERF_SCOPE(renderGeodesics)` |
| P3-5 | Fix VAO↔program attribute mismatch causing shader recompile (H2) | removes driver recompile stall | GL debug warning gone |
| P3-6 | Cache uniform locations / group into UBOs (H6) | removes per-frame location lookups | `PERF_SCOPE` |

Each item gets its own TDD cycle: write/extend a test asserting the speedup (gate + `PERF_SCOPE`), implement, run the gate to confirm no regression and a measurable improvement.
