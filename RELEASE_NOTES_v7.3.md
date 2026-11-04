QuantumVerse v7.3.0

CPack NSIS fix: excluded `_CPack_Packages/` and build artifacts from install rules,
resolving infinite recursion hang. CI went from 74 min hang → 6 min green (12×).
Installer: 31 MB.

Verified end-to-end:
- Silent install succeeds
- Installed binary launches, prints "23 experiments registered"
- Paper pipeline generates PDF

Known issues / install note:
- If you have v7.2 installed, uninstall it first (Start Menu → QuantumVerse → Uninstall)
  before running the v7.3.0 installer. Windows UAC may silently cancel the in-place
  upgrade, leaving v7.2 on disk while reporting success. If you see `r_+ = nanm` or the
  old "RL Discovery Agent: FAILED to load ONNX model" message, you're running v7.2,
  not v7.3.0.
