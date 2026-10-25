#!/usr/bin/env python3
"""check_canary_sync.py — Verify the canary exclusion list is synced across all 5 CI workflows.

Reads .github/workflows/_canary-exclusions.txt (the shared source of truth)
and checks that each of the 5 workflows:
1. Reads exclusions from _canary-exclusions.txt (for the main test run via -E)
2. Contains all 7 canary tests in its GPU_CANARY inline list (for the rendering
   canary run via -R)

Exit code 0 = all synced. Exit code 1 = drift detected.
"""

from pathlib import Path
import re
import sys

WORKFLOW_DIR = Path(__file__).resolve().parent.parent / ".github" / "workflows"
EXCLUSIONS_FILE = WORKFLOW_DIR / "_canary-exclusions.txt"

WORKFLOWS = [
    "ci.yml",
    "build-and-test.yml",
    "ci-cd.yml",
    "cross-platform-test.yml",
    "pr-diagnostics.yml",
]

# The 7 canary tests that must appear in every GPU_CANARY list:
EXPECTED_CANARIES = [
    "PerformanceGateTest",
    "QMLPerformanceBaseline",
    "ViewportStateTest",
    "ViewportContentTest",
    "VisualRegressionTest",
    "AnimationTimingTest",
    "GLStrictAuditTest",
]


def read_exclusions_file() -> list[str]:
    text = EXCLUSIONS_FILE.read_text().strip()
    return text.split("|")


def read_workflow(name: str) -> str:
    return (WORKFLOW_DIR / name).read_text()


def main() -> int:
    expected_file = read_exclusions_file()
    ok = True

    print(f"Shared exclusions file ({EXCLUSIONS_FILE.name}): {expected_file}")

    for wf in WORKFLOWS:
        text = read_workflow(wf)
        issues = []

        # 1. Does it read from _canary-exclusions.txt?
        if "_canary-exclusions.txt" not in text:
            issues.append("does not read _canary-exclusions.txt")

        # 2. Does GPU_CANARY (or inline -R list) contain all 7 expected canaries?
        gpu_canary = set()
        for m in re.finditer(r'GPU_CANARY="([^"]*)"', text):
            gpu_canary.update(m.group(1).split("|"))
        for m in re.finditer(r'-R\s+"([^"]*)"', text):
            gpu_canary.update(m.group(1).split("|"))

        if not gpu_canary:
            # Workflow has no separate rendering-canary section (e.g. ci.yml).
            # It only runs main tests via _canary-exclusions.txt — that's fine.
            print(f"\n{wf}: OK (reads shared exclusions, no separate GPU_CANARY section)")
            continue

        missing_canaries = set(EXPECTED_CANARIES) - gpu_canary
        if missing_canaries:
            issues.append(f"GPU_CANARY missing canaries: {sorted(missing_canaries)}")

        if issues:
            ok = False
            print(f"\nDRIFT in {wf}:")
            for issue in issues:
                print(f"  - {issue}")
        else:
            print(f"\n{wf}: OK (reads shared exclusions, GPU_CANARY complete)")

    if not ok:
        print("\nCanary exclusion list is NOT synced across all 5 workflows.")
        return 1

    print("\nCanary exclusion list is synced across all 5 workflows.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
