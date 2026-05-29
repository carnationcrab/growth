# Critical issues audit (code review)

**Status:** Living audit  
**Last reviewed:** 2026-05-28  
**Method:** Static review of the repo tree, epics, and `worldgen_bench` validation output—not a formal issue tracker.

This document records the **highest-impact gaps** found when comparing implementation to [epic_overworld_generation.md](epic_overworld_generation.md), [epic_game_world_streaming.md](epic_game_world_streaming.md), and [moddability.md](moddability.md). Each item links to **user stories** added to close the gap. When a bug is fixed, update this file (mark resolved, note PR) and verify acceptance criteria on the linked stories.

**Related backlogs:** [README.md](README.md) epics table · [game_scene_order.md](game_scene_order.md) (today vs target)

---

## Summary

| # | Issue | Symptom | Primary stories |
|---|--------|---------|-----------------|
| 1 | ~~No overworld atlas in `SimBridge`~~ **partial** | Atlas + `sample_surface` in bridge; marshal still duplicates data (OW-E6) | OW-E6, OW-E20 |
| 2 | Sync `take_result` on main thread | Frozen UI, OOM on huge worlds | SA-0.2, SA-E1–E2, OW-4.6, SA-4.1 |
| 3 | No `poll_world_gen_async` in API | Fake progress, no cancel | SA-0.2, OW-4.1–4.2, OW-4.6 |
| 4 | Incomplete world-gen form | Size preset useless, always heavy sim | OW-3.4, SA-3.1, OW-3.6 |
| 5 | `world_size` ignored in C++ | Size dropdown has no effect | SA-E4, OW-3.5 |
| 6 | Boot loads `data/core/` only | Mod defs ignored | SA-0.1, OW-0.2, OW-0.3 |
| 7 | Dig / intents no-op | Tools do nothing | GW-E10, GW-E11, GW-UI.1 |
| 8 | Stale chunk cache after preview | Broken streaming after preview | GW-2.4, GW-7.0b–7.0c |
| 9 | Invalid region triangle rings | Wrong rivers / elevation | SC-E1, SC-E4, OW-E18 |
| 10 | ~~Inward terrain tris in sim export~~ | Resolved OW-E12 / SC-E2 | — |

---

## 1. Overworld generation is thrown away after marshalling to Godot — **partial (2026-05-28)**

**Resolved for session play:** `apply_world_gen_form` builds `PlanetSurfaceAtlas` (per-region sites, topology triangles, neighbours CSR, plate/elevation/moisture) into `SimBridge`. **Start** calls `commit_overworld_for_play` and `enter_game_world`. `sample_surface(unit_dir)` returns region scalars + derived temperature.

**Still open:** Full `PlanetGlobe` / half-edge mesh is not retained after the job completes (OW-E20). Godot preview still receives a large marshal dict in parallel (OW-E6). Rivers / triangle_values not in atlas v1.

**Stories:** OW-E6 (marshal from atlas), OW-E20 (release transient globe memory).

---

## 2. World gen blocks the Godot main thread

`WorldGenJob` uses a worker thread, but `GrowthSim::apply_world_gen_form` creates a **local** job and immediately calls `take_result`, which **joins** the worker on the calling thread:

```68:79:gde/src/sim_api_gdextension.cpp
	growth::WorldGenJob job;
	if (!job.start(parsed, bridge_->bridge.defs(), pipeline_id)) {
		// ...
	}

	growth::WorldGenJobResult result;
	if (!job.take_result(result)) {
```

`WorldGenLoadScreen.gd` calls `SimAPI.apply_world_gen_form` from `_do_run_generation` on the main thread. Large `voronoi_sites` (slider up to **500,000**) can freeze or crash Godot for minutes while the progress bar stays static.

**Stories:** SA-E1 (persistent job on bridge), SA-E2 (GDExtension async bindings), OW-4.6 (load screen uses async only), SA-4.1.

---

## 3. Async progress API is designed but not wired

`WorldGenLoadScreenProgress.gd` expects `poll_world_gen_async` snapshots, but **`SimAPI` / `GrowthSim` expose no** `start` / `poll` / `cancel` / `take` async methods—only synchronous `apply_world_gen_form`. `WorldGenLoadScreenLauncher` is documented as “async” but the load screen path is sync.

**Impact:** OW-4.1 and OW-4.2 acceptance criteria cannot be met; users cannot cancel long runs.

**Stories:** SA-0.2, SA-4.1–4.2, OW-4.1–4.2, OW-4.6.

---

## 4. `WorldGenMenu` submits an incomplete form (`WorldGenCardFormBinder` unused)

`WorldGenCardFormBinder` builds `sim_voronoi_sites`, `world_gen_mode`, and `use_simd`, but **no caller uses it**—only `WorldGenMenu.gd` builds the dict manually. It omits coarse sim sites, hardcodes `"Earthlike"`, and ignores mode/SIMD toggles when those nodes exist on the card.

**Impact:** Large worlds always run at full preview resolution in sim; size preset tables in `WorldGenCardPaths` have no effect.

**Stories:** OW-3.4, SA-3.1, OW-3.6.

---

## 5. `world_size` from the form is ignored in C++

