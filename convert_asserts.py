#!/usr/bin/env python3
"""Sweep-convert assert() -> QV_CHECK()/QV_CHECK_NEAR() across tests/.

assert() is compiled out under NDEBUG, and every test binary is built
Release, so the entire existing suite was silently swallowing failures.
These macros throw std::runtime_error, which survives NDEBUG.
"""
import os
import re
import sys

TESTS = os.path.join(os.path.dirname(os.path.abspath(__file__)), "tests")
HEADER = '#include "test_assert.h"'
CTEST = "#include <cassert>"


def find_matching_paren(s, start):
    """Return index of the ')' matching the '(' at s[start].

    String literals are skipped so messages like "in (0, 1]" do not
    perturb the parenthesis depth.
    """
    depth = 0
    i = start
    n = len(s)
    in_str = False
    esc = False
    while i < n:
        c = s[i]
        if in_str:
            if esc:
                esc = False
            elif c == "\\":
                esc = True
            elif c == '"':
                in_str = False
        else:
            if c == '"':
                in_str = True
            elif c == '(':
                depth += 1
            elif c == ')':
                depth -= 1
                if depth == 0:
                    return i
        i += 1
    return -1


def split_top_level_minus(expr):
    """Split 'a - b' at the top-level binary minus.

    Returns (a, b) or None when there is no top-level binary minus.  Handles
    '->' member access, string literals, and float exponents (6.67430e11)
    so those '-' characters are not mistaken for binary operators.
    """
    depth = 0
    i = 0
    n = len(expr)
    in_str = False
    esc = False
    while i < n:
        c = expr[i]
        if in_str:
            if esc:
                esc = False
            elif c == "\\":
                esc = True
            elif c == '"':
                in_str = False
        else:
            if c == '"':
                in_str = True
            elif c == '(':
                depth += 1
            elif c == ')':
                depth -= 1
            elif c == '-' and depth == 0:
                nxt = expr[i + 1] if i + 1 < n else ''
                if nxt == '>':
                    pass  # '->' member access
                else:
                    # Skip whitespace backwards to the real previous token.
                    k = i - 1
                    while k >= 0 and expr[k] in " \t":
                        k -= 1
                    prev = expr[k] if k >= 0 else ''
                    # A binary minus has a left operand ending in a letter,
                    # digit, '_', '.', or ')'.  But 'e'/'E' right after a
                    # digit is a float exponent (6.67430e11), not a minus.
                    if prev and prev in 'eE' and k > 0 and expr[k - 1].isdigit():
                        pass
                    elif prev and (prev.isalnum() or prev in '_.)'):
                        return expr[:i].strip(), expr[i + 1:].strip()
        i += 1
    return None


def expr_has_top_level_slash(expr):
    """True if expr contains a top-level '/' (i.e. a ratio, not a plain diff)."""
    depth = 0
    i = 0
    n = len(expr)
    in_str = False
    esc = False
    while i < n:
        c = expr[i]
        if in_str:
            if esc:
                esc = False
            elif c == "\\":
                esc = True
            elif c == '"':
                in_str = False
        else:
            if c == '"':
                in_str = True
            elif c == '(':
                depth += 1
            elif c == ')':
                depth -= 1
            elif c == '/' and depth == 0:
                return True
        i += 1
    return False


def strip_trailing_msg(inner):
    """Pull a trailing `&& "msg"` / `&& 'msg'` off the end of an assert body.

    String-aware: a message that itself contains `&&` or `"` is left alone.
    Returns (body, msg) where msg is "" when there is no trailing message.
    """
    i = len(inner) - 1
    while i >= 0 and inner[i] in " \t":
        i -= 1
    if i < 0 or inner[i] not in ('"', "'"):
        return inner.strip(), ""
    quote = inner[i]
    j = i - 1
    esc = False
    while j >= 0:
        c = inner[j]
        if esc:
            esc = False
        elif c == "\\":
            esc = True
        elif c == quote:
            break
        j -= 1
    if j < 0:
        return inner.strip(), ""
    # j is the opening quote; everything before it must be `&&` (with ws).
    k = j - 1
    while k >= 0 and inner[k] in " \t":
        k -= 1
    if k < 0 or inner[k] != '&' or inner[k - 1] != '&':
        return inner.strip(), ""
    body = inner[:k - 1].strip()
    msg = inner[j:i + 1]
    return body, msg


def split_top_level_op(inner):
    """Find the top-level comparison operator in an assert body.

    Returns (lhs, op, rhs) where op is one of < <= > >= == !=, or None when
    there is no top-level comparison.  String literals and parentheses are
    tracked so operators inside them are ignored.
    """
    depth = 0
    i = 0
    n = len(inner)
    in_str = False
    esc = False
    while i < n:
        c = inner[i]
        if in_str:
            if esc:
                esc = False
            elif c == "\\":
                esc = True
            elif c == '"':
                in_str = False
            i += 1
            continue
        if c == '"':
            in_str = True
        elif c == '(':
            depth += 1
        elif c == ')':
            depth -= 1
        elif depth == 0 and c in '<>=!':
            for op in ('<=', '>=', '==', '!=', '<', '>'):
                if inner[i:i + len(op)] == op:
                    return inner[:i].strip(), op, inner[i + len(op):].strip()
        i += 1
    return None


