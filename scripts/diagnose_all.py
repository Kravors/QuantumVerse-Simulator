from diagnostic.static_analysis import StaticAnalysis
from diagnostic.dynamic_analysis import DynamicAnalysis
from diagnostic.fuzzing import FuzzingRunner
from diagnostic.ui_testing import UITester
from diagnostic.performance import PerformanceTester
from diagnostic.reporting import ReportGenerator
from pathlib import Path

def load_config(config_path: str) -> dict:
    import yaml
    from pathlib import Path
    path = Path(config_path)
    if path.exists():
        with open(path, "r") as f:
            return yaml.safe_load(f) or {}
    return {}

def main() -> int:
    import argparse
    parser = argparse.ArgumentParser(description="QuantumVerse Ultimate Diagnostic Suite")
    parser.add_argument("--quick", action="store_true", help="Run quick diagnostic checks")
    parser.add_argument("--ui", action="store_true", help="Run UI tests")
    parser.add_argument("--fuzz", action="store_true", help="Run fuzz tests")
    parser.add_argument("--bench", action="store_true", help="Run benchmarks")
    parser.add_argument("--full", action="store_true", help="Run full diagnostic suite")
    parser.add_argument("--report-format", choices=["json", "html"], default="html", help="Output report format")
    parser.add_argument("--config", default="scripts/diagnostic/config.yaml", help="Path to config file")
    args = parser.parse_args()

    project_root = str(Path(__file__).resolve().parent.parent)
    config = load_config(args.config)

    generator = ReportGenerator(config, project_root)

    if args.quick or args.full:
        static = StaticAnalysis(config, project_root)
        result = static.run_all()
        generator.add_result("static-analysis", "clang-tidy-cppcheck", result)

    if args.ui or args.full:
        ui = UITester(config, project_root)
        result = ui.run_all(config.get("ui", {}))
        generator.add_result("ui-tests", "headless-ui", result)

    if args.fuzz or args.full:
        fuzz = FuzzingRunner(config, project_root)
        result = fuzz.run_all_targets()
        generator.add_result("fuzz-tests", "hybrid-fuzz", result)

    if args.bench or args.full:
        perf = PerformanceTester(config, project_root)
        result = perf.run_all_benchmarks()
        generator.add_result("performance", "benchmark-suite", result)

    if not (args.quick or args.ui or args.fuzz or args.bench or args.full):
        parser.print_help()
        return 0

    if args.report_format == "json":
        content = generator.generate_json_report()
    else:
        content = generator.generate_html_report()
    output_path = generator.save_report(args.report_format, content)
    print(f"Report saved to: {output_path}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
