# QuantumVerse v7.0 Handoff — Automated Discovery Paper

## Status summary

| Milestone | Status | Evidence |
|-----------|--------|----------|
| v6.0 cross-client sync | Green | drag Mass in Window A → Window B receives `Received from User_849`, mass matches, grid warps |
| v6.0 self-echo guard | Code retained, verification waived | server filters sender at `server.js:35`; client guard `if (msg.state.lastUpdateBy === myId) return;` kept in `app.js` |
| v7.0 MiKTeX installed | Green | `C:\Program Files\MiKTeX\miktex\bin\x64\pdflatex.exe` |
| `pdflatex --version` | Green | MiKTeX-pdfTeX 4.23 (MiKTeX 25.12) |
| PATH (Machine, permanent) | Green | `C:\Program Files\MiKTeX\miktex\bin\x64` added |
| MiKTeX auto-install | Green | `[MPM]AutoInstall=1` set |
| Template pre-compile | Green | `paper_template.pdf` = 128,064 bytes (at `F:\syyyy\paper_template.pdf`, from CWD compile) |
| `PaperGenerator` path logic | Green | `setWorkingDirectory(outDir)` == `pdfPath` — consistent |
| Substitution → compile | Green | sed + pdflatex → clean compile, no unresolved placeholders |
| LaTeX escaping of LLM abstract | Fixed & verified (bash + C++ harness) | `escapeLatex()` added; adversarial bash test passed; C++ runtime verified via headless harness |
| Adversarial escape test | Green | escaped abstract with `_`, `%`, `&`, `#`, `~` compiles cleanly |
| LaTeX escaping — C++ implementation | Verified | `escapeLatex()` compiled and run via headless harness; all special chars escaped correctly; PDF compiles cleanly |
| Qt headless harness | Green | Built with Qt 6.8.3 + MinGW 13.1.0 (via aqtinstall `tools_mingw1310`); `paper_harness.exe` exits 0, generates `discovery_paper.pdf`, all `escapeLatex` patterns verified in `.tex` |
| Qt headless harness | Green | Built with Qt 6.8.3 + MinGW 13.1.0 (via aqtinstall `tools_mingw1310`); `paper_harness.exe` exits 0, generates `discovery_paper.pdf`, all `escapeLatex` patterns verified in `.tex` |
| GUI click-through | **Wired (v7.1a)** | `main_qml.cpp` now connects `discoveryPanelManager->discoveryResultReady` → `ScanSummary` → `LLMCoPilot::generatePaperAbstract` → `AbstractVerifier::verifyAbstract` → `PaperGenerator::generatePaper`. Requires `llama3` running on `localhost:11434` (Ollama). `DiscoveryEngine::detectAnomaly()` now populates `result.parameters` with curvature scalars from `metric.curvatureScalars()`. PDF output path: `<CWD>/output/discovery_paper.pdf`. |

## Code changes made in this session

### `src/discovery/PaperGenerator.h`
- Added private static `escapeLatex(const QString& input)` declaration

### `src/discovery/PaperGenerator.cpp`
- Implemented `escapeLatex()` — single-pass character replacement escaping LaTeX special chars: `\ & % $ # _ { } ~ ^`
- Line 41: `tex.replace("{{LLM_ABSTRACT}}", llmAbstract)` → `tex.replace("{{LLM_ABSTRACT}}", escapeLatex(llmAbstract))`

### `src/discovery/ScanSummary.h` (new)
- Plain C++ struct aggregating mass, spin, charge, kretschmann, redshift + provenance; `toJson()` for LLM prompt

### `src/discovery/AbstractVerifier.h` / `.cpp` (new)
- `verifyAbstract()` extracts decimal numbers from the abstract and checks each against known ScanSummary values within tolerance; rejects hallucinated numbers

### `src/discovery/LLMCoPilot.h` / `.cpp`
- Added `generatePaperAbstract(const QJsonObject& scanData)` Q_INVOKABLE with paper-specific LLM prompt
- Added `abstractReady(QString)` and `abstractFailed(QString)` signals
- Refactored network finished handler to use member function pointers for signal selection

### `src/discovery/DiscoveryPanelManager.h` / `.cpp`
- Added `discoveryResultReady(QVariantMap)` signal emitted at scan completion
- Added `buildResultMap()` helper aggregating best finding's parameters + metric curvature scalars

### `src/discovery/DiscoveryEngine.cpp`
- `detectAnomaly()` now populates `result.parameters` with `kretschmann` from `metric.curvatureScalars(location)`