def lhs_is_abs(lhs):
    """True if lhs is exactly std::abs(X) or std::fabs(X) at the top level.

    Returns the inner X, or None.  Parenthesis-aware: the closing paren of
    std::abs is the first one that balances the opening one, and if anything
    follows it (e.g. '/ Y'), this returns None so the caller can try the
    abs-over pattern instead.
    """
    m = re.match(r'^std::(?:abs|fabs)\((.*)\)$', lhs, re.DOTALL)
    if not m:
        return None
    body = m.group(1)
    # Re-find the matching close paren inside body.
    depth = 0
    in_str = False
    esc = False
    close = -1
    for k, ch in enumerate(body):
        if in_str:
            if esc:
                esc = False
            elif ch == "\\":
                esc = True
            elif ch == '"':
                in_str = False
        else:
            if ch == '"':
                in_str = True
            elif ch == '(':
                depth += 1
            elif ch == ')':
                depth -= 1
                if depth == 0:
                    close = k
                    break
    if close < 0:
        return None
    # If anything follows the abs(...) call, this is not a bare abs(X).
    if body[close + 1:].strip():
        return None
    return body[:close].strip()


def lhs_is_abs_over(lhs):
    """True if lhs is std::abs(X) / Y at the top level.

    Returns the (X, Y) pair, or None.
    """
    m = re.match(r'^std::(?:abs|fabs)\((.*)\)\s*/\s*(.+)$', lhs, re.DOTALL)
    if not m:
        return None
    body = m.group(1)
    depth = 0
    in_str = False
    esc = False
    close = -1
    for k, ch in enumerate(body):
        if in_str:
            if esc:
                esc = False
            elif ch == "\\":
                esc = True
            elif ch == '"':
                in_str = False
        else:
            if ch == '"':
                in_str = True
            elif ch == '(':
                depth += 1
            elif ch == ')':
                depth -= 1
                if depth == 0:
                    close = k
                    break
    if close < 0:
        return None
    return body[:close].strip(), m.group(2).strip()


def classify_and_rewrite(raw):
    """Return replacement text for the full assert(...) expression."""
    inner = raw[1:-1].strip()  # strip the outer parens
    inner, _msg = strip_trailing_msg(inner)

    split = split_top_level_op(inner)
    if split is None:
        # No comparison: just a truthiness check.
        return f"QV_CHECK({inner})"

    lhs, op, rhs = split
    arg = lhs_is_abs(lhs)
    if arg is not None:
        # std::abs(X) < E  ->  QV_CHECK_NEAR(X, 0.0, E)
        sp = split_top_level_minus(arg)
        if sp is not None and not expr_has_top_level_slash(arg):
            return f"QV_CHECK_NEAR({sp[0]}, {sp[1]}, {rhs})"
        return f"QV_CHECK_NEAR({arg}, 0.0, {rhs})"

    over = lhs_is_abs_over(lhs)
    if over is not None:
        # std::abs(X) / Y < tol  ->  QV_CHECK_NEAR(X, 0.0, tol * Y)
        x, y = over
        return f"QV_CHECK_NEAR({x}, 0.0, ({rhs}) * ({y}))"

    # Plain boolean comparison, e.g. std::abs(X)/Y < tol or a < b.
    return f"QV_CHECK({lhs} {op} {rhs})"


def convert_file(path):
    with open(path, "r", encoding="utf-8") as f:
        src = f.read()

    if "assert(" not in src:
        return False, "no assert"

    # Insert header after the last #include at the top of the file.
    lines = src.split("\n")
    insert_at = 0
    last_include = -1
    for i, line in enumerate(lines):
        if line.lstrip().startswith("#include"):
            last_include = i
    if last_include >= 0:
        insert_at = last_include + 1

    already = any(l.strip() == HEADER for l in lines)
    if not already:
        lines.insert(insert_at, HEADER)
        # Also drop the now-unused <cassert>.
        lines = [l for l in lines if l.strip() != CTEST]

    text = "\n".join(lines)

    out = []
    i = 0
    n = len(text)
    changes = 0
    while i < n:
        # Find next 'assert('
        idx = text.find("assert(", i)
        if idx == -1:
            out.append(text[i:])
            break
        out.append(text[i:idx])
        # The '(' after 'assert'
        paren_open = idx + len("assert")
        # skip whitespace
        j = paren_open
        while j < n and text[j] in " \t":
            j += 1
        if j >= n or text[j] != '(':
            # 'assert' not followed by '(' -- leave alone.
            out.append(text[idx:idx + len("assert")])
            i = idx + len("assert")
            continue
        paren_close = find_matching_paren(text, j)
        if paren_close == -1:
            out.append(text[idx:])
            break
        raw = text[idx:paren_close + 1]
        replacement = classify_and_rewrite(raw[6:])  # 'assert(' -> '('
        out.append(replacement)
        changes += 1
        i = paren_close + 1

    result = "".join(out)
    if result != src:
        with open(path, "w", encoding="utf-8") as f:
            f.write(result)
        return True, f"{changes} asserts converted"
    return False, "no change"


def main():
    count = 0
    failed = []
    for root, _, files in os.walk(TESTS):
        for fn in files:
            if not fn.endswith(".cpp"):
                continue
            path = os.path.join(root, fn)
            try:
                changed, msg = convert_file(path)
            except Exception as e:  # noqa: BLE001
                failed.append((path, str(e)))
                continue
            if changed:
                count += 1
                print(f"  {os.path.relpath(path, TESTS)}: {msg}")
    print(f"\nConverted {count} files.")
    if failed:
        print("FAILED:")
        for p, e in failed:
            print(f"  {p}: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()