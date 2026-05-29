# Epic: Overworld generation

**Status:** In progress  
**Owner:** —  
**Last updated:** 2026-05-28 (stories expanded: platform, form, topology QA)

Living epic for **overworld** creation: one deterministic planet-scale globe per session, stored in the sim as queryable data—not only as Godot preview arrays.

Scene flow for this epic: [game_scene_order.md](game_scene_order.md) steps 0–6. The **game world** (streamed local sim) is **[epic_game_world_streaming.md](epic_game_world_streaming.md)**.

**Known gaps (2026-05-28):** [critical_bugs_audit.md](critical_bugs_audit.md) — items 1, 4, 5, 9, 10 and related platform issues.

---

## Terminology

| Term | Meaning |
|------|---------|
| **Overworld** | Generated macro planet: spherical region atlas (plates, elevation, moisture, …). Low resolution, whole planet, read-only baseline for play. |
| **Game world** | Generated micro simulation around the player: streamed chunks, metre-scale detail, player-editable. See streaming epic. |
| **Planet preview** | Debug Godot view of overworld data (`SpherePreview`); not the game world. |

Code names such as `PlanetGlobe`, `PlanetSurfaceAtlas`, and `PlanetGlobePipeline` refer to **overworld** sim structures until renamed in a later refactor.

---

## Vision

The player configures an overworld (seed, climate, resolution, plates). The sim runs the data-driven `PlanetGlobePipeline`, produces an **overworld surface atlas** (regions on a unit sphere), and keeps **planet genome** + **world seed** for the session. Godot scenes show progress and optional debug preview; gameplay systems read overworld location through the sim API, not by parsing marshal dictionaries.

---

## Goals

| Goal | Notes |
|------|--------|
| Deterministic overworld | Same form + pipeline → same region fields (mod `deterministic_globe` when needed). |
| Authoritative sim storage | Overworld atlas (`PlanetSurfaceAtlas` or equivalent) survives after generation; preview is optional. |
| Location context | Given a direction on the sphere, sim returns region id, plate, elevation, moisture, derived biome/climate. |
| Moddable pipeline | Stage order from `world_gen_pipelines.xml`; no hardcoded stage lists in Godot. |
| Profile-driven presentation | Menu, load screen, preview paths from `game_profile.xml` only. |

---

## Out of scope (this epic)

- Game world chunks and player terrain edits → [game world streaming](epic_game_world_streaming.md).
- Save/load file format (atlas serialisation may be a late story here or in streaming).
- Gameplay HUD, build menu, pause (registered scenes not in default profile).
- Replacing debug `SpherePreview` with final art direction (preview may remain dev-only).

---

## Architecture (target)

```text
WorldGenMenu (form)
    → WorldGenLoadScreen (async job UI)
        → C++: ParsedWorldGenForm → WorldGenRunner (seed + genome)
        → PlanetGlobePipeline → PlanetGlobe
        → build overworld PlanetSurfaceAtlas → SimBridge
        → optional: PreviewExport → result dict → SpherePreview
```

| Layer | Responsibility |
|-------|----------------|
| **Content** | `planet_presets.xml`, `world_gen_pipelines.xml`, `biomes.xml` (rules for derived biome). |
| **Sim** | Pipeline stages, overworld atlas build, `SurfaceSample` / `locate_region`. |
| **Godot** | Form, progress, optional overworld visualisation; `SimAPI` only entry to C++. |

**Anti-patterns:** Growing `ApplyWorldGenForm` forward lists as the long-term gameplay API; storing the only copy of `region_elevation` in `SpherePreview`.

---

## Dependencies

- GDExtension `GrowthSim` built (`gde/build.ps1`).
- `DefDatabase` + `game_profile` pipeline id (`default_globe`).
- See [worldgen_modern.md](worldgen_modern.md) for algorithm reference, [moddability.md](moddability.md) for rules.

---

## User stories by scene

Stories use **As a / I want / So that**. Scene ids match `scene_registry.xml` / [game_scene_order.md](game_scene_order.md). Story ids use prefix **OW-** (overworld).

