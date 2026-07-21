from typing import Dict, Any, List, Optional
import json
from pathlib import Path
from datetime import datetime

class ReportGenerator:
    def __init__(self, config: Dict[str, Any], project_root: str) -> None:
        self.config = config
        self.project_root = Path(project_root)
        self.results: List[Dict[str, Any]] = []
        self.trends_file = self.project_root / "reports" / "trends.json"

    def _load_trends(self) -> Dict[str, Any]:
        if self.trends_file.exists():
            with open(self.trends_file, "r") as f:
                return json.load(f)
        return {}

    def _save_trends(self, trends: Dict[str, Any]) -> None:
        self.trends_file.parent.mkdir(parents=True, exist_ok=True)
        with open(self.trends_file, "w") as f:
            json.dump(trends, f, indent=2)

    def add_result(self, category: str, name: str, result: Dict[str, Any]) -> None:
        self.results.append({"category": category, "name": name, "result": result})

    def generate_summary(self) -> Dict[str, Any]:
        summary: Dict[str, Any] = {}
        for r in self.results:
            cat = r["category"]
            summary.setdefault(cat, {"passed": 0, "failed": 0, "skipped": 0, "results": []})
            status = r["result"].get("status", "unknown")
            if status == "completed" or status == "passed":
                summary[cat]["passed"] += 1
            elif status == "failed" or status == "error":
                summary[cat]["failed"] += 1
            else:
                summary[cat]["skipped"] += 1
            summary[cat]["results"].append(r)
        return summary

    def generate_html_report(self, summary: Optional[Dict[str, Any]] = None) -> str:
        if summary is None:
            summary = self.generate_summary()
        html = ["<html><head><title>QuantumVerse Diagnostic Report</title></head><body>",
                "<h1>QuantumVerse Diagnostic Report</h1>",
                f"<p>Generated: {datetime.utcnow().isoformat()}Z</p>"]
        for category, data in summary.items():
            html.append(f"<h2>{category}</h2>")
            html.append(f"<p>Passed: {data['passed']}, Failed: {data['failed']}, Skipped: {data['skipped']}</p>")
        html.append("</body></html>")
        return "\n".join(html)

    def generate_json_report(self) -> Dict[str, Any]:
        return {"timestamp": datetime.utcnow().isoformat() + "Z", "summary": self.generate_summary(), "results": self.results}

    def save_report(self, format_type: str, content: Any, output_path: Optional[str] = None) -> str:
        if output_path is None:
            output_path = str(self.project_root / f"diagnostic_report.{format_type}")
        if format_type == "json":
            if isinstance(content, dict):
                data = content
            else:
                data = self.generate_json_report()
            with open(output_path, "w") as f:
                json.dump(data, f, indent=2)
        else:
            if isinstance(content, str):
                data = content
            else:
                data = self.generate_html_report()
            with open(output_path, "w") as f:
                f.write(data)
        return output_path
