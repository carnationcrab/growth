# Epic: Sim correctness & QA

**Status:** In progress (SC-E2 gate landed 2026-05-28)  
**Owner:** —  
**Last updated:** 2026-05-28

Living epic for **trustworthy overworld topology and mesh output**: automated validation, `worldgen_bench` gates, determinism regression, and fixes that belong in sim_core—not only Godot presentation workarounds.

Depends on [epic_overworld_generation.md](epic_overworld_generation.md) pipeline stages. Complements OW-E11–E16 (algorithm quality).

**Known gaps (2026-05-28):** [critical_bugs_audit.md](critical_bugs_audit.md) — items 9, 10.

---

## Vision

Every `default_globe` run produces a **closed dual region–triangle structure**, outward-facing exported terrain meshes, and **bit-identical** hashes under `deterministic_globe` for fixed inputs. CI runs headless benches and fails on regression thresholds documented here.

---

## Goals

| Goal | Notes |
|------|--------|
| Topology invariants | Each triangle in exactly three region rings; ring steps share mesh edges. |
| Mesh export quality | Inward-facing triangle rate below threshold in C++ export, not only after `PresentationCoords`. |
| Determinism | Hash or metric regression for reference configs. |
| Actionable bench output | Warnings become CI failures when over threshold. |

---

## Out of scope

- Godot-only winding fix (keep as presentation fallback; sim must still be correct for streaming/export).
- Full floating-point portability across CPUs (document known limits for SIMD paths).

---

## User stories

Story ids use prefix **SC-** (sim correctness).

### Engineering stories

| ID | Story | Effort | Impact | Acceptance criteria |
|----|--------|--------|--------|---------------------|
| SC-E1 | As a **developer**, I want **`worldgen_bench` to fail CI when region rings are invalid**, so that dual-mesh and river stages do not ship on broken topology. | Medium | High | Exit non-zero when `bad_adjacency > 0` or `tris_not_in_3_rings > 0` for reference configs (document which presets). |
| SC-E2 | As a **developer**, I want **inward terrain triangle rate gated in bench**, so that OW-E12 is enforced automatically. | Low | High | Align with OW-E12: &lt; 5% inward tris on reference `default_globe` + terrain_mesh path; CI fails above threshold. |
| SC-E3 | As a **developer**, I want **determinism regression hashes in bench**, so that parallel/SIMD changes do not silently drift overworld. | Medium | High | `deterministic_globe` run prints stable hash of region elevation + moisture (+ flow optional); stored golden hash per platform or tolerance doc. |
| SC-E4 | As a **developer**, I want **topology validation exposed as a pipeline stage or post-pass**, so that failures are caught before rivers and mesh. | Medium | High | Stage `validate_topology` (or built into `region_neighbours`) logs region count, ring errors; optional auto-fail in dev profile. |
| SC-E5 | As a **developer**, I want **ring repair or regen policy documented**, so that teams know whether to fix `SphereDual` or regenerate sites. | Medium | Medium | When SC-E1 fails: runbook in `worldgen_modern.md` § troubleshooting; link to icosphere/jitter params. |
| SC-E6 | As a **developer**, I want **steep-edge / orphan-flow metrics after priority-flood**, so that OW-E11 improvements are measurable. | Low | Medium | Bench reports orphan branch count or max disconnected flow component size; baseline recorded in epic changelog. |
| SC-E7 | As a **developer**, I want **presentation vs sim coordinate contract tested**, so that Z-up sim and Y-up Godot do not drift. | Low | Medium | Unit or bench check: known sim vector → expected Godot vector via documented `PresentationCoords` mapping; no duplicate transforms in marshal. |

### Cross-epic links (owned elsewhere, validated here)

| ID | Linked epic story | Bench / QA role |
|----|-------------------|-----------------|
| SC-X1 | OW-E11 priority-flood | SC-E6 metrics before/after |
| SC-X2 | OW-E12 outward winding | SC-E2 gate |
| SC-X3 | OW-E16 stream-power erosion | Determinism hash includes post-erosion elev |

---

## Phases

| Phase | Stories | Deliverable |
|-------|---------|-------------|
| **Q0 — Visibility** | SC-E1, SC-E2 (warn-only mode first) | Bench prints; local dev habit. |
| **Q1 — CI gates** | SC-E1, SC-E2, SC-E3 | Fail build on regression. |
| **Q2 — Pipeline integration** | SC-E4, SC-E5, SC-E6 | Validate before rivers; OW-E11 measurable. |
| **Q3 — Presentation contract** | SC-E7 | Coord tests documented. |

---

## Technical reference

| Topic | Location |
|-------|----------|
| Bench | `gde/tools/worldgen_bench.cpp` (`gate` subcommand) |
| SC-E2 CI | `tools/run_worldgen_bench_gate.py`; `pre-commit run worldgen-bench-sc-e2 --all-files` |
| Region rings | `SphereDual.cpp`, `validate_region_rings` in bench |
| Terrain mesh | `PlanetTerrainMesh.cpp`, `generate_planet_terrain_mesh_dual_folded` |
| Presentation | `godot/autoload/sim/PresentationCoords.cs` |
| Pipelines | `data/core/defs/world_gen_pipelines.xml` |

---

## Related epics

- [Critical issues audit](critical_bugs_audit.md) — topology and mesh findings.
- [Overworld generation](epic_overworld_generation.md) — OW-E11–E16 implementation.
- [Sim API & platform](epic_sim_api_platform.md) — headless sync gen for CI.