### Scene: `SimAPI` autoload (step 0)

| ID | Story | Acceptance criteria |
|----|--------|---------------------|
| OW-0.1 | As a **developer**, I want **overworld generation to run only through SimAPI**, so that moddability lint passes and Godot never calls `GrowthSim` directly. | All gen entry points go through `apply_world_gen_form` / async variants; no `GrowthSim` in gameplay scripts. |
| OW-0.2 | As a **developer**, I want **boot to load defs and profile** before any gen, so that pipeline id and presentation paths resolve. | `SimAPI.boot(res://data/, profile_id)` loads merged defs; `get_presentation_scene_path` works for menu and load screen. *(Implementation: [SA-0.1](epic_sim_api_platform.md).)* |
| OW-0.3 | As a **mod author**, I want **overworld pipeline defs from enabled packs**, so that a new pack can add stages or presets without forking core. | `DefDatabase` after boot includes merged `world_gen_pipelines.xml` / `planet_presets.xml`; active profile pipeline id resolves from merged defs. |

### Scene: `Main.tscn` (step 1)

| ID | Story | Acceptance criteria |
|----|--------|---------------------|
| OW-1.1 | As a **player**, I want **the game to start on Main with sim initialised**, so that the overworld gen menu can submit a form immediately. | `Main._ready` calls `SimAPI.boot`; no hardcoded `res://` scene paths in `Main.gd`. *(SA-1.1, SA-1.2.)* |
| OW-1.2 | As a **developer**, I want **Main to instantiate the entry menu from the active profile**, so that swapping `game_profile.xml` changes the first screen. | Entry menu path from `get_presentation_scene_path("entry_menu")`. |
| OW-1.3 | As a **developer**, I want **world gen menu and load screen paths from profile**, so that presentation swaps do not require editing `WorldGenMenu.gd`. | Generate uses `world_gen_load_screen` binding; no literal `res://godot/ui/menus/worldGenLoadScreen.tscn` in gameplay scripts. *(SA-3.2.)* |

### Scene: `world_gen_menu` → `worldGenMenu.tscn` (step 3)

| ID | Story | Acceptance criteria |
|----|--------|---------------------|
| OW-3.1 | As a **player**, I want **to set seed, climate, resolution, plates, and terrain mesh option**, so that I control the generated overworld. | Form dict includes keys parsed by `WorldGenFormParser` (`voronoi_sites`, `jitter`, `num_plate_regions`, `use_planet_terrain_mesh`, `world_gen_mode`, `use_simd`, `sim_voronoi_sites`, etc.). |
| OW-3.2 | As a **player**, I want **Generate to open the load screen**, so that I see progress instead of freezing the menu. | Menu frees or hides after load screen completes; does not block on C++ work on the menu scene. |
| OW-3.3 | As a **developer**, I want **panel styling from ui_assets defs**, so that UI paths are data-driven. | Card texture via `SimAPI.get_ui_asset_path`; no literal `res://data/` in menu scripts. |
| OW-3.4 | As a **developer**, I want **a single form builder (`WorldGenCardFormBinder`)**, so that menu and future cards do not duplicate field lists. | All Generate paths call binder; no parallel hand-built dict in `WorldGenMenu.gd`. *(SA-3.1.)* |
| OW-3.5 | As a **player**, I want **world size preset to reduce sim cost for large preview counts**, so that Medium/Large runs stay interactive. | Binder sets `sim_voronoi_sites` when `voronoi_sites` exceeds preset cap; parser accepts `world_size` or explicit `sim_voronoi_sites`. *(SA-E4.)* |
| OW-3.6 | As a **player**, I want **world gen mode and SIMD toggles on the form**, so that I can pick determinism vs performance. | Form includes `world_gen_mode` (`default` / `max_determinism`) and `use_simd`; C++ selects `deterministic_globe` pipeline and parallel profile accordingly. |

### Scene: `world_gen_load_screen` → `worldGenLoadScreen.tscn` (step 4)

