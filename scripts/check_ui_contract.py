#!/usr/bin/env python3
"""
UI Contract Checker for QuantumVerse

Validates that every interactive QML element either:
  1. Has a functional handler (onClicked, onTriggered, onValueChanged)
  2. Is explicitly disabled with a reason (enabled: false + ToolTip)
  3. Uses a placeholder pattern (FAILURE)

Usage:
    python scripts/check_ui_contract.py <path-to-main.qml>

Exit codes:
    0 = no violations
    1 = violations found
    2 = usage error / file not found
"""

import re
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Element contract rules
# ---------------------------------------------------------------------------

# (element_kind, handler_required, tooltip_required_when_disabled)
ELEMENT_RULES = [
    ("Button",       "onClicked:", True),
    ("MenuItem",     "onTriggered:", True),
    ("Action",       "onTriggered:", False),
    ("Slider",       "onValueChanged:", False),
    ("ComboBox",     None, True),
    ("TextField",    None, False),
]

PLACEHOLDER_PATTERNS = [
    re.compile(r"console\.log\s*\(", re.IGNORECASE),
    re.compile(r"print\s*\(", re.IGNORECASE),
    re.compile(r"TODO", re.IGNORECASE),
    re.compile(r"FIXME", re.IGNORECASE),
    re.compile(r"placeholder", re.IGNORECASE),
]

HANDLER_PATTERNS = {
    "onClicked:":      re.compile(r"onClicked\s*:", re.IGNORECASE),
    "onTriggered:":    re.compile(r"onTriggered\s*:", re.IGNORECASE),
    "onValueChanged:": re.compile(r"onValueChanged\s*:", re.IGNORECASE),
}

ENABLED_FALSE_PATTERN = re.compile(r"enabled\s*:\s*false\b", re.IGNORECASE)
TOOLTIP_PATTERN = re.compile(r"ToolTip", re.IGNORECASE)
READONLY_TRUE_PATTERN = re.compile(r"readOnly\s*:\s*true\b", re.IGNORECASE)
MODEL_BINDING_PATTERN = re.compile(r"model\s*:", re.IGNORECASE)
BINDING_PATTERN = re.compile(r"\w+\s*:", re.IGNORECASE)


def count_lines_before(text, pos):
    """Return 1-based line number for a position in text."""
    return text[:pos].count("\n") + 1


def extract_blocks(text, element_name):
    """
    Extract top-level QML blocks for a given element name.
    Handles nested braces by tracking depth. Returns list of (start_pos, block_text).
    Blocks are non-overlapping; inner blocks of the same type are handled by the
    outer block's parent scan.
    """
    pattern = re.compile(
        r"\b" + re.escape(element_name) + r"\s*\{",
        re.MULTILINE
    )
    blocks = []
    for m in pattern.finditer(text):
        start = m.start()
        # Find matching closing brace by tracking depth from the opening '{'
        depth = 0
        i = m.end() - 1  # position of '{'
        in_string = False
        string_char = None
        escape_next = False
        while i < len(text):
            ch = text[i]
            if escape_next:
                escape_next = False
                i += 1
                continue
            if ch == "\\" and in_string:
                escape_next = True
                i += 1
                continue
            if in_string:
                if ch == string_char:
                    in_string = False
                i += 1
                continue
            if ch == '"' or ch == "'" or ch == "`":
                in_string = True
                string_char = ch
                i += 1
                continue
            if ch == "{":
                depth += 1
            elif ch == "}":
                depth -= 1
                if depth == 0:
                    block_text = text[start:i + 1]
                    blocks.append((start, block_text))
                    break
            i += 1
    return blocks


def check_element(block, element_name, handler_key, tooltip_required, line):
    """Check a single element block against its contract. Returns list of violation msgs."""
    violations = []

    # Determine which handler pattern to look for
    handler_pattern = HANDLER_PATTERNS.get(handler_key) if handler_key else None

    has_handler = False
    if handler_pattern:
        has_handler = bool(handler_pattern.search(block))

    has_enabled_false = bool(ENABLED_FALSE_PATTERN.search(block))
    has_tooltip = bool(TOOLTIP_PATTERN.search(block))
    has_readonly_true = bool(READONLY_TRUE_PATTERN.search(block))
    has_model = bool(MODEL_BINDING_PATTERN.search(block))

    # Placeholder detection anywhere in the block
    has_placeholder = any(p.search(block) for p in PLACEHOLDER_PATTERNS)

    # Element-specific checks
    if element_name == "Button":
        if has_placeholder:
            violations.append("Button has placeholder handler (console.log/print/TODO/FIXME)")
        elif not has_handler and not has_enabled_false:
            violations.append("Button has no onClicked handler and is not enabled: false")
        elif has_enabled_false and not has_tooltip:
            violations.append("Button is enabled: false but has no ToolTip explaining why")

    elif element_name == "MenuItem":
        if has_placeholder:
            violations.append("MenuItem has placeholder handler")
        elif not has_handler and not has_enabled_false:
            violations.append("MenuItem has no onTriggered handler and is not enabled: false")
        elif has_enabled_false and not has_tooltip:
            violations.append("MenuItem is enabled: false but has no ToolTip explaining why")

    elif element_name == "Action":
        if has_placeholder:
            violations.append("Action has placeholder handler")
        elif not has_handler:
            violations.append("Action has no onTriggered handler")

    elif element_name == "Slider":
        if has_placeholder:
            violations.append("Slider has placeholder handler")
        elif not has_handler:
            violations.append("Slider has no onValueChanged handler")

    elif element_name == "ComboBox":
        if has_placeholder:
            violations.append("ComboBox has placeholder content")
        elif not has_model and not has_enabled_false:
            violations.append("ComboBox has no model binding and is not enabled: false")
        elif has_enabled_false and not has_tooltip:
            violations.append("ComboBox is enabled: false but has no ToolTip explaining why")

    elif element_name == "TextField":
        if has_placeholder:
            violations.append("TextField has placeholder content")
        # TextField needs a binding OR readOnly:true
        has_any_binding = bool(BINDING_PATTERN.search(block))
        if not has_any_binding and not has_readonly_true and not has_enabled_false:
            violations.append("TextField has no binding, is not readOnly:true, and is not enabled: false")

    return violations


def check_ui_contract(qml_text):
    """Run all contract checks on QML text. Returns list of (line, element, message)."""
    violations = []

    for element_name, handler_key, tooltip_required in ELEMENT_RULES:
        blocks = extract_blocks(qml_text, element_name)
        for start_pos, block in blocks:
            line = count_lines_before(qml_text, start_pos)
            for msg in check_element(block, element_name, handler_key, tooltip_required, line):
                violations.append((line, element_name, msg))

    return violations


def main():
    if len(sys.argv) != 2:
        print("Usage: check_ui_contract.py <path-to-main.qml>", file=sys.stderr)
        sys.exit(2)

    qml_path = Path(sys.argv[1])
    if not qml_path.exists():
        print(f"File not found: {qml_path}", file=sys.stderr)
        sys.exit(2)

    qml_text = qml_path.read_text(encoding="utf-8")
    violations = check_ui_contract(qml_text)

    if violations:
        print(f"UI contract violations found: {len(violations)}\n")
        for line, element, msg in violations:
            print(f"  {qml_path}:{line}  [{element}]  {msg}")
        sys.exit(1)

    print(f"UI contract check passed: {qml_path} ({len(violations)} violations)")
    sys.exit(0)


if __name__ == "__main__":
    main()
