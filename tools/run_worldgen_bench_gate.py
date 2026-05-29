#!/usr/bin/env python3
"""SC-E2 / OW-E12: build worldgen_bench (if needed) and fail when inward tris exceed 5%.

Reference preset: default_globe coarse upsample (target=100000, sim=50000, 25% jitter, mesh).
Run from repo root:

    python tools/run_worldgen_bench_gate.py

CI / manual pre-commit:

    pre-commit run worldgen-bench-sc-e2 --all-files
"""
from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def bench_exe(gde: Path) -> Path:
    if sys.platform == "win32":
        return gde / "bin" / "worldgen_bench.exe"
    return gde / "bin" / "worldgen_bench"


def main() -> int:
    root = repo_root()
    gde = root / "gde"
    exe = bench_exe(gde)

    if not exe.is_file():
        print(f"[gate] building {exe.name} via scons in {gde}", flush=True)
        plat = "windows" if sys.platform == "win32" else sys.platform
        build = subprocess.run(
            ["scons", f"platform={plat}", "bin/worldgen_bench"],
            cwd=gde,
            check=False,
        )
        if build.returncode != 0:
            print("[gate] scons build failed", file=sys.stderr)
            return build.returncode
        if not exe.is_file():
            print(f"[gate] missing executable after build: {exe}", file=sys.stderr)
            return 1

    env = os.environ.copy()
    env.setdefault("PYTHONUNBUFFERED", "1")
    run = subprocess.run([str(exe), "0", "gate"], cwd=gde, env=env, check=False)
    if run.returncode != 0:
        print("[gate] worldgen_bench gate failed (SC-E2 / OW-E12)", file=sys.stderr)
    return run.returncode


if __name__ == "__main__":
    raise SystemExit(main())
