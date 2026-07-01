# Release v1.0.0 - Execution Instructions

## Status: ⏳ In Progress

**Started:** 2026-06-19 10:54 UTC
**Stress test:** Running (log shows stable memory at ~187.56 MB, 1.27M+ geodesic rays processed)
**WSL:** Not available on this system (manual step required)
**Self-hosted runner:** `scripts/setup_runner.bat` created - run as Administrator to configure (now prompts for token instead of hardcoded)

---

## 🔴 Step 0: Set Up Self-Hosted Runner (Required for 3 workflows)

The following workflows use a self-hosted runner:
- `nightly-pipeline.yml`
- `sanitization-pipeline.yml`
- `security-audit.yml`

The `fuzz-testing.yml` uses `ubuntu-22.04` (runs in GitHub cloud).

Run this **as Administrator** in PowerShell:

```powershell
cd f:\syyyy
.\scripts\setup_runner.bat
```

This will:
1. Prompt for your GitHub registration token
2. Download the GitHub Actions runner
3. Configure it with the repository
4. Start the runner (keep the window open)

**To get a token:**
- Go to: https://github.com/Kravors/QuantumVerse-Simulator/settings/actions
- Click "New runner" → "Add runner" → "Copy token"
- Tokens expire after 1 hour - generate fresh if needed

---

## 🔴 Step 1: Trigger GitHub CI Pipelines (Manual)

Go to your GitHub repository → **Actions** tab → Run these workflows manually:

1. **nightly-pipeline.yml**
   - URL: `https://github.com/YOUR_USERNAME/REPO_NAME/actions/workflows/nightly-pipeline.yml`
   - Click "Run workflow" → "Run workflow" button
   - **Note:** Uses self-hosted runner (requires Step 0 to be running)

2. **sanitization-pipeline.yml**
   - URL: `https://github.com/YOUR_USERNAME/REPO_NAME/actions/workflows/sanitization-pipeline.yml`
   - Click "Run workflow" → "Run workflow" button
   - **Note:** Uses self-hosted runner (requires Step 0 to be running)

3. **fuzz-testing.yml**
   - URL: `https://github.com/YOUR_USERNAME/REPO_NAME/actions/workflows/fuzz-testing.yml`
   - Click "Run workflow" → "Run workflow" button
   - **Note:** Uses ubuntu-22.04 (runs in GitHub cloud, no self-hosted runner needed)

4. **security-audit.yml**
   - URL: `https://github.com/YOUR_USERNAME/REPO_NAME/actions/workflows/security-audit.yml`
   - Click "Run workflow" → "Run workflow" button
   - **Note:** Uses self-hosted runner (requires Step 0 to be running)

---

## 🔴 Step 2: Start 24-Hour Stress Test (PowerShell)

Open **PowerShell as Administrator** and run:

```powershell
cd f:\syyyy
.\scripts\stress_test_24h.ps1
```

**Important:** Keep the PowerShell window open. The script will:
- Run the application for 24 hours
- Log memory usage every 5 minutes
- Log progress every 10 minutes
- Output to `test_output\stress_test\stress_test_YYYYMMDD_HHMMSS.log`

---

## 🔴 Step 3: Run Fuzzer on WSL/Linux (6 hours)

If you have WSL installed, run:

```bash
# In WSL terminal
cd /mnt/f/syyyy
mkdir -p build_fuzz
cd build_fuzz
cmake -B . -DCMAKE_BUILD_TYPE=Debug -DQUANTUMVERSE_BUILD_FUZZER=ON -DCMAKE_CXX_COMPILER=clang++
cmake --build . --parallel
./tests/fuzz_metric -max_total_time=21600
```

Or if the build directory already exists:
```bash
cd /mnt/f/syyyy/build_fuzz
./tests/fuzz_metric -max_total_time=21600
```

---

## 🟢 Tomorrow (After 24h) - Validation Checklist

### Check Stress Test Window
- [ ] Process is still running (not crashed)
- [ ] No red ASan errors in the log
- [ ] Memory usage is stable (not growing continuously)
- [ ] Log file shows steady progress

### Check GitHub Actions
- [ ] All 4 workflows show **green** status
- [ ] No failed jobs
- [ ] All artifacts uploaded successfully

### Check Fuzzer Output
- [ ] Zero crashes reported
- [ ] No `crash-*` files generated
- [ ] Fuzzer completed 6 hours of execution

---

## 🚀 Release v1.0.0 (If All Checks Pass)

```batch
git tag v1.0.0
git push origin v1.0.0
```

Then:
1. Go to GitHub Releases
2. Find the auto-created v1.0.0 release
3. Download the deploy package from CI artifacts
4. Attach to release
5. Mark as **Production Ready**

---

## Files Verified

| File | Status |
|------|--------|
| `.github/workflows/nightly-pipeline.yml` | ✅ Exists |
| `.github/workflows/sanitization-pipeline.yml` | ✅ Exists |
| `.github/workflows/fuzz-testing.yml` | ✅ Exists |
| `.github/workflows/security-audit.yml` | ✅ Exists |
| `.github/workflows/release.yml` | ✅ Exists |
| `scripts/stress_test_24h.ps1` | ✅ Exists |
| `deploy/windows/quantumverse_imgui.exe` | ✅ Exists |
| `tests/fuzz_metric.cpp` | ✅ Exists |

---

## Quick Commands Summary

```powershell
# Stress test (run in PowerShell Admin)
cd f:\syyyy; .\scripts\stress_test_24h.ps1
```

```bash
# Fuzzer (run in WSL)
cd /mnt/f/syyyy/build_fuzz && ./tests/fuzz_metric -max_total_time=21600
```

```batch
# Release (run in CMD after validation)
git tag v1.0.0
git push origin v1.0.0