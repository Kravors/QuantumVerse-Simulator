# Lensing Null at Frame 0 — Disposition (2026-10-20)

## Summary

**Not a shipped bug.** The `m_lensing= null / enabled= -1` at the first
`[DIAG-renderLensing]` is a cold-start artifact of `--frames 1`, not a persistent
null or lifecycle defect.

## Evidence

1. **`--frames 1` run** (`lens_run.log:64`): one `[DIAG-renderLensing] SKIPPED,
   m_lensing= null enabled= -1`. The app exits after frame 0, before
   `engine.load()` completes (log:75) and before `syncLensingParams()` builds
   `m_lensing` (log:82).

2. **`--frames 10` run** (`lens_run_f10.log`): `[LENS]` appears 100+ times from
   line 103 onward. Lensing renders correctly from frame 1 onward with
   `u_cameraPos=(80 0 2.44929e-15)`, `u_bhPos=(40 0 0)`, `theta=1.5708`
   (equatorial), `hasCameraPos` set.

3. **`syncLensingParams()`** (`qmlglviewport.cpp:3786-3845`): no early return on
   GL context; creates `m_lensing` at line 3793 and applies
   `setEnabled(m_lensingEnabled)` at line 3843. Works correctly — proven by
   `lens_run.log:82` (`[DIAG-setLensingStarFieldEnabled] setting starField= false`,
   which only prints when `m_lensing` is non-null).

4. **`synchronize()`** (`qmlglviewport.cpp:844-908`): no early return before
   line 882 (`m_lensing = viewport->lensingRenderer();`). `lensingRenderer()`
   returns `m_lensing` directly with no guard. No instance mismatch.

## Root cause

`QmlGlRenderer::render()` #0 fires **during** `engine.load()`, before
`main_qml.cpp:1336-1382` calls `setLensingEnabled(true)` / `syncLensingParams()` /
`setLensingCameraPos()`. `synchronize()` copies `m_lensing = null` at line 882,
so frame 0 skips lensing. With `--frames 1`, the app exits before any subsequent
frame renders, so `renderLensing()` never executes.

## Correction to handoff

- **DoD gate:** must specify `--frames ≥ 2` (recommend `--frames 10` for margin).
  The Definition of Done ("`[DIAG-renderLensing]` shows `m_lensing= <non-null>
  enabled= 1`") is unsatisfiable under `--frames 1`.
- **"GPU capture shows the shadow":** capture at frame ≥ 1, not frame 0. The
  GPU-capture command was not saved; the claim is unverifiable.
- **No code change needed.** `setLensingEnabled(true)` keeps `m_lensingEnabled =
  true` (header default); the setter is not a red herring but not the bug either.
- **Phase 33 unblocked.** No fix blocks Phase 33.

## If frame-0 lensing is ever required

Add a 3-`qDebug()` ordering check (`componentComplete()`, `synchronize()` entry,
`render()` entry) with `--frames 1`. If `componentComplete()` precedes render #0,
an eager `syncLensingParams()` in `componentComplete()` fixes frame 0. If not,
the fix is in `main_qml.cpp` ordering (move `setLensingEnabled`/`syncLensingParams`
before `engine.load()` returns). Do not implement without the ordering check.