| ID | Story | Acceptance criteria |
|----|--------|---------------------|
| OW-4.1 | As a **player**, I want **live stage progress during overworld generation**, so that I know the sim is working. | Progress bar and status text from `poll_world_gen_async`; `WorldGenLoadScreenProgress` drives labels and heartbeats; stage ids match pipeline. *(SA-0.2, SA-4.1.)* |
| OW-4.2 | As a **player**, I want **to cancel long-running generation**, so that I can return to the form. | Cancel stops between pipeline stages; load screen handles cancelled state without crashing. *(SA-4.2.)* |
| OW-4.3 | As a **player**, I want **logs and performance timings when generation finishes**, so that I can diagnose slow stages. | Result includes `log_lines` and per-stage timings where implemented. |
| OW-4.4 | As a **developer**, I want **generation to persist seed and genome in the sim bridge**, so that game world streaming and overworld queries use the same session. | On success, `SimBridge` holds `WorldSeed` + `PlanetGenome`; overworld atlas persistence is OW-E2. |
| OW-4.5 | As a **developer**, I want **heavy marshal work on the main thread only after the worker joins**, so that Godot variants stay valid. | `take_world_gen_async_result` marshals on main thread; documented in scene order. *(SA-4.3.)* |
| OW-4.6 | As a **developer**, I want **the load screen to use async APIs only**, so that the UI thread never calls blocking `apply_world_gen_form`. | `_do_run_generation` starts async job and polls until done; sync API reserved for tools. |
| OW-4.7 | As a **player**, I want **stage titles from a catalog**, so that log lines are human-readable. | `WorldGenStageCatalog` maps `stage_id` → title/detail; catalog data-driven or shared with pipeline defs. |
| OW-4.8 | As a **player**, I want **a clear failure state when generation fails**, so that I am not stuck on Continue with empty data. | Failed job shows error message; Continue disabled or returns to menu; `ok: false` in result. |
| OW-4.9 | As a **player**, I want **warnings before very large overworlds**, so that I do not accidentally request hundreds of thousands of sites. | Confirm or cap from profile when `voronoi_sites` exceeds threshold; link to SA-E5. |

### Scene: `sphere_preview` → `SpherePreview.tscn` (step 5, debug)

| ID | Story | Acceptance criteria |
|----|--------|---------------------|
| OW-5.1 | As a **developer**, I want **an optional overworld preview after Continue**, so that I can validate plates and elevation without entering the game world. | `WorldGenPreviewApplier` pushes result dict; preview clears prior `SpherePreview` siblings. |
| OW-5.2 | As a **developer**, I want **preview to use profile-bound scene path**, so that preview is swappable per profile. | Path from `get_presentation_scene_path("planet_preview")`. |
| OW-5.3 | As a **developer**, I want **large overworlds to defer heavy mesh apply**, so that Continue does not hitch. | Sites/cells/mesh above threshold scheduled via timer (see `WorldGenPreviewApplier`). |
| OW-5.4 | As a **developer**, I want **preview geometry from sim Z-up converted once in presentation**, so that sim_core stays renderer-agnostic. | `PresentationCoords` used for sites, mesh, rivers; no second ad-hoc axis swap in `SpherePreview` except documented exceptions. *(SC-E7.)* |
| OW-5.5 | As a **developer**, I want **optional preview driven from `SimAPI.sample_surface` later**, so that preview is not the long-term data owner. | After OW-E4, overlay colours can sample atlas; marshal dict remains debug-only path until OW-E6. |

### Scene: `sphere_preview_overlay` → `SpherePreviewOverlay.tscn` (step 6)

| ID | Story | Acceptance criteria |
|----|--------|---------------------|
| OW-6.1 | As a **developer**, I want **to toggle debug layers** (sites, Delaunay, Voronoi, plates, terrain mesh maps), so that I can inspect overworld pipeline outputs. | Overlay toggles visibility without re-running generation. |

---

## Engineering stories (sim / data, no single scene)