The menu sends `"world_size": world_size_idx`, but `WorldGenFormParser.cpp` has **no handler for `world_size`**. The size preset only affects the UI unless the user moves the points slider directly.

**Stories:** SA-E4, OW-3.5.

---

## 6. Boot loads `data/core/` only — mods never reach the sim

```33:36:godot/main/Main.gd
func _init_sim_bridge() -> void:
	var defdb_path: String = ProjectSettings.globalize_path("res://data/core/")
	SimAPI.boot(defdb_path)
```

Moddability expects boot at `res://data/` with pack manifests. C++ `DefDatabase` only sees the core pack; example mod defs under `data/mods/` are not merged. `PresentationSceneEngine` also falls back to `res://data/core/` when boot path resolution fails.

**Stories:** SA-0.1, OW-0.2, OW-0.3, SA-E3. Linter rule: `godot_boot_data_root`.

---

## 7. Player intents do not work (`IntentRouter` bypassed, dig is a no-op)

Docs describe `IntentRouter` routing `apply_intent` with interaction defs. The GDExtension path hardcodes `"dig"` and calls `apply_intent_dig`, which is empty:

```30:36:gde/sim_core/src/bridge/SimBridge.cpp
void SimBridge::apply_intent_dig(ChunkCoord chunk, int local_x, int local_z) {
	(void)chunk;
	(void)local_x;
	(void)local_z;
}
```

`IntentRouter` is compiled but **not used** from `IntentHandler.cpp`.

**Stories:** GW-E10, GW-E11, GW-UI.1.

---

## 8. Chunk streaming vs planet preview leaves stale chunk state

`Main` skips `update_streaming` when `SpherePreview` exists, but `SpherePreview._Ready` only `QueueFree`s `WorldRoot` children—it does **not** clear `WorldViewManager`’s `_chunks` dictionary. If preview is removed later, streaming may think chunks are still loaded and **skip reloading** (references to freed nodes).

`ClearWorldTerrain` clears visuals only; see [game_scene_order.md](game_scene_order.md) step 5 vs step 7.

**Stories:** GW-2.4, GW-2.5, GW-7.0b, GW-7.0c.

---

## 9. Region triangle rings can be topologically invalid

`worldgen_bench` validates region rings and warns when triangles are not in exactly three rings or ring steps are non-adjacent:

```102:103:gde/tools/worldgen_bench.cpp
	if (bad_adjacency > 0 || wrong_tri_count > 0)
		Ciostream::cerr() << "  [validate] WARN: region triangle rings are not a closed dual mesh\n";
```

**Impact:** Priority-flood, river flow, and dual terrain mesh assume a valid dual structure—**silent wrong hydrology** rather than a hard crash.

**Stories:** SC-E1, SC-E4, SC-E5, OW-E18.

---

## 10. Terrain mesh winding / inward triangles in sim (preview-only fix)

**Resolved (2026-05-28, OW-E12, SC-E2):** `ensure_planet_terrain_mesh_outward_winding` post-pass after `generate_planet_terrain_mesh_dual_folded`; `worldgen_bench gate` enforces &lt; 5% inward tris on coarse upsample (`tools/run_worldgen_bench_gate.py`).

Previously ~50% inward on coarse upsample; Godot-only `PresentationCoords` could not repair mixed winding for streaming/export.

**Stories:** SC-E2, OW-E12, OW-5.4, SC-E7.

---

## Additional risks (not in top 10)

| Risk | Notes | Stories |
|------|--------|---------|
| Marshal OOM on huge worlds | Full dict to Godot for 100k+ regions | OW-E19, SA-E5, OW-4.9 |
| Hardcoded scene paths | `Main.gd`, `WorldGenMenu.gd` literal `res://godot/...` | SA-1.2, SA-3.2, OW-1.3, GW-E13 |
| `ChunkStore` never unloads in sim | View unloads; sim retains all loaded chunks | GW-E9 |
| Sync path keeps local `WorldGenJob` | Cannot poll/cancel across frames | SA-E1 |
| Stub backend silent UX | Missing DLL → empty result | SA-0.4 |

---

## How to use this audit

1. **Triage:** Pick items by [README.md](README.md) suggested cross-epic order.  
2. **Implement:** Close linked stories; do not duplicate acceptance criteria here.  
3. **Verify:** Run `worldgen_bench`, moddability lint, and manual load-screen / preview / streaming flows.  
4. **Close:** When fixed, add a “Resolved” line under the section with date and story id.

---

## Epic cross-reference

| Epic | Bugs primarily addressed |
|------|-------------------------|
| [epic_sim_api_platform.md](epic_sim_api_platform.md) | 2, 3, 4, 5, 6, hardcoded paths |
| [epic_overworld_generation.md](epic_overworld_generation.md) | 1, 4, 5, marshal/atlas/preview |
| [epic_sim_correctness.md](epic_sim_correctness.md) | 9, 10 |
| [epic_game_world_streaming.md](epic_game_world_streaming.md) | 7, 8, chunk unload |

---

## Changelog

| Date | Change |
|------|--------|
| 2026-05-28 | Initial audit from codebase review; stories added to epics |
