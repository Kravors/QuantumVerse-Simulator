# Malformed ONNX models

## rl_discovery_agent_opset27declared18.onnx (20,054 bytes)

- **Declares** opset version 18 in `model.opset_import` (safe for ORT 1.22.1)
- **Contains** opset-27 nodes in `model.graph.node`
- ORT 1.22.1 aborts with: "The requested API version [27] is not available, only API versions [1, 22] are supported in this build."
- **Root cause:** bad export, not pathing. The file loads from the correct repo-root path but ORT rejects its internal opset version.

## Checkpoint status

- `export_best.py` (in `.kilo/worktrees/amenable-myth/python/`) loads `models/geodesic_ode/best_model.pth` via relative path
- **No `.pth`/`.pt`/`.ckpt` exists anywhere accessible** — not in main repo, not in worktrees, never git-committed
- `*.pth` is in `.gitignore` (line: `*.pth`)
- No training data (`.h5`, `.npz`) for `geodesic_ode` exists outside worktrees

## RL Discovery Agent status

| Component | Status |
|-----------|--------|
| `QmlRLDiscoveryAgent` C++ class | Exists, registered as context property |
| `runDiscoveryStep()` | Returns zeros when `m_loaded=false` (ONNX not loaded) |
| Discovery Console panel | Live UI in `main.qml` (`discoveryConsole` id, scrolling AI log) |
| RL output → Console | Wired via `appendLog()`, shows "FAILED to load ONNX model!" |
| ONNX model | Malformed (see above), not loadable |

## Fix path

**Cannot be fixed by re-exporting alone.** `export_best.py` requires a checkpoint that doesn't exist. Retraining would require:
1. Training data (`data/geodesics_1M.h5`) — doesn't exist outside worktrees
2. Training script (`train.py`) — exists only in worktrees
3. Compute budget for RL training

**This is a product decision, not a code fix.** The C++/Qt architecture is complete and ready for a valid `.onnx` file; the blocker is entirely model-level.

Preserved as evidence that the RL Discovery Agent was in a broken state at Session C close.
Not loaded by the application (`.gitignore` line 444 excludes `*.onnx`).
