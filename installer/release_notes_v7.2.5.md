## QuantumVerse v7.2.5 — Installer

Rebuild of the installer from current `main` (commit 8527fce).

### What's new since v7.2-final
- Multi-probe physics validator (catches the probe-drift class of bug)
- Viewport banner shows correctly when any probe fails
- Geodesic sandbox integrator (verified zero NaN/Inf at spins 0.0–0.99)
- ML panel gracefully shows "ML Model Offline" when model unavailable
- ONNX runtime support compiled in (bundled DLLs — see caveats)

### Known limitations (honestly stated)
- **RL Discovery Agent is non-functional.** The bundled ONNX model is
  malformed (declares opset 18, contains opset-27 nodes; ORT max is 22).
  No checkpoint exists to re-export. Retraining is required — a product
  decision, not a code fix. See HANDOFF.md.
- **ML anomaly score source is unresolved.** The panel displays a score
  but it's unclear whether this comes from the model or a fallback
  heuristic. The panel is honest about "ML Model Offline" state either way.
- **Paper generation requires MiKTeX** installed separately.
  https://miktex.org/download
- **LLM abstract requires Ollama** (localhost:11434, model llama3)
  installed separately. Falls back to template abstract without it.

### Install
1. Download `QuantumVerse-7.2-Setup.exe` (~37 MB)
2. **Windows SmartScreen will warn** — installer is unsigned.
   Click "More info" → "Run anyway"
3. Installs to `%LOCALAPPDATA%\Programs\QuantumVerse` (per-user, no admin required)
4. Launch from Start Menu

### Testing status
- ✓ Built from current `main` (8527fce)
- ✓ Staged smoke test passed with **stripped PATH** (Qt + MinGW removed;
     proves bundled DLLs are complete) — exit code 0, 47.5 FPS
- ✓ Same Inno Setup script as v7.2-final, which passed non-admin install
     and uninstall cleanly
- ⚠ **Non-admin install on a fresh Windows account was not re-tested for
     this build.** The install mechanism is unchanged from v7.2-final, but
     if you encounter issues, please open an issue at
     https://github.com/Kravors/QuantumVerse-Simulator/issues

### Requirements for full functionality
- Windows 10/11 x64
- MiKTeX (for paper PDF generation)
- Ollama + llama3 model (for LLM abstracts)
- Everything else is bundled.