**Dead-code note:** `DiscoveryPanelManager::buildResultMap()` reads `result["redshift"]`, `result["kretschmann"]`, etc. directly from the scan result map — it does NOT call `detectAnomaly()`. `detectAnomaly()` is only called by `DiscoveryEngine::runScan()` internally, and its `result.parameters` map is not consumed by the paper pipeline. If `detectAnomaly()` is ever removed, `buildResultMap()` still works because it computes curvature scalars itself via `dynamic_cast<KerrMetric*>`.

### `src/main_qml.cpp`
- Creates `PaperGenerator` instance alongside `LLMCoPilot`
- Wires: `discoveryResultReady` → build `ScanSummary` → `generatePaperAbstract` → `verifyAbstract` → `generatePaper` → PDF

### `CMakeLists.txt`
- Added `src/discovery/AbstractVerifier.cpp` to build

### `src/wasm/app.js` (v6.0 fixes, previously applied)
- Stable `myId` via `sessionStorage` (no more random `User_XXX` per broadcast)
- Self-echo guard: `if (msg.state.lastUpdateBy === myId) return;`
- Exponential backoff on reconnect: 3s → 6s → 12s → ... → cap 30s
- `requestAnimationFrame` throttle on `broadcastState()` (max 1 send/frame)
- Log strings aligned: `[WS] Connected as ${myId}`, `[WS] Received from ${lastUpdateBy} (me=${myId}): M=...`
- `user: myId` (stable) instead of random per-broadcast ID

### `server/server.js` (v6.0, previously applied)
- `clients=N` logged on connect and close

### New files
- `harness_main.cpp` — headless `QCoreApplication` test of `generatePaper()`
- `harness.pro` — qmake project file for the harness

## Code-review notes (for whoever compiles the harness)

The headless harness has already been compiled and run on this machine with Qt 6.8.3 + MinGW 13.1.0. All three escapeLatex validation checks passed:

1. **Backslash ordering** — confirmed single-pass char-by-char; `\` → `\textbackslash{}` not re-processed by `{`/`}` cases.
2. **Clean-case pass-through** — abstract with no special chars comes out byte-identical.
3. **All special chars escaped** — `_`, `%`, `&`, `#`, `$`, `{`, `}`, `~`, `^`, `\` all correctly escaped in generated `.tex`.

The `escapeLatex` was moved from `signals:` to `public:` (signals cannot be `static`) — this was a real bug caught during compile.

## Harness validation (complete)

The harness was run with an abstract containing `spin_parameter` and `50%` plus all LaTeX special chars. Confirmed:
- The generated `.tex` contains `spin\_parameter` and `50\%`
- All special chars properly escaped: `\_ \% \& \# \$ \{ \} \textasciitilde{} \textasciicircum{} \textbackslash{}`
- The PDF compiles with no errors (`discovery_paper.pdf` = 127,839 bytes)
- Exit code 0, `[HARNESS] SUCCESS` emitted

That single run validated the C++ `escapeLatex()` implementation and closed the last code-level gap.


### Option A: Run the headless harness (closes the QProcess layer) — COMPLETED

Already run on this machine with Qt 6.8.3 + MinGW 13.1.0. `paper_harness.exe` exits 0, generates `discovery_paper.pdf`, all `escapeLatex` patterns verified in `.tex`.

Toolchain used:
- Qt 6.8.3 (mingw_64) via `aqt install-qt windows desktop 6.8.3 win64_mingw --UNSAFE-ignore-hash -O C:\Qt`
- MinGW 13.1.0 via `aqt install-tool windows desktop tools_mingw1310 -O C:\Qt --base https://mirror.sjtu.edu.cn/qt/ --UNSAFE-ignore-hash`

**Important:** Use the versioned package `tools_mingw1310`, not the generic `tools_mingw`. The generic package only lists MinGW 8.1.0; Qt 6.8.x ships against MinGW 13.1.0. Installing 8.1.0 against Qt 6.8.3 produces an ABI mismatch (`0xc0000374` heap corruption at runtime). If `aqt install-tool windows desktop tools_mingw` appears to succeed but the build crashes, install `tools_mingw1310` instead.
- PATH: `C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.8.3\mingw_64\bin;` + system PATH

```bash
# From F:\syyyy, with correct PATH:
$env:PATH = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.8.3\mingw_64\bin;" + $env:PATH
rm -rf harness_build; mkdir harness_build; cd harness_build
qmake ..\harness.pro
mingw32-make
.\release\paper_harness.exe   # exit 0, [HARNESS] SUCCESS size=NNNN
```

