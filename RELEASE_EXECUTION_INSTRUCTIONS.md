# QuantumVerse Simulator v3.9.0 — Release Execution Instructions

**Release Manager**: —  
**Target Date**: 2026-09-29  
**Release Branch**: `main`  
**Tag**: `v3.9.0`

---

## Pre-Flight Checklist

Before starting, ensure:

- [ ] You are on the `main` branch and up to date: `git pull origin main`
- [ ] All CI workflows are green on `origin/main`
- [ ] `CHANGELOG.md` is committed
- [ ] `docs/PRODUCTION_RELEASE_PLAN_v3.9.0.md` is committed
- [ ] `docs/PROJECT_PACKAGE_MANIFEST.md` is committed
- [ ] `docs/VERIFICATION_3D_VIEWPORT.md` is committed
- [ ] `docs/DEPLOYMENT.md` is committed
- [ ] `docs/WINDOWS_DEPLOYMENT.md` is updated to v3.9.0
- [ ] CodeQL workflow removal is committed (`.github/workflows/codeql.yml` deleted)
- [ ] Runtime Monitoring job removal is committed (`.github/workflows/advanced-error-discovery.yml` updated)

---

## Step 1: Final Verification

```bash
# Run full test suite
cd build
ctest -C Release --output-on-failure

# Run headless sanity check
build/Release/quantumverse_qml.exe --headless --frames 1 --metric schwarzschild

# Run 3D viewport verification
# Follow docs/VERIFICATION_3D_VIEWPORT.md
```

**Expected**: All tests pass, headless run succeeds, viewport verification passes.

---

## Step 2: Create Release Commit

If any last-minute fixes are needed, commit them now:

```bash
git add .
git commit -m "chore: final v3.9.0 release preparation"
```

---

## Step 3: Create Annotated Tag

```bash
git tag -a v3.9.0 -m "QuantumVerse Simulator v3.9.0 (VR Multi-User & CI Hygiene) - Production Release"
```

Verify the tag:

```bash
git tag -l "v3.9.0" -n
```

Expected output:
```
v3.9.0  QuantumVerse Simulator v3.9.0 (VR Multi-User & CI Hygiene) - Production Release
```

---

## Step 4: Push Tag to Remote

```bash
git push origin v3.9.0
```

---

## Step 5: Create GitHub Release

1. Go to https://github.com/Kravors/QuantumVerse-Simulator/releases/new
2. Select tag: `v3.9.0`
3. Title: `QuantumVerse Simulator v3.9.0`
4. Description: Paste contents of `CHANGELOG.md` v3.9.0 section
5. Attach artifacts (if any):
   - `QuantumVerse-3.9.0-windows-x64.exe`
   - `QuantumVerse-3.9.0-linux-x86_64.AppImage`
   - `QuantumVerse-3.9.0-macos.dmg`
6. Check "Set as latest release"
7. Click "Publish release"

---

## Step 6: Post-Release Tasks

- [ ] Update `docs/DEVELOPMENT_STATUS.md` version to next development version (e.g., `3.10.0-dev`)
- [ ] Announce release to stakeholders
- [ ] Monitor GitHub Issues for post-release bug reports

---

## Rollback Procedure

If critical issues are discovered:

```bash
# Delete remote tag
git push origin :refs/tags/v3.9.0

# Delete local tag
git tag -d v3.9.0

# Revert release commit if needed
git revert HEAD
git push origin main
```

Then create a hotfix release from the revert commit.

---

## Quick Command Reference

```bash
# Full release sequence
git pull origin main
ctest --output-on-failure
git add .
git commit -m "chore: final v3.9.0 release preparation"
git tag -a v3.9.0 -m "QuantumVerse Simulator v3.9.0 (VR Multi-User & CI Hygiene) - Production Release"
git push origin main
git push origin v3.9.0
```

---

*Release execution instructions for QuantumVerse v3.9.0.*
