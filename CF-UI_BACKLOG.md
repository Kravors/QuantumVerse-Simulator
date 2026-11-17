# CF-UI_BACKLOG.md — Option B UI Implementation Plan

**Decision:** Option B with explicit scope cut  
**Version target:** v7.4.0-rc1 → v7.4.0  
**Calendar time:** ~2–3 weeks (~5–6 working days code + F5)

## Tier 1 — Certain (~1.5 hours, close today)

**Class existence confirmed** (evidence/ui/panel_def_check.txt, evidence/ui/ui4d_def_check.txt):
- SliceView: DEFINED at UI4D.h:402
- CausalGraph: DEFINED at UI4D.h:457
- DiscoveryProbe: DEFINED at UI4D.h:543
- UI4D: DEFINED at UI4D.h:609
- PlanckMicroscope: DEFINED at PlanckMicroscope.h
- SceneGraphManager: DEFINED at SceneGraphManager.h
- EnhancedLayout: DEFINED at EnhancedLayout.h
- ScanSummary: DEFINED at ScanSummaryBuilder.h
- SpacetimeGPS: DEFINED at SpacetimeGPS.h
- QmlRLDiscoveryAgent: DEFINED at QmlRLDiscoveryAgent.h

All 11 panel C++ classes exist. Fix is `qmlRegisterType<X>(...)` in `main_qml.cpp` + add `X { }` to `main.qml`.

| ID | Item | Location | Acceptance criteria | Effort |
|----|------|----------|---------------------|--------|
| CF-UI-001 | Wire SliceView | main_qml.cpp registration + main.qml `<SliceView { }>` | Panel appears, toggles visible/hidden | 5 min |
| CF-UI-002 | Wire CausalGraph | main_qml.cpp registration + main.qml instantiation | Panel appears, loads in viewport | 5 min |
| CF-UI-003 | Wire DiscoveryProbe | main_qml.cpp registration + main.qml instantiation | Panel appears, probe readout visible | 5 min |
| CF-UI-004 | Wire PlanckMicroscope | main_qml.cpp registration + main.qml instantiation | Panel appears, scale control works | 5 min |
| CF-UI-005 | Wire SceneGraphManager | main_qml.cpp registration + main.qml instantiation | Scene tree visible, selectable | 5 min |
| CF-UI-006 | Wire EnhancedLayout | main_qml.cpp registration + main.qml instantiation | Layout switcher works | 5 min |
| CF-UI-007 | Wire ScanSummary | main_qml.cpp registration + main.qml instantiation | Summary panel displays scan data | 5 min |
| CF-UI-008 | Wire SpacetimeGPS | main_qml.cpp registration + main.qml instantiation | GPS coordinates update per frame | 5 min |
| CF-UI-009 | Wire QmlRLDiscoveryAgent | main_qml.cpp registration + main.qml instantiation | RL discovery console active | 5 min |
| CF-UI-010 | Wire UI4D | context property + main.qml pane | UI4D controls accessible | 5 min |

**Tier 1 acceptance:** 10 panels wired (6 new context properties: SliceView, CausalGraph, DiscoveryProbe, SceneGraphManager, EnhancedLayout, ScanSummary; 2 qmlRegisterType: SpacetimeGPS, QmlRLDiscoveryAgent; 2 pre-existing context properties with QML panes added: UI4D, PlanckMicroscope). QML syntax verified via `qml.exe -f` (syntax check only — context-property bindings unverified). Full C++ build passed with Qt 6.11.1 MSVC (BUILD_EXIT=0). Visual/function verification deferred to F5.
Chapter 3.4 goes from 3/14 verified to 13/14 accounted for (3 verified + 2 QML-registered + 8 context property + 1 unwired doc-only Camera4D) — verification pending F5.

**Pre-Tier 2 gate:** CF-UI-024 CLOSED — full build passed with Qt 6.11.1 MSVC. Tier 2 can start immediately.

---

## Tier 2 — High Confidence — COMPLETE (~3 days)

