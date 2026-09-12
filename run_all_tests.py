#!/usr/bin/env python3
"""Build and run every tests/*.cpp target under Debug, report failures."""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.abspath(__file__))
TESTS = os.path.join(ROOT, "tests")
BUILD = os.path.join(ROOT, "build")

SKIP = {"fuzz_", "crash_helper", "benchmark_physics", "qml_performance_baseline"}
targets = []
for root, _, files in os.walk(TESTS):
    for fn in files:
        if not fn.endswith(".cpp"):
            continue
        if any(fn.startswith(s) for s in SKIP):
            continue
        targets.append(fn[:-4])

targets.sort()
print(f"{len(targets)} targets")

# Cache: target name -> absolute exe path (walk the build tree once).
exe_index = {}
for dirpath, _, files in os.walk(BUILD):
    for fn in files:
        if fn.endswith(".exe"):
            exe_index[fn[:-4]] = os.path.join(dirpath, fn)
print(f"cached {len(exe_index)} exes")

def build(t):
    r = subprocess.run(
        ["cmake", "--build", BUILD, "--target", t, "--config", "Debug"],
        capture_output=True, text=True, encoding="utf-8", errors="replace",
    )
    err = [l for l in r.stderr.splitlines() if "error C" in l]
    return r.returncode, err


def run(t):
    exe = exe_index.get(t)
    if not exe:
        return None, "NO EXE"
    try:
        r = subprocess.run(
            [exe], capture_output=True, text=True, timeout=120,
            encoding="utf-8", errors="replace",
        )
    except subprocess.TimeoutExpired:
        return None, "TIMEOUT"
    out = r.stdout + r.stderr
    return r.returncode, out

failures = []
built = 0
for t in targets:
    rc, err = build(t)
    if err:
        failures.append((t, "BUILD", err[0]))
        continue
    built += 1
    rc, out = run(t)
    if rc is None:
        failures.append((t, "RUN", out))
        continue
    if rc != 0:
        # Non-zero exit is the reliable signal: QV_CHECK throws, soft
        # counters (test_scenario_system) print [FAIL] but still exit 0.
        failures.append((t, f"EXIT {rc}", out[-400:]))
        continue
    if re.search(r"exception|terminate|aborted|Segmentation|std::runtime_error", out):
        failures.append((t, "RUNTIME", out[-400:]))

print(f"built: {built}/{len(targets)}")
print(f"hard failures: {len(failures)}")
for t, kind, msg in failures:
    print(f"  {t}: {kind}")
    for line in msg.splitlines()[-6:]:
        print(f"      {line}")

# Soft failures: tests that print [FAIL] but still exit 0 (pre-existing
# soft-counter design, e.g. test_scenario_system).  Not assert failures,
# but worth knowing about.
soft = []
for t in targets:
    rc, out = run(t)
    if rc is None or rc != 0:
        continue
    fails = [l for l in out.splitlines() if "[FAIL]" in l]
    if fails:
        soft.append((t, len(fails)))
print(f"soft [FAIL] prints (exit 0): {len(soft)}")
for t, n in soft:
    print(f"  {t}: {n}")