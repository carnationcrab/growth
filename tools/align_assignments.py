#!/usr/bin/env python3
"""Align '=' in consecutive simple assignment lines within a source file."""

from __future__ import annotations

import re
import sys
from pathlib import Path

SKIP_PREFIXES = (
    "#if",
    "#elif",
    "#else",
    "#endif",
    "#include",
    "#pragma",
    "using ",
    "namespace ",
    "return ",
    "return;",
    "case ",
    "default:",
    "break;",
    "continue;",
    "goto ",
    "throw ",
    "delete ",
    "new ",
    "typedef ",
    "friend ",
    "explicit ",
    "static_assert",
    "ClassDB::",
    "D_METHOD(",
)

SKIP_STARTS = (
    "if ",
    "if(",
    "else",
    "for ",
    "for(",
    "while ",
    "while(",
    "switch ",
    "switch(",
    "catch ",
    "catch(",
)

COMPOUND_OPS = ("+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=", "=>", ":=")


def _strip_trailing_comment(line: str) -> tuple[str, str]:
    in_string = False
    quote = ""
    i = 0
    while i < len(line):
        ch = line[i]
        if in_string:
            if ch == "\\" and i + 1 < len(line):
                i += 2
                continue
            if ch == quote:
                in_string = False
            i += 1
            continue
        if ch in "\"'":
            in_string = True
            quote = ch
            i += 1
            continue
        if ch == "/" and i + 1 < len(line):
            nxt = line[i + 1]
            if nxt == "/":
                return line[:i].rstrip(), line[i:]
            if nxt == "*":
                return line[:i].rstrip(), line[i:]
        i += 1
    return line.rstrip(), ""


def _assignment_split(code: str) -> tuple[str, str] | None:
    if not code.strip():
        return None
    stripped = code.lstrip()
    for prefix in SKIP_PREFIXES:
        if stripped.startswith(prefix):
            return None
    for start in SKIP_STARTS:
        if stripped.startswith(start):
            return None
    if stripped.startswith("{") or stripped.startswith("}") or stripped == "{" or stripped == "}":
        return None

    for op in COMPOUND_OPS:
        if op in code:
            return None

    in_string = False
    quote = ""
    depth_paren = depth_brack = depth_angle = 0
    i = 0
    while i < len(code):
        ch = code[i]
        if in_string:
            if ch == "\\" and i + 1 < len(code):
                i += 2
                continue
            if ch == quote:
                in_string = False
            i += 1
            continue
        if ch in "\"'":
            in_string = True
            quote = ch
            i += 1
            continue
        if ch == "(":
            depth_paren += 1
        elif ch == ")":
            depth_paren -= 1
        elif ch == "[":
            depth_brack += 1
        elif ch == "]":
            depth_brack -= 1
        elif ch == "<":
            depth_angle += 1
        elif ch == ">":
            depth_angle -= 1
        elif ch == "=" and depth_paren == depth_brack == depth_angle == 0:
            if i + 1 < len(code) and code[i + 1] == "=":
                return None
            if i > 0 and code[i - 1] in "!<>":
                return None
            if i > 0 and code[i - 1] == ":":
                return None
            lhs = code[:i].rstrip()
            rhs = code[i + 1 :].lstrip()
            if not lhs or not rhs:
                return None
            if re.search(r",\s*\w+\s*=", rhs):
                return None
            return lhs, rhs
        i += 1
    return None


def _is_assignment_line(line: str) -> bool:
    code, _ = _strip_trailing_comment(line)
    if not code.strip():
        return False
    indent = code[: len(code) - len(code.lstrip())]
    return _assignment_split(code[len(indent) :]) is not None


def _indent_unit(sample: str) -> str:
    if "\t" in sample:
        return "\t"
    if "    " in sample:
        return "    "
    return "\t"


def _expected_indent_after(prev_line: str) -> str | None:
    if not prev_line.strip():
        return None
    prev_indent = prev_line[: len(prev_line) - len(prev_line.lstrip())]
    stripped = prev_line.strip()
    unit = _indent_unit(prev_indent or prev_line)
    if stripped.endswith("{"):
        return prev_indent + unit
    return prev_indent


def _align_group(lines: list[str], prev_context: str | None = None) -> list[str]:
    parsed: list[tuple[str, str, str, str, str]] = []
    for line in lines:
        indent = line[: len(line) - len(line.lstrip())]
        code, comment = _strip_trailing_comment(line)
        body = code[len(indent) :]
        split = _assignment_split(body)
        if split is None:
            return lines
        lhs, rhs = split
        parsed.append((indent, lhs, rhs, comment, line.endswith("\n") and "\n" or ""))

    max_lhs = max(len(p[1]) for p in parsed)
    common_indent = min((p[0] for p in parsed), key=len)
    if prev_context is not None:
        expected = _expected_indent_after(prev_context)
        if expected is not None:
            common_indent = expected
    out: list[str] = []
    for _indent, lhs, rhs, comment, nl in parsed:
        pad = " " * (max_lhs - len(lhs))
        body = f"{common_indent}{lhs}{pad} = {rhs}"
        if comment:
            if comment.startswith("//") or comment.startswith("/*"):
                body += " " + comment if not body.endswith(" ") else comment
            else:
                body += comment
        out.append(body + nl)
    return out


def _collect_assignment_group(lines: list[str], start: int) -> tuple[list[str], int]:
    group = [lines[start]]
    indent = lines[start][: len(lines[start]) - len(lines[start].lstrip())]
    j = start + 1
    while j < len(lines):
        nxt = lines[j]
        if not nxt.strip():
            break
        nxt_indent = nxt[: len(nxt) - len(nxt.lstrip())]
        if nxt_indent != indent or not _is_assignment_line(nxt):
            break
        group.append(nxt)
        j += 1
    return group, j


def _normalise_whitespace(text: str) -> str:
    lines = text.splitlines()
    cleaned = [ln.rstrip() for ln in lines]
    body = "\n".join(cleaned)
    if body:
        body += "\n"
    return body


def format_source(text: str) -> str:
    text = _normalise_whitespace(text)
    lines = text.splitlines(keepends=True)
    result: list[str] = []
    i = 0
    while i < len(lines):
        line = lines[i]
        if _is_assignment_line(line):
            group, j = _collect_assignment_group(lines, i)
            if len(group) >= 2:
                prev_ctx = None
                k = i - 1
                while k >= 0 and not lines[k].strip():
                    k -= 1
                if k >= 0:
                    prev_ctx = lines[k]
                result.extend(_align_group(group, prev_ctx))
            else:
                result.append(line)
            i = j
        else:
            result.append(line)
            i += 1
    return "".join(result)


def process_file(path: Path) -> bool:
    original = path.read_text(encoding="utf-8")
    formatted = format_source(original.replace("\r\n", "\n"))
    if formatted != original:
        path.write_text(formatted, encoding="utf-8", newline="\n")
        return True
    return False


def main(argv: list[str]) -> int:
    roots = argv[1:] if len(argv) > 1 else [
        "gde/sim_core",
        "gde/src",
        "godot",
    ]
    exts = {".cpp", ".hpp", ".h", ".cs"}
    changed = 0
    for root in roots:
        base = Path(root)
        if not base.exists():
            continue
        for path in sorted(base.rglob("*")):
            if path.suffix.lower() not in exts:
                continue
            if "godot-cpp" in path.parts:
                continue
            if process_file(path):
                print(path)
                changed += 1
    print(f"Aligned assignments in {changed} file(s).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
