#!/usr/bin/env python3
"""Analyze performance traces and compare against baseline."""
import argparse
import json
import sys
from pathlib import Path


def load_baseline(path: str) -> dict:
    p = Path(path)
    if p.exists():
        try:
            with open(p, "r") as f:
                return json.load(f)
        except (json.JSONDecodeError, OSError) as e:
            print(f"Failed to load baseline: {e}")
            return {}
    return {}


def analyze_log(log_path: str, baseline: dict, threshold: float = 5.0) -> dict:
    report = {
        "log_path": log_path,
        "baseline_path": baseline.get("path", ""),
        "threshold_percent": threshold,
        "benchmarks": [],
        "regressions": [],
    }
    try:
        with open(log_path, "r") as f:
            lines = f.readlines()
        for line in lines:
            line = line.strip()
            if not line:
                continue
            parts = line.split()
            if len(parts) >= 2:
                name = parts[0]
                try:
                    value = float(parts[1])
                except ValueError:
                    continue
                baseline_value = baseline.get(name, {}).get("time_ms", value)
                change = ((value - baseline_value) / baseline_value * 100.0) if baseline_value else 0.0
                entry = {"name": name, "value": value, "baseline": baseline_value, "change_percent": change}
                report["benchmarks"].append(entry)
                if abs(change) > threshold:
                    report["regressions"].append(entry)
    except FileNotFoundError:
        report["error"] = f"Log file not found: {log_path}"
    return report


def save_baseline(log_path: str, baseline_path: str) -> int:
    baseline = {}
    try:
        with open(log_path, "r") as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                parts = line.split()
                if len(parts) >= 2:
                    name = parts[0]
                    try:
                        value = float(parts[1])
                    except ValueError:
                        continue
                    baseline[name] = {"time_ms": value}
    except FileNotFoundError:
        print(f"Log file not found: {log_path}")
        return 1
    Path(baseline_path).parent.mkdir(parents=True, exist_ok=True)
    with open(baseline_path, "w") as f:
        json.dump(baseline, f, indent=2)
    print(f"Baseline saved to: {baseline_path}")
    return 0


def generate_html(report: dict) -> str:
    lines = [
        "<html><head><title>Performance Trace Analysis</title></head><body>",
        "<h1>Performance Trace Analysis</h1>",
        f"<p>Log: {report.get('log_path')}</p>",
        f"<p>Threshold: {report.get('threshold_percent')}%</p>",
    ]
    if report.get("regressions"):
        lines.append("<h2>Regressions</h2><ul>")
        for r in report["regressions"]:
            lines.append(f"<li>{r['name']}: {r['change_percent']:+.1f}% (current={r['value']}, baseline={r['baseline']})</li>")
        lines.append("</ul>")
    else:
        lines.append("<p>No regressions detected.</p>")
    lines.append("</body></html>")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Analyze performance traces")
    parser.add_argument("log", nargs="?", default="reports/performance.log", help="Performance log file")
    parser.add_argument("output", nargs="?", default="reports/performance_report.html", help="Output report path")
    parser.add_argument("--compare-baseline", action="store_true", help="Compare against baseline")
    parser.add_argument("--save-baseline", action="store_true", help="Save current results as baseline")
    parser.add_argument("--baseline-path", default="tests/baselines/performance_baseline.json", help="Baseline JSON path")
    parser.add_argument("--threshold", type=float, default=5.0, help="Regression threshold percent")
    parser.add_argument("--format", choices=["html", "json"], default="html", help="Output format")
    args = parser.parse_args()

    if args.save_baseline:
        return save_baseline(args.log, args.baseline_path)

    log_path = Path(args.log)
    if not log_path.exists():
        print(f"ERROR: log file not found: {args.log}")
        return 1

    baseline = load_baseline(args.baseline_path) if args.compare_baseline else {}
    report = analyze_log(args.log, baseline, threshold=args.threshold)

    if report.get("error"):
        print(f"ERROR: {report['error']}")
        return 1

    Path(args.output).parent.mkdir(parents=True, exist_ok=True)
    if args.format == "html":
        content = generate_html(report)
    else:
        content = json.dumps(report, indent=2)

    with open(args.output, "w") as f:
        f.write(content)
    print(f"Report saved to: {args.output}")
    if report.get("regressions"):
        print(f"WARNING: {len(report['regressions'])} regression(s) detected")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
