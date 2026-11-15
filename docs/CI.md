# CI Pipeline — QuantumVerse v7.4.0 → v7.5.0

## What runs on every push to `main` and every PR

| Stage | Command | Fails CI? |
|---|---|---|
| Build | `cmake --build build --config Release --parallel` | ✅ Yes |
| Unit Tests | `ctest --test-dir build -C Release --output-on-failure` | ✅ Yes |
| QML Lint | `qml.exe -f src/main.qml` + no `Placeholder clicked` | ✅ Yes |
| UI Contract | `scripts/check_ui_contract.py src/main.qml` | ⚠️ Warn-only (Phase 4) |
| Headless Smoke | `--headless --frames 30 --metric schwarzschild` + log parse | ✅ Yes |
| Config Presence | `Test-Path build/Release/config/simulator.json` | ✅ Yes |

## What gets verified

- `[REALITY] bodies= 9` — physics body count (CF-UI-026 regression check)
- `all 17 checks passed` — validator status
- No `Placeholder clicked` — UI placeholder lint
- `config/simulator.json` present in Release output
- UI contract violations surfaced (warn-only until Phase 4)

## How to add a new check

1. Edit `.github/workflows/ci.yml`
2. Add a new step under the `build-test-smoke` job
3. Test on a branch first
4. Update this file

## Why this exists

Before Phase 1, ~30 defects were found through manual verification. Phase 1 automates the top 6 categories. Every class of bug found in v7.3.1 verification is now impossible to reintroduce silently.

## Current UI Contract Status

`scripts/check_ui_contract.py` reports **20 violations** on current `src/main.qml`.
See `CF-UI_BACKLOG.md` (CF-UI-032) for the full breakdown: 7 REAL (need fixing in Phase 4) and 13 FALSE-POSITIVE (script refinement needed).

## Qt Install Note

The CI job installs Qt 6.11.1 MSVC via the online installer. Requires `QT_ACCOUNT_USER` and `QT_ACCOUNT_PASSWORD` GitHub secrets on cache miss.