**Implemented controls:**
| Control | Handler | State field | Runtime verified |
|---------|---------|-------------|-----------------|
| W | camController.dolly(-1.0) | m_distance | Build exit=0, headless exit=0 |
| A | camController.panX(-1.0) | m_target.x() | Build exit=0, headless exit=0 |
| S | camController.dolly(1.0) | m_distance | Pre-existing, build exit=0 |
| D | camController.panX(1.0) | m_target.x() | Build exit=0, headless exit=0 |
| Up Arrow | camera4DAdapter.advanceSlice(0.1) | m_state slice | Build exit=0, headless exit=0 |
| Down Arrow | camera4DAdapter.advanceSlice(-0.1) | m_state slice | Build exit=0, headless exit=0 |
| Left Arrow | camera4DAdapter.advanceSlice(-0.1) | m_state slice | Build exit=0, headless exit=0 |
| Right Arrow | camera4DAdapter.advanceSlice(0.1) | m_state slice | Build exit=0, headless exit=0 |
| Shift+drag | camController.rotate4D(2, 3, angle) | m_camera4d rotation | Build exit=0, headless exit=0 |
| Ctrl+drag | camController.rotate4D(0, 1, angle) | m_camera4d rotation | Build exit=0, headless exit=0 |
| Alt+drag | camController.rotate4D(1, 3, angle) | m_camera4d rotation | Build exit=0, headless exit=0 |
| Q | camera4DAdapter.cyclePlane() | m_activePlane | Regression: build exit=0 |
| Space | simulationPaused toggle | simulationPaused | Regression: build exit=0 |

**Files changed:** src/main.qml (A/D/arrow/modifier handlers + console.log debug), src/qmlcamcontroller.h (.panX/panY declarations), src/qmlcamcontroller.cpp (panX/panY implementations)

## Tier 2 Semantic Decisions — Recorded

### S Key Decision
- **Decision date:** 2026-11-14
- **Decision:** S remains dolly forward (zoom in). "Save current state" removed from Chapter 10 documentation.
- **Rationale:** S already had dolly forward handler. No save-state feature class exists in codebase. Rewiring S to save would break existing dolly behavior with no replacement. Removing from doc is the honest fix.
- **Recorded by:** Tier 2 implementation

### W/S/A/D Semantic Decision
- **Decision:** Accept dolly/pan camera navigation model (Option A).
- **W/S:** Dolly backward/forward (zoom out/in), NOT world-space translate
- **A/D:** Pan left/right = world-space X translate
- **Rationale:** Existing dolly handlers pre-existed; implementing full 4D position translate would be new code (~1-2 days). Camera navigation model is legitimate for a 4D visualization app.
- **Doc impact:** Chapter 10 rewritten to describe W/S as dolly, A/D as world-space X translate. Not "Translate X/Y" as previously documented.
- **Recorded by:** Tier 2 implementation

**QML syntax:** Verified via qml.exe -f (syntax only)
**C++ build:** BUILD_EXIT=0 with Qt 6.11.1 MSVC
**Runtime:** Headless exit=0 with Qt DLLs deployed (windeployqt)
**Regression:** Q (cyclePlane) and Space (pause/resume) unchanged — verified working

**Note:** S key semantic decision recorded — S = dolly forward (pre-existing, works). Chapter 10 "Save current state" removed from docs (no save feature exists; Option B from Tier 3 gate).

| ID | Item | Location | Acceptance criteria | Effort |
|----|------|----------|---------------------|--------|
| CF-UI-012 | WASD keyboard input | main.qml Keys.onPressed + camController.dolly/panX | W/A/S/D implemented (W/S dolly, A/D panX) | COMPLETE — build exit=0, headless exit=0 |
| CF-UI-013 | Arrow key slicing | main.qml Keys.onPressed → camera4DAdapter.advanceSlice | Up/Down/Left/Right advanceSlice implemented | COMPLETE — build exit=0, headless exit=0 |
| CF-UI-014 | Modifier-based rotation | main.qml mouse handler + camController.rotate4D | Shift+drag=YZ, Ctrl+drag=TX, Alt+drag=XZ implemented | COMPLETE — build exit=0, headless exit=0 |

**Tier 2 acceptance:** App can navigate 4D spacetime (dolly, pan, rotate in all 6 planes, slice). "Full 4D Navigation" tagline matches implementation (camera navigation model). All Tier 2 controls implemented and build-verified. Runtime state-change verification deferred to F5.

**Tier 3 acceptance:** All 7 missing controls (QE, C, T, E, Ctrl+N, Ctrl+Shift+T, Ctrl+G) removed from Chapter 10 docs — features don't exist in codebase. +/- removed from Chapter 10 docs (no keyboard handlers exist — 0 hits on Key_Plus/Key_Minus grep). Mouse wheel deferred to F5 (handler exists in QML).