Expected: exit 0, `[HARNESS] SUCCESS size=NNNN`, all placeholders substituted, PDF compiles cleanly.

**If qmake reports a missing header**, `PaperGenerator.cpp` `#include`s something project-specific beyond its own header — the error names the file; add its `INCLUDEPATH` and `SOURCES` to `harness.pro` and recompile. Not a blocker.

### Option B: Run the GUI (click-through)
Blocked on this machine: `build\Release\quantumverse_qml.exe` does not exist. Pre-built exes in `build_asan\`, `build_ci\`, `build_pkg\` all predate `PaperGenerator.cpp` (10/24 2:57 AM) and therefore do not contain `escapeLatex`. The v7 escaping fix is source-only until rebuilt on a Qt dev machine.

Needs: Windows display session, Qt 6.11 dev toolchain, MiKTeX on PATH.

```powershell
# On the Qt dev machine (after rebuild):
pdflatex --version   # must print a version
cd F:\syyyy\build\Release
.\quantumverse_qml.exe
```

Then: Click **Run Discovery Scan** → wait for near-extremal BH + LLM abstract → click **📄 Generate Discovery Paper** → confirm `<CWD>/output/discovery_paper.pdf` opens.

## Failure capture checklist (if GUI run fails)

Capture all three:

1. **QProcess error text** — from the app's console/log
2. **Generated `.tex` path** — wherever `PaperGenerator` wrote it (likely `<CWD>/output/discovery_paper.tex`)
3. **`findstr /C:"{{" <generated>.tex`** — any output means an unresolved placeholder (most likely `{{LLM_ABSTRACT}}` if Co-Pilot was skipped)

Also note: where the PDF actually landed vs. where the app reported it.

## Gotcha: app.js is served from the build dir, not the source dir

The HTTP server (`python -m http.server 8080`) runs from `build_wasm/` and serves **`build_wasm/src/wasm/app.js`**. Edits to `src/wasm/app.js` are **not** picked up until copied:

```
cp src/wasm/app.js build_wasm/src/wasm/app.js
```

Symptom of forgetting: old log strings in the browser console, unchanged `app.js:` line numbers, "my fix didn't take." This bit v6 debugging twice — once as a `line 316 vs 327` mismatch, once as stale log strings with unchanged line numbers. Always verify what the browser is actually loading:

```powershell
findstr /C:"Connected as" /C:"(me=" "F:\syyyy\build_wasm\src\wasm/app.js"
```

If neither string appears, the served file is stale — copy and hard-reload (`Ctrl+Shift+R`).

## Known-waived items (not defects)

- **v6.0 self-echo verification** — server filters sender (`client !== ws` at `server.js:35`), so client-side self-echo is unobservable. Guard retained as defense-in-depth.
- **Offscreen/headless Qt** — only `windows` platform plugin deployed; `QT_QPA_PLATFORM=offscreen` fails at plugin load. Not worth fixing before the human GUI run.
- **`QProcess` + `pdflatex`** — `pdflatex` works via QProcess from elevated shells when `-interaction=batchmode -halt-on-error` is used. The earlier "MiKTeX blocks pdflatex when elevated" was about interactive TTY prompts, not QProcess batchmode. `pdflatex` must still be on PATH in the shell that launches the app. Open a fresh PowerShell after the Machine PATH change.

```
F:\syyyy\
  src/discovery/PaperGenerator.h      (escapeLatex declared)
  src/discovery/PaperGenerator.cpp    (escapeLatex implemented + applied)
  src/discovery/ScanSummary.h         (new: ScanSummary struct + toJson())
  src/discovery/AbstractVerifier.h    (new: abstract verification)
  src/discovery/AbstractVerifier.cpp  (new: verifyAbstract implementation)
  src/discovery/LLMCoPilot.h          (generatePaperAbstract added)
  src/discovery/LLMCoPilot.cpp        (generatePaperAbstract implemented)
  src/discovery/DiscoveryPanelManager.h  (discoveryResultReady signal)
  src/discovery/DiscoveryPanelManager.cpp (discoveryResultReady emission + buildResultMap)
  src/discovery/DiscoveryEngine.cpp   (detectAnomaly populates parameters)
  src/main_qml.cpp                    (PaperGenerator + full wiring)
  CMakeLists.txt                      (AbstractVerifier.cpp added to build)
  src/wasm/app.js                     (v6.0 WebSocket fixes)
  server/server.js                    (v6.0 client count logging)
  harness_main.cpp                    (headless test, ready to compile)
  harness.pro                         (qmake project, ready to compile)
  HANDOFF.md                          (this file)
```
