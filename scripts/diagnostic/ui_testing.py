from typing import Dict, Any, List, Optional
import time
import hashlib
import shutil
from pathlib import Path

class UITester:
    def __init__(self, config: Dict[str, Any], project_root: str) -> None:
        self.config = config
        self.project_root = project_root

    def _get_executable_suffix(self) -> str:
        return ""

    def check_headless_support(self, result: Dict[str, Any], exe_path: str) -> Dict[str, Any]:
        result["status"] = "skipped"
        result["headless"] = False
        return result

    def run_interaction_sequence(self, result: Dict[str, Any], sequence_file: str, ui_config: Dict[str, Any],
                                 exe_path: str, headless: bool = True) -> Dict[str, Any]:
        result["status"] = "skipped"
        result["steps_passed"] = 0
        return result

    def compare_screenshots(self, result: Dict[str, Any], ui_config: Dict[str, Any],
                            baseline_dir: str, output_dir: str) -> Dict[str, Any]:
        result["status"] = "skipped"
        result["diffs"] = []
        return result

    def check_imgui_state(self, result: Dict[str, Any], exe_path: str) -> Dict[str, Any]:
        result["status"] = "skipped"
        result["issues"] = []
        return result

    def run_all(self, ui_config: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
        return {"status": "completed", "tests": [], "passed": 0, "failed": 0}
