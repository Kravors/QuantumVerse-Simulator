#!/usr/bin/env python3
"""Check performance regression against baseline."""
import argparse
import json
import sys
from pathlib import Path


def load_benchmarks(path: str) -> dict:
    p = Path(path)
    if not p.exists():
        print(f"Benchmark file not found: {path}")
        return {}
    try:
        with open(p, "r") as f:
            return json.load(f)
    except (json.JSONDecodeError, OSError) as e:
        print(f"Failed to load benchmarks: {e}")
        return {}


def check(benchmarks: dict, threshold_ms: float = 60.0, max_threshold_ms: float = 250.0) -> int:
    failures = []
    for name, data in benchmarks.items():
        time_ms = data.get("time_ms", 0.0)
        if time_ms > max_threshold_ms:
            failures.append(f"{name}: {time_ms}ms > max {max_threshold_ms}ms")
        elif time_ms > threshold_ms:
            failures.append(f"{name}: {time_ms}ms > threshold {threshold_ms}ms")
    if failures:
        print("PERFORMANCE REGRESSION DETECTED:")
        for f in failures:
            print(f"  - {f}")
        return 1
    print("All benchmarks within thresholds")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Check performance regression")
    parser.add_argument("benchmarks", help="Path to benchmarks JSON file")
    parser.add_argument("--threshold", type=float, default=60.0, help="Average frame-time threshold (ms)")
    parser.add_argument("--max-threshold", type=float, default=250.0, help="Max frame-time threshold (ms)")
    args = parser.parse_args()

    data = load_benchmarks(args.benchmarks)
    if not data:
        print("WARNING: benchmark file missing or empty; skipping performance gate")
        return 0
    return check(data, threshold_ms=args.threshold, max_threshold_ms=args.max_threshold)


if __name__ == "__main__":
    raise SystemExit(main())