| ID | Story | Acceptance criteria |
|----|--------|---------------------|
| OW-E1 | As a **gameplay system**, I want **overworld `PlanetSurfaceAtlas` built from `PlanetGlobe` after the pipeline**, so that runtime does not keep full half-edge mesh unless needed. | Atlas holds per-region plate, elevation, moisture, positions; neighbours CSR for adjacency. |
| OW-E2 | As a **gameplay system**, I want **`SimBridge` to own the overworld atlas for the session**, so that generation is not throwaway after marshal. | `set_world_gen` extended or companion `set_world_surface`; globe not required for basic queries. |
| OW-E3 | As a **gameplay system**, I want **`locate_region(unit_dir)` and `sample_surface`**, so that “where am I on the overworld?” is one sim call. | Returns region id + scalar fields + derived temperature/biome from defs. |
| OW-E4 | As a **developer**, I want **`SimAPI.sample_surface` (or equivalent)** exposing a small dict, so that Godot does not forward fifteen arrays. | Documented keys; preview may call same API for map colours later. |
| OW-E5 | As a **mod author**, I want **pipeline stages declared in XML**, so that packs can override order without recompiling. | New optional stages registered in `PlanetGlobePipeline`; unknown stage fails loudly in dev. |
| OW-E6 | As a **developer**, I want **preview export generated from the overworld atlas**, so that marshal and gameplay share one source. | `WorldGenMarshal` reads atlas + optional mesh builder; reduces duplicated field lists. |
| OW-E7 | As a **mod author**, I want **a prevailing-wind field on the overworld sphere**, so that climate stages can advect moisture consistently. | Wind direction (unit tangent or equivalent) per region or from a global vector + latitude banding; sourced from planet preset / genome (not hardcoded in C++); deterministic for same seed + form; optional pipeline stage `wind_field` registered in `PlanetGlobePipeline`. |
| OW-E8 | As a **player**, I want **moisture to advect with prevailing wind**, so that coasts and interiors look climatically connected—not only radially from the ocean. | Pipeline stage `moisture_advection` (or extended `moisture`) transports moisture along upwind→downwind edges on the region neighbour graph; ocean regions remain moisture sources; output still 0–1 per region; runs after elevation and before `triangle_values`; same seed + pipeline → same fields (`deterministic_globe` bit-identical). |
| OW-E9 | As a **player**, I want **rain shadows behind mountain ranges**, so that leeward slopes and basins can be arid while windward slopes stay wet. | When moisture crosses a ridge (positive elevation gain along wind path), windward side gains orographic lift bonus and leeward side loses moisture by a configurable factor; uses region elevation + wind from OW-E7/E8; visible in preview moisture colouring and in downstream river/biome fields; stage id `rain_shadow` (or folded into advection) listed in `world_gen_pipelines.xml` after elevation. |
| OW-E10 | As a **developer**, I want **optional debug visualisation of wind and rain shadow**, so that I can validate climate stages without re-running the full pipeline. | `SpherePreviewOverlay` toggle (or profile flag) shows wind arrows and/or moisture delta from pre-advection baseline; no new marshal-only moisture arrays as the long-term source of truth (align with OW-E6). |

### Terrain & hydrology refinement (Orogen-inspired, ideas not code)