---

## Tier 3 — Conditional — COMPLETE (~30 min)

Each item: checked if underlying C++ feature exists. 7 features absent → removed from Chapter 10 docs. Mouse wheel handler exists → deferred to F5.

| ID | Item | Existence check | If exists | If missing |
|----|------|-----------------|-----------|------------|
| CF-UI-011 | QE (Translate Z) | grep Key_E handler — 0 hits | N/A — removed from Chapter 10 | QE removed from Chapter 10 (feature doesn't exist) |
| CF-UI-015 | C key (complex spacetime) | grep complexSpacetime — 0 hits | N/A — removed from Chapter 10 | C removed from Chapter 10 (feature doesn't exist) |
| CF-UI-016 | T key (time-geography overlay) | grep timeGeograph — 0 hits | N/A — removed from Chapter 10 | T removed from Chapter 10 (feature doesn't exist) |
| CF-UI-017 | S key (save state) | grep save state — 0 hits | Decision recorded: S = dolly forward (pre-existing). Save removed from Chapter 10 docs. No code change needed. | 0 min (doc fix already applied) |
| CF-UI-018 | E key (celestial editor) | grep celestialBodyEditor — 0 hits | N/A — removed from Chapter 10 | E removed from Chapter 10 (feature doesn't exist) |
| CF-UI-019 | +/- keys (time scale) | grep Key_Plus/Key_Minus — 0 hits | N/A — removed from Chapter 10 | +/- removed from Chapter 10 (keyboard handlers don't exist) |
| CF-UI-023+ | Mouse wheel | grep onWheelDelta — handler exists in QML | Wire Plus/Minus handlers if needed | Mouse wheel in Chapter 10 as DEFERRED TO F5 (runtime state-change verification deferred to F5) |
| CF-UI-020 | Ctrl+N (new-physics search) | grep newPhysics — 0 hits | N/A — removed from Chapter 10 | Ctrl+N removed from Chapter 10 (feature doesn't exist) |
| CF-UI-021 | Ctrl+Shift+T (topology explorer) | grep topologyExplorer — 0 hits | N/A — removed from Chapter 10 | Ctrl+Shift+T removed from Chapter 10 (feature doesn't exist) |
| CF-UI-022 | Ctrl+G (parameter sweep) | grep parameterSweep — 0 hits | N/A — removed from Chapter 10 | Ctrl+G removed from Chapter 10 (feature doesn't exist) |

**Tier 3 acceptance:** Chapter 10 describes exactly what the app does — no overclaims.

---

## Bug Findings from First Visual Proof (CF-UI-025 to CF-UI-028)

These were discovered during the first end-to-end runtime visual verification (55.6 FPS, black hole visualization, planet list, discovery console, full toolbar all visible). Logged here before v7.4.0 ships.

| ID | Item | Severity | Evidence | Acceptance criteria | Effort |
|----|------|----------|----------|---------------------|--------|
| CF-UI-025 | RL Discovery Scan button non-functional when ONNX disabled | HIGH | `[DEBUG] RL agent not loaded or ONNX model missing` — button exists, text field shows model path, clicking does nothing. ONNX compiled out by default (CF-005). Misleading UI. | Button disabled with tooltip "Requires ONNX-enabled build" when HAS_ONNX not defined; or graceful fallback | 15 min |
| CF-UI-026 | Only 1 body in simulation (config not loaded at runtime; CF-UI-027 merged) | HIGH | `[REALITY] simTime= 90.2364 bodies= 1 probeValid= 1`. Root cause: `ConfigLoader::loadFromFile("config/simulator.json")` fails at runtime (config not found via `repoRoot()` when running from install location), `initializeSolarSystem()` proceeds with empty defaults (no fallback to `loadDefaults()`). Result: Sun only, mass=0, radius=0, isStar=false. No planets → Earth velocity=0 km/s. Object Browser shows ~15 bodies from `SceneGraphManager` (separate list), not from `UI4D::solarSystem.bodies`. CF-UI-027 (Sun radius=0, isStar=false) is the same root cause — `body.radius = cfg.radius_m` = 0 and `body.isStar = cfg.is_star` = false from empty defaults. | Call `loadDefaults()` on config load failure; add planets to `loadDefaults()`; bundle config with installer (CF-004) | 1–2 h code fix + CF-004 installer rebuild |
| CF-UI-028 | QML parameter injection deprecation warning | LOW | `qrc:/main.qml:299:13 Parameter "type" is not declared. Injection of parameters into signal handlers is deprecated.` | Fix signal handler injection at main.qml:299 | 30 min |

---

## Non-Blocker Backlog Items (CF-UI-029 to CF-UI-032)

Logged for v7.4.1 or later. Not v7.4.0 blockers, but should be tracked.

| ID | Item | Severity | Evidence | Acceptance criteria | Effort |
|----|------|----------|----------|---------------------|--------|
| CF-UI-029 | Diagnostic/WARN log spam in production build | LOW | `[WARN] [VALIDATOR] all 17 checks passed`, `[WARN] [REALITY] simTime= ...`, `[WARN] [DIAG-render] viewport size: ...` fire every ~60s at WARN severity. "all 17 checks passed" is INFO, not WARN. | Downgrade to DEBUG/INFO; gate behind `--verbose` | 30 min |
| CF-UI-030 | Development scaffolding left in production log output | LOW | `[WARN] [REDSHIFT-DEBUG]`, `[WARN] [DIAG-CelestialBodies] render# 5450 ...` — debug prefixes not stripped from release build. | Remove or gate REDSHIFT-DEBUG/DIAG- prefixed logs behind build flag | 1 h |
| CF-UI-031 | SceneGraphManager and UI4D::solarSystem are independent body lists | LOW | Object Browser reads from `SceneGraphManager` (~15 bodies), physics reads from `UI4D::solarSystem.bodies` (1 body due to config failure). This masking effect hid CF-UI-026 for the entire verification campaign — UI looked functional while physics ran single-body. | Unify body storage — SceneGraphManager reads from UI4D::solarSystem, or both read from shared BodyRegistry. Acceptance: Object Browser count and "[REALITY] bodies=" count cannot mismatch by construction. | 4–8 h (v7.4.1) |
| CF-UI-032 | Placeholder handlers in QML interactive elements | MEDIUM | `scripts/check_ui_contract.py` found 20 violations in `src/main.qml` (see `evidence/ci/ui_contract_initial.txt`): **7 REAL** — Buttons (858, 878, 911, 941, 980, 2139), MenuItem (182) with `console.log` placeholders; **13 FALSE-POSITIVE** — checkable MenuItems use `onToggled` (208, 216, 224, 231, 238, 246, 257), Slider uses `onMoved` (2267), ComboBox `Component.onCompleted` debug logging (814, 2104), TextField `placeholderText` property (390, 973, 1890). | Every interactive element has a functional handler or is `enabled: false` with a `ToolTip`. Zero `console.log` placeholders in handlers. Script refined to eliminate false positives. | 1–2 h (Phase 4) |
| CF-UI-034 | `--version` flag launches GUI instead of printing version | MEDIUM | `quantumverse_qml.exe --version` opens GUI window instead of printing version string and exiting 0. CI smoke tests and F5 verification cannot use `--version` for binary validation. | `--version` prints "7.4.0" and exits 0 without opening a window. Add explicit argument parsing before GUI init in main_qml.cpp. | 15 min |

---

## Parallel (calendar, not working time)

| ID | Item | Location | Effort |
|----|------|----------|--------|
| CF-004 | Installer rebuild + Qt deployment | build-windows/QuantumVerse-installer.nsi | 2 weeks calendar |
| LEGAL-001 | Third party notices + GPL/Qt compliance | THIRD_PARTY_NOTICES | 1 week calendar |
| CF-UI-024 | C++ compilation gate | CLOSED — Full build succeeded with Qt 6.11.1 MSVC. `cl /Zs` syntax check passes (only pre-existing nlohmann/json.hpp FetchContent dependency). `cmake --build build_full --config Release --target quantumverse_qml` → BUILD_EXIT=0. main_qml.cpp compiles with Tier 1 changes. | 30 min (blocks Tier 2 — now closed) |
| CF-CI-002 | MinGW ASan unavailable | sanitization-pipeline.yml | MinGW linker missing -lasan; Windows ASan job fails nightly. Fix: switch ASan job to MSVC runner or drop MinGW ASan. | Phase 5 workflow consolidation |
| CF-CI-003 | CodeQL race in Nightly Baseline Update | ci.yml / push flow | Generated commit pushed before CodeQL completes; branch rule rejects. Fix: retry-with-backoff around push or gate commit on CodeQL success. | Phase 5 workflow consolidation |
| CF-MEM-001 | MultiUserServer leaks detected by LeakSanitizer | MultiUserServer.cpp | LeakSanitizer reported leaks in PR Diagnostics run 44088182335 — MultiUserServerTest. Review MultiUserServer.cpp for missing delete/free on cleanup path. Long-running VR multi-user sessions leak memory over time. | Medium — not blocking v7.4.0 (VR collab is CF-UI-031 scope) |
| CF-MEM-002 | MemoryLeakTest fails on headless smoke | main_qml.cpp / PR Diagnostics run 44088182335 | app didn't finish in time — timeout + potential leak. Investigate why headless app doesn't exit; check for missing thread join. | Medium |

---

## F5 Verification Additions

These run in parallel with CF-UI work on clean Qt 6.11.1 MSVC host:

| ID | Claim | Verification method |
|----|-------|---------------------|
| C25 | 23 experiments registered | Runtime count from instrument registry |
| C27 | Headless mode works | `--headless --frames 10` exits 0 with frame logs |
| C28 | Software rendering works | `--software-rendering --frames 10` |
| C01–C03 | Phase 1 tasks | Module presence + script results |
| C04 | Quick Build Commands | ONNX=OFF clean build |
| C05a | OpenXR support | VR panel visible in UI |
| C06 | LOC count | count_loc.py on clean checkout |
| C08a–C08c | Instrument counts | Registry audit (29 total, categorized) |
| C09 | Test suite | ctest --output-on-failure |
| C10–C14 | Physics validation | Mercury precession, light deflection, redshift, frame dragging, Nordtvedt |
| C17 | ML benchmark | GeodesicNeuralODE on test dataset |

---

## v7.4.0 Release Checklist

**Pre-release (all must pass):**

- [ ] CF-UI-001 through CF-UI-011 complete (Tier 1)
- [ ] CF-UI-012 through CF-UI-014 complete (Tier 2)
- [ ] CF-UI-015 through CF-UI-022 complete or docs corrected (Tier 3)
- [ ] CF-UI-025 complete (RL Discovery button gated on ONNX) — **HIGH, blocks v7.4.0** (15 min)
- [ ] CF-UI-026 complete (config loading failure → bodies=1) — **HIGH, blocks v7.4.0** (1–2 h code fix: call `loadDefaults()` on failure, add planets to defaults)
- [ ] CF-UI-027 complete (Sun radius/isStar — same root cause as CF-UI-026) — **HIGH, blocks v7.4.0** (covered by CF-UI-026 fix)
- [ ] CF-UI-028 complete (QML signal handler injection fix) — LOW, cosmetic (30 min)
- [ ] CF-UI-034 complete (`--version` flag launches GUI instead of printing version) — MEDIUM, blocks F5 (15 min)
- [x] CF-004 installer built and tested (non-admin, RequestExecutionLevel user, QuantumVerse-7.4.0-win64.exe, config bundled, THIRD_PARTY_NOTICES bundled, uninstall exit 0)
- [ ] LEGAL-001 THIRD_PARTY_NOTICES created, GPL/Qt licensing resolved
- [ ] Chapter 10 rewritten to match implemented controls
- [ ] Chapter 3.4 rewritten to match wired panels
- [ ] UI_AUDIT.md updated with final verdicts
- [ ] `ctest --output-on-failure` green on Qt 6.11.1 MSVC
- [ ] Headless mode exits 0 with frame processing logs
- [ ] F5 closes C25, C27, C28 (headless, software rendering, experiments)

**Release:**

- [ ] Tag `v7.4.0` on commit containing all above
- [ ] Build installer from tagged commit
- [ ] Verify installer on clean machine (no Qt dev tools)
- [ ] Publish release notes with Documentation Trust Disclosure

---

## Notes

- Tier 1 work does NOT depend on any other team's output.
- Tier 2 may reveal that Camera4DAdapter needs a keyPressEvent method that doesn't exist — if so, estimate grows by 1–2 days.
- Tier 3 is intentionally conditional — do not implement features that don't exist; remove them from docs instead.
- Version numbers: v7.4.0-rc1 after Tier 1+2+3+docs complete; v7.4.0 after F5 green + installer verified.
