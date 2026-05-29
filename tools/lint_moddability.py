#!/usr/bin/env python3
"""
Enforce moddability architecture rules from docs/moddability.md.

Usage:
  python tools/lint_moddability.py
  python tools/lint_moddability.py --warn-only
  python tools/lint_moddability.py path/to/file.gd
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path

try:
    import yaml
except ImportError:
    yaml = None  # type: ignore


REPO_ROOT = Path(__file__).resolve().parents[1]
CONFIG_PATH = Path(__file__).with_name("moddability_lint.yaml")


@dataclass(frozen=True)
class Violation:
    rule_id: str
    path: Path
    line: int
    message: str
    severity: str

    def format(self) -> str:
        rel = self.path.relative_to(REPO_ROOT)
        return f"{rel}:{self.line}: [{self.severity}] {self.rule_id}: {self.message}"


def _normalise_path(p: str) -> str:
    return p.replace("\\", "/")


def _load_config() -> dict:
    if yaml is None:
        raise SystemExit("PyYAML is required: pip install pyyaml")
    with CONFIG_PATH.open(encoding="utf-8") as f:
        return yaml.safe_load(f)


def _is_ignored(line: str, rule_id: str, suppress_token: str) -> bool:
    if suppress_token not in line:
        return False
    idx = line.find(suppress_token)
    rest = line[idx + len(suppress_token) :].strip()
    if not rest or rest.startswith("all"):
        return True
    return rule_id in rest.split()


def _allow_file(rel_path: str, allow_files: list[str]) -> bool:
    rel = _normalise_path(rel_path)
    for entry in allow_files:
        if rel == _normalise_path(entry) or rel.endswith("/" + _normalise_path(entry)):
            return True
    return False


def _allow_path_prefix(rel_path: str, allow_prefixes: list[str]) -> bool:
    rel = _normalise_path(rel_path)
    for prefix in allow_prefixes:
        p = _normalise_path(prefix)
        if not p.endswith("/"):
            p = p + "/"
        if rel.startswith(p) or rel == p.rstrip("/"):
            return True
    return False


def _iter_files(scan_dirs: list[str], extensions: list[str], roots: list[Path]) -> list[Path]:
    out: list[Path] = []
    for root in roots:
        for scan_dir in scan_dirs:
            base = REPO_ROOT / scan_dir
            if not base.is_dir():
                continue
            for ext in extensions:
                out.extend(base.rglob(f"*{ext}"))
    return sorted(set(out))


def _check_pattern_rules(
    rule_id: str,
    rule: dict,
    files: list[Path],
    suppress_token: str,
) -> list[Violation]:
    violations: list[Violation] = []
    patterns = rule.get("patterns")
    if patterns is None:
        patterns = [rule["pattern"]]
    compiled = [re.compile(p) for p in patterns]
    allow_files = rule.get("allow_files") or []
    allow_path_prefixes = rule.get("allow_path_prefixes") or []
    severity = rule.get("severity", "error")
    description = rule.get("description", rule_id)

    for path in files:
        rel = _normalise_path(str(path.relative_to(REPO_ROOT)))
        if _allow_file(rel, allow_files):
            continue
        if _allow_path_prefix(rel, allow_path_prefixes):
            continue
        try:
            text = path.read_text(encoding="utf-8")
        except OSError as exc:
            violations.append(
                Violation(rule_id, path, 0, f"cannot read file: {exc}", severity)
            )
            continue
        for line_no, line in enumerate(text.splitlines(), start=1):
            if _is_ignored(line, rule_id, suppress_token):
                continue
            for pattern in compiled:
                if pattern.search(line):
                    violations.append(
                        Violation(
                            rule_id,
                            path,
                            line_no,
                            description,
                            severity,
                        )
                    )
                    break
    return violations


def run_lint(paths: list[Path] | None, warn_only: bool) -> int:
    config = _load_config()
    suppress_token = config.get("suppress_comment", "moddability:ignore")
    rules: dict = config["rules"]

    all_violations: list[Violation] = []

    if paths:
        file_set = [p.resolve() for p in paths]
    else:
        file_set = None

    for rule_id, rule in rules.items():
        scan_dirs = rule["scan_dirs"]
        extensions = rule["extensions"]
        if file_set is not None:
            candidates = [
                p
                for p in file_set
                if p.suffix in extensions
                and any(_normalise_path(str(p)).startswith(_normalise_path(d)) for d in scan_dirs)
            ]
        else:
            candidates = _iter_files(scan_dirs, extensions, [REPO_ROOT])

        all_violations.extend(
            _check_pattern_rules(rule_id, rule, candidates, suppress_token)
        )

    errors = [v for v in all_violations if v.severity == "error"]
    warnings = [v for v in all_violations if v.severity == "warn"]

    for v in sorted(all_violations, key=lambda x: (str(x.path), x.line, x.rule_id)):
        print(v.format())

    if errors:
        print(f"\n{len(errors)} error(s)", end="")
    if warnings:
        print(f", {len(warnings)} warning(s)", end="")
    if errors or warnings:
        print()
    else:
        print("moddability lint: OK")

    if warn_only:
        return 0
    return 1 if errors else 0


def main() -> None:
    parser = argparse.ArgumentParser(description="Lint moddability architecture boundaries.")
    parser.add_argument(
        "paths",
        nargs="*",
        type=Path,
        help="Optional files to lint (default: full repo scan)",
    )
    parser.add_argument(
        "--warn-only",
        action="store_true",
        help="Report violations but exit 0 (warnings still print).",
    )
    args = parser.parse_args()
    roots = [p.resolve() for p in args.paths] if args.paths else None
    raise SystemExit(run_lint(roots, args.warn_only))


if __name__ == "__main__":
    main()