Reference: [World Orogen](https://github.com/raguilar011095/planet_heightmap_generation) (GPL-3.0 — reimplement algorithms only; do not copy JS). Complements elevation smoothing already in `RegionElevationSmooth` / `prepare_elevation_for_terrain_mesh`.

| ID | Story | Effort | Impact | Acceptance criteria |
|----|--------|--------|--------|---------------------|
| OW-E11 | As a **player**, I want **every land region to drain toward the ocean before rivers are computed**, so that river paths are dendritic instead of spaghetti in trapped basins. | Medium | High | New pipeline stage `priority_flood` (or pre-pass in `erosion`) runs Barnes-style priority-flood pit resolution on **region** elevation before `river_downflow`; land cells gain monotonic drainage to open ocean; saddle spill points carved (not only filled); stage registered in `PlanetGlobePipeline` and listed in `world_gen_pipelines.xml` before `river_downflow`; deterministic under `deterministic_globe`; `worldgen_bench` reports fewer steep orphan flow branches (metric TBD). |
| OW-E12 | As a **developer**, I want **dual-folded terrain triangles to face outward after displacement**, so that preview shading is consistent and inward-face warnings drop. | Low | High | `generate_planet_terrain_mesh_dual_folded` (or post-pass) checks each triangle normal against centroid; swaps winding when dot(normal, centre) &lt; 0; `worldgen_bench` inward-triangle count &lt; 5% on coarse-upsampling path; no change to sim elevation fields. |
| OW-E13 | As a **player**, I want **gentler terrain displacement with softer ocean floors**, so that peaks and trenches look natural on the 3D preview mesh. | Low | Medium | Shared constant(s) for radial scale: land `0.04` (was `0.08`), subsea `0.3×` land scale; applied consistently in `PlanetTerrainMesh`, `WorldGenPreviewExport`, and any duplicate `k_elevation_scale`; visual regression: fewer needle spikes at same region count; bench `max_tri_radius_spread` not worse than post-smooth baseline. |
| OW-E14 | As a **player**, I want **river valleys carved into the terrain mesh by default**, so that major rivers read as depressions not only as overlay lines. | Trivial | Medium | `river_carve` stage added to `default_globe` (and `performance_globe`) in `world_gen_pipelines.xml` after `erosion`; `carve_rivers_from_flow` runs when pipeline completes; preview shows deeper channels along high-`s_flow` edges; stage catalog / load screen lists `river_carve`. |
| OW-E15 | As a **player**, I want **elevation bands smoothed without flattening mountain ridges**, so that plate-boundary artefacts blend before mesh displacement. | Medium | Medium | Bilateral (edge-preserving) smooth on region elevation — neighbour weight falls off with elevation difference; coastline / sea-level cells optionally locked; runs after `elevation` (or replaces part of Laplacian passes); tunable pass count via preset or pipeline; pairs with existing dual Laplacian in `prepare_elevation_for_terrain_mesh`. |
| OW-E16 | As a **player**, I want **rivers to deepen iteratively as flow accumulates**, so that valley networks stabilise and match drainage area. | High | High | Replace or augment single-pass `apply_hydraulic_erosion` with iterative implicit stream-power erosion on **regions** (Braun–Willett style): rebuild steepest-descent / flow each iteration; optional sediment deposit in flat receivers; N iterations from preset or pipeline def; runs after OW-E11 when both enabled; bit-identical under `deterministic_globe` for fixed iteration count; preview rivers align with carved valleys. |
| OW-E17 | As a **player**, I want **optional domain-warped coastlines, glacial valleys, and full climate**, so that overworlds can reach Orogen-level realism when content asks for it. | High | Low (rivers) / High (realism) | Separate optional pipeline stages: `terrain_warp` (FBM domain warp on region elevation), `glacial_erosion`, extended climate (beyond OW-E7–E9); each stage optional in XML; defaults off in `default_globe`; documented in [worldgen_modern.md](worldgen_modern.md); no GPL code import. |
| OW-E18 | As a **developer**, I want **region triangle rings validated before hydrology**, so that rivers and dual terrain are not built on broken dual topology. | Medium | High | SC-E1 metrics pass on reference configs; optional `validate_topology` stage; pipeline abort or loud error in dev when rings invalid. *(SC-E4, SC-E5.)* |
| OW-E19 | As a **developer**, I want **marshal/preview export capped for huge region counts**, so that Godot does not allocate multi-gigabyte dicts. | Medium | High | Skip or simplify debug half-edge / river arrays above `k_max_regions_with_debug_mesh`; document limits in scene order; optional LOD export profile. |
| OW-E20 | As a **developer**, I want **overworld generation to release transient globe memory after atlas build**, so that session RAM stays bounded. | Medium | Medium | After OW-E1, drop half-edge mesh and triangle buffers not needed for atlas; retain CSR neighbours + per-region fields. |

**Note:** OW-E11 and OW-E16 change **region** drainage/elevation before or instead of triangle-only mapgen4 flow; keep `assign_downflow` / `assign_flow` until replaced explicitly. OW-E12–E14 are preview/mesh quality; OW-E15 bridges elevation assign and terrain mesh. OW-E18–E20 are platform/QA hygiene tied to [epic_sim_correctness.md](epic_sim_correctness.md).

**Suggested order:** OW-E14 → OW-E12 → OW-E13 → OW-E11 → OW-E15 → OW-E16 → OW-E17.


---

## Phases (suggested sprint order)

| Phase | Stories | Deliverable |
|-------|---------|-------------|
| **G0 — Baseline** | OW-0.*, OW-1.*, OW-3.*, OW-4.1–4.5 | Current flow stable, documented. |
| **G0b — Platform** | [SA-*](epic_sim_api_platform.md) P0–P1, OW-4.6–4.9, OW-1.3, OW-3.4–3.6 | Merged boot, async load screen, binder form, profile paths. |
| **G0c — Correctness gates** | [SC-E1–E3](epic_sim_correctness.md), OW-E18 | Bench CI; topology validated before rivers. |
| **G1 — Data layer** | OW-E1, OW-E2, OW-E3, OW-E20 | Overworld atlas in bridge; locate + sample in C++; transient globe freed. |
| **G2 — API & preview** | OW-E4, OW-E6, OW-5.*, OW-E19 | SimAPI query; preview optional from atlas; marshal caps. |
| **G3 — Climate refinement** | OW-E7, OW-E8, OW-E9, OW-E10 | Wind field, moisture advection, rain shadow; optional preview overlays. |
| **G3b — Terrain & hydrology** | OW-E14, OW-E12, OW-E13, OW-E11, OW-E15, OW-E16 | River carve default, mesh winding, displacement tuning, priority-flood, bilateral smooth, stream-power erosion. |
| **G4 — Content** | OW-E5 + biome derivation | Biome id from `biomes.xml` rules at sample time (uses refined moisture). |
| **G4b — Advanced terrain (optional)** | OW-E17 | Domain warp, glacial erosion, extended climate stages. |
| **G5 — Persistence** | OW-P1 (new below) | Load overworld without re-running full pipeline (optional). |

### Persistence (late phase)

| ID | Story | Acceptance criteria |
|----|--------|---------------------|
| OW-P1 | As a **player**, I want **to save and load overworld session header**, so that I can resume without regenerating. | Save stores seed, genome, atlas checksum or blob; load restores `SimBridge` atlas; format versioned. |

---

## Technical reference

| Topic | Location |
|-------|----------|
| Pipeline stages | `data/core/defs/world_gen_pipelines.xml`, `PlanetGlobePipeline.cpp` |
| Globe struct (overworld build) | `gde/sim_core/include/world/PlanetGlobe.hpp` |
| Job + marshal | `WorldGenJob.cpp`, `WorldGenMarshal.cpp` |
| Form parser | `gde/src/WorldGenFormParser.cpp` |
| Algorithms | [worldgen_modern.md](worldgen_modern.md) (§7 climate: wind advection, rain shadow), [worldgen.md](worldgen.md) |
| Moisture (current) | `MoistureAssigner.cpp` — ocean-distance BFS; OW-E7–E9 extend |
| Terrain mesh | `PlanetTerrainMesh.cpp`, `RegionElevationSmooth.cpp` — OW-E11–E16 |
| External reference (ideas only) | [World Orogen](https://github.com/raguilar011095/planet_heightmap_generation), [RBG 1843](https://github.com/redblobgames/1843-planet-generation) |

---

## Related epics

- **[Critical issues audit](critical_bugs_audit.md)** — code review findings mapped to stories below.
- **[Sim API & platform](epic_sim_api_platform.md)** — boot, async job, form binder, profile paths.
- **[Sim correctness & QA](epic_sim_correctness.md)** — bench gates, topology validation.
- **[Game world streaming](epic_game_world_streaming.md)** — game world proc gen from overworld, chunks, player edits.
- **[Game scene order](game_scene_order.md)** — maintained step table when scenes change.
