#!/usr/bin/env python3
"""
Verify every sim_core and gde/src translation unit is listed in gde/SConstruct.

Usage:
  python tools/lint_sconstruct_sources.py
  python tools/lint_sconstruct_sources.py --warn-only
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
SCONSTRUCT = REPO_ROOT / "gde" / "SConstruct"

# Only directories that are part of the shipped GDExtension (see gde/SConstruct).
# Other sim_core/src trees may exist as WIP and are not linted.
SCAN_RELATIVE_DIRS = (
    "sim_core/src/api",
    "sim_core/src/bridge",
    "sim_core/src/data",
    "sim_core/src/gen",
    "sim_core/src/util",
    "sim_core/src/world",
    "src",
)

# Bench-only or non-library sources inside gde/ that are not in `sources`
IGNORE_RELATIVE = {
    "tools/worldgen_bench.cpp",
}

# Present on disk but not yet wired in SConstruct (fix or remove; do not add new entries here).
KNOWN_UNLISTED = frozenset({
    "sim_core/src/world/BowyerWatson.cpp",
    "sim_core/src/world/Delaunay2D.cpp",
    "sim_core/src/world/FibonacciSphere.cpp",
    "sim_core/src/world/PlateElevationAssigner.cpp",
    "sim_core/src/world/PlateMoistureAssigner.cpp",
    "sim_core/src/world/VoronoiSphereGenerator.cpp",
    "sim_core/src/world/WorldGrid.cpp",
})


def _normalise(path: str) -> str:
    return path.replace("\\", "/")


def parse_sconstruct_sources(text: str) -> set[str]:
    in_sources = False
    found: set[str] = set()
    for line in text.splitlines():
        stripped = line.strip()
        if stripped.startswith("sources") and "=" in stripped:
            in_sources = True
            continue
        if not in_sources:
            continue
        if stripped.startswith("]"):
            break
        m = re.search(r'"([^"]+\.cpp)"', stripped)
        if m:
            found.add(_normalise(m.group(1)))
    return found


def discover_cpp_files() -> set[str]:
    gde = REPO_ROOT / "gde"
    rel: set[str] = set()
    for dir_rel in SCAN_RELATIVE_DIRS:
        base = gde / dir_rel
        if not base.is_dir():
            continue
        for path in base.rglob("*.cpp"):
            r = _normalise(str(path.relative_to(gde)))
            if r in IGNORE_RELATIVE:
                continue
            rel.add(r)
    return rel


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--warn-only",
        action="store_true",
        help="Exit 0 even when sources are missing from SConstruct",
    )
    args = parser.parse_args()

    if not SCONSTRUCT.is_file():
        print(f"error: {SCONSTRUCT} not found", file=sys.stderr)
        return 2

    listed = parse_sconstruct_sources(SCONSTRUCT.read_text(encoding="utf-8"))
    on_disk = discover_cpp_files()

    missing = sorted(on_disk - listed - KNOWN_UNLISTED)
    orphans = sorted(listed - on_disk - IGNORE_RELATIVE)
    known = sorted(on_disk & KNOWN_UNLISTED - listed)

    exit_code = 0
    if missing:
        exit_code = 1
        print("CPP files not listed in gde/SConstruct sources:")
        for p in missing:
            print(f'  "{p}",')
        print("\nAdd the lines above to gde/SConstruct, then rebuild.")

    if orphans:
        exit_code = 1
        print("\nSConstruct lists missing .cpp files:")
        for p in orphans:
            print(f"  {p}")

    if known:
        print(f"Note: {len(known)} legacy .cpp file(s) on disk but not in SConstruct (see KNOWN_UNLISTED in script).")

    if not missing and not orphans:
        print(f"OK: {len(on_disk) - len(known)} translation units match SConstruct.")

    if args.warn_only and exit_code != 0:
        return 0
    return exit_code


if __name__ == "__main__":
    sys.exit(main())
