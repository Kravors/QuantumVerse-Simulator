#!/usr/bin/env python3
"""Run every tests/*.cpp test binary under Debug and report failures.

A test passes iff it exits 0.  Anything else (non-zero exit, timeout, crash)
is a hard failure.  Soft [FAIL] counters that still exit 0 are reported
separately because they predate the QV_CHECK conversion.
"""
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.abspath(__file__))
BUILD = os.path.join(ROOT, "build")

SKIP_PREFIX = ("fuzz_", "crash_helper", "CompilerId")
SKIP_NAMES = {"benchmark_physics", "qml_performance_baseline", "quantumverse_qml"}

exe_index = {}
for dirpath, _, files in os.walk(BUILD):
    for fn in files:
        if fn.endswith(".exe"):
            exe_index[fn[:-4]] = os.path.join(dirpath, fn)

targets = sorted(
    n for n in exe_index
    if n.startswith("test_")
    and not n.startswith(SKIP_PREFIX)
    and n not in SKIP_NAMES
)
print(f"{len(targets)} test targets")

TIMEOUT = 180


def run(t):
    exe = exe_index[t]
    try:
        r = subprocess.run(
            [exe], capture_output=True, text=True, timeout=TIMEOUT,
            encoding="utf-8", errors="replace",
        )
    except subprocess.TimeoutExpired:
        return t, "TIMEOUT", ""
    return t, r.returncode, r.stdout + r.stderr


hard = []
soft = []
ok = 0
with ThreadPoolExecutor(max_workers=6) as pool:
    for t, rc, out in pool.map(run, targets):
        if rc == "TIMEOUT":
            hard.append((t, "TIMEOUT", ""))
        elif isinstance(rc, int) and rc != 0:
            # Non-zero exit is the only reliable failure signal.  QV_CHECK
            # throws std::runtime_error, which propagates out of main and
            # terminates the process; soft-counter tests print [FAIL] but
            # still exit 0.
            hard.append((t, f"EXIT {rc} (0x{rc & 0xFFFFFFFF:08X})", out[-300:]))
        else:
            fails = [l for l in out.splitlines() if "[FAIL]" in l]
            if fails:
                soft.append((t, len(fails)))
            else:
                ok += 1

print(f"pass (exit 0): {ok}")
print(f"soft [FAIL] prints (exit 0): {len(soft)}")
for t, n in soft:
    print(f"  {t}: {n}")
print(f"hard failures: {len(hard)}")
for t, kind, msg in hard:
    print(f"  {t}: {kind}")
    for line in msg.splitlines()[-6:]:
        print(f"      {line}")
sys.exit(1 if hard else 0)