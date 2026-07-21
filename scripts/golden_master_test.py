#!/usr/bin/env python3
"""Golden master UI tests for QuantumVerse."""
import argparse
import hashlib
import shutil
from pathlib import Path


GOLDEN_DIR = Path("scripts/test_data/golden_master")
CURRENT_DIR = Path("scripts/test_data/current")


def get_test_paths(test_name: str) -> tuple:
    golden = GOLDEN_DIR / f"{test_name}.png"
    current = CURRENT_DIR / f"{test_name}.png"
    return golden, current


def run_test(test_name: str, update_golden: bool = False) -> int:
    golden, current = get_test_paths(test_name)
    CURRENT_DIR.mkdir(parents=True, exist_ok=True)

    if update_golden:
        if current.exists():
            GOLDEN_DIR.mkdir(parents=True, exist_ok=True)
            shutil.copy(current, golden)
            print(f"Updated golden master: {golden}")
            return 0
        print(f"No current screenshot found for {test_name}")
        return 1

    if not golden.exists():
        print(f"Golden master not found: {golden}")
        return 0

    if not current.exists():
        print(f"Current screenshot not found: {current} - skipping comparison")
        return 0

    golden_hash = hashlib.sha256(golden.read_bytes()).hexdigest()
    current_hash = hashlib.sha256(current.read_bytes()).hexdigest()

    if golden_hash == current_hash:
        print(f"PASS: {test_name}")
        return 0
    else:
        print(f"FAIL: {test_name} - screenshots differ")
        return 1


def main() -> int:
    parser = argparse.ArgumentParser(description="Golden master UI tests")
    parser.add_argument("test_name", help="Test name (ui_main_viewport, ui_discovery_panel, ui_physics_viewport)")
    parser.add_argument("--update-golden", action="store_true", help="Update golden master images")
    args = parser.parse_args()

    valid_tests = {"ui_main_viewport", "ui_discovery_panel", "ui_physics_viewport"}
    if args.test_name not in valid_tests:
        print(f"Unknown test: {args.test_name}. Valid: {', '.join(sorted(valid_tests))}")
        return 1

    return run_test(args.test_name, update_golden=args.update_golden)


if __name__ == "__main__":
    raise SystemExit(main())
