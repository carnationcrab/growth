# Epic: Sim API & platform foundation

**Status:** In progress  
**Owner:** —  
**Last updated:** 2026-05-28

Living epic for the **Godot ↔ C++ boundary**: `SimAPI` autoload, `GrowthSim` GDExtension, session boot, async job lifecycle, and moddability compliance on paths and data roots. Overworld **algorithms** live in [epic_overworld_generation.md](epic_overworld_generation.md); game world **streaming** in [epic_game_world_streaming.md](epic_game_world_streaming.md).

Scene flow: [game_scene_order.md](game_scene_order.md) steps 0–1 and cross-cutting load-screen behaviour (step 4).

**Known gaps (2026-05-28):** [critical_bugs_audit.md](critical_bugs_audit.md) — items 2, 3, 4, 5, 6 and hardcoded paths.

---

## Vision

Gameplay scripts call **only** `SimAPI`. The C++ sim owns long-running work on worker threads, exposes poll/cancel/take for world gen, and boots from a **merged data root** (`res://data/`) so core + mod packs share one `DefDatabase`. Presentation paths resolve from the same boot profile—no hardcoded `res://godot/...` scene loads in `Main` or menus.

---

## Goals

| Goal | Notes |
|------|--------|
| Non-blocking world gen | Worker runs pipeline; Godot polls progress; marshal on main thread after join. |
| Single job contract | One active `WorldGenJob` per session; clear states: idle, running, done, cancelled, failed. |
| Merged defs at boot | `manifest.xml` at data root; `DefDatabase` sees core + enabled mods. |
| Profile-driven scenes | Entry menu, load screen, preview from `game_profile.xml` only. |
| Lint-clean boot | `godot_boot_data_root`, `godot_literal_res_path` pass on gameplay scripts. |

---

## Out of scope

- Overworld pipeline stages and atlas fields → overworld epic.
- Chunk generation and dig mutation logic → streaming epic (this epic wires **APIs** only).
- Save/load file format.

---

## Architecture (target)

```text
Godot SimAPI
    → GrowthSimBackend → GrowthSim (GDExtension)
        → SimBridge (session)
            → DefDatabase (boot)
            → WorldGenJob (async, optional persistent)
            → ChunkStore / IntentRouter (streaming epic)
```

| Layer | Responsibility |
|-------|----------------|
| **SimAPI.cs** | Thin facades: boot, async world gen, step, chunks, intents, presentation paths. |
| **GrowthSim** | Bind C++ methods; no business logic in `gde/src` beyond parse/marshal/intent glue. |
| **SimBridge** | Owns defs, seed, genome, overworld atlas (overworld epic), job handle. |

**Anti-patterns:** Stack-allocating `WorldGenJob` inside `apply_world_gen_form` and joining on the caller thread; booting `data/core/` only; duplicating form dict construction in `WorldGenMenu.gd` instead of `WorldGenCardFormBinder`.

---

## Dependencies

- `WorldGenJob` in `gde/sim_core` (worker thread exists today).
- [moddability.md](moddability.md) linter rules.
- Overworld marshal keys: `WorldGenPreviewKeys.cs` / `WorldGenMarshal.cpp`.

---

## User stories by scene

Story ids use prefix **SA-** (sim API).

### Scene: `SimAPI` autoload (step 0)

| ID | Story | Acceptance criteria |
|----|--------|---------------------|
| SA-0.1 | As a **developer**, I want **`SimAPI.boot` to load the merged data root**, so that mod packs contribute defs and pipelines. | `Main` (and tests) call `boot` with `res://data/` (or globalised equivalent); `DefDatabase` loads root `manifest.xml`; `godot_boot_data_root` lint passes. |
| SA-0.2 | As a **developer**, I want **async world gen methods on SimAPI**, so that UI never blocks on the pipeline. | `start_world_gen_async(form)`, `poll_world_gen_async()`, `cancel_world_gen_async()`, `take_world_gen_async_result()` documented and callable from GDScript/C#. |
| SA-0.3 | As a **developer**, I want **sync `apply_world_gen_form` retained for headless/tools**, so that benches and quick tests still work. | Sync path joins worker and returns dict; documented as blocking; load screen uses async API in production flow. |
| SA-0.4 | As a **developer**, I want **stub backend behaviour explicit**, so that missing DLL fails world gen with `ok: false` and clear error. | Stub returns empty geometry + error string; no silent success. |

### Scene: `Main.tscn` (step 1)

| ID | Story | Acceptance criteria |
|----|--------|---------------------|
| SA-1.1 | As a **developer**, I want **Main to boot sim before any UI**, so that presentation paths and defs exist for menus. | `_ready` calls `SimAPI.boot(data_root)` once; data root is merged pack root, not core-only. |
| SA-1.2 | As a **developer**, I want **entry menu from profile binding**, so that `Main` has no hardcoded menu scene path. | `get_presentation_scene_path("entry_menu")`; `godot_literal_res_path` lint passes on `Main.gd`. |

### Scene: `world_gen_menu` (step 3)

| ID | Story | Acceptance criteria |
|----|--------|---------------------|
| SA-3.1 | As a **developer**, I want **world gen menu to use `WorldGenCardFormBinder`**, so that form fields stay in one place. | `WorldGenMenu` (or shared card controller) calls binder `build_form_dict()`; includes `sim_voronoi_sites`, `world_gen_mode`, `use_simd` when UI present. |
| SA-3.2 | As a **developer**, I want **Generate to open load screen via profile path**, so that menu does not hardcode `worldGenLoadScreen.tscn`. | Uses `WorldGenLoadScreenLauncher` or `get_presentation_scene_path("world_gen_load_screen")`. |

### Scene: `world_gen_load_screen` (step 4)

| ID | Story | Acceptance criteria |
|----|--------|---------------------|
| SA-4.1 | As a **player**, I want **the load screen to poll async generation each frame**, so that progress and heartbeats update while C++ runs. | `_process` or timer calls `poll_world_gen_async`; `WorldGenLoadScreenProgress.update_from_poll` drives bar and log. |
| SA-4.2 | As a **player**, I want **Cancel on the load screen**, so that I can abort a long run. | Cancel button calls `cancel_world_gen_async`; UI shows cancelled state; no crash on `take` after cancel. |
| SA-4.3 | As a **developer**, I want **marshal only after `take_world_gen_async_result` on main thread**, so that Godot variants are valid. | Documented in scene order; aligns with OW-4.5. |

---

## Engineering stories

| ID | Story | Acceptance criteria |
|----|--------|---------------------|
| SA-E1 | As a **developer**, I want **`WorldGenJob` owned by `SimBridge`**, so that poll/cancel work across frames. | Single job instance; `start` rejects if already running; `take_result` joins and clears state. |
| SA-E2 | As a **developer**, I want **GrowthSim GDExtension methods bound for async world gen**, so that C# can delegate without reimplementing threads. | Methods in `sim_api_gdextension.cpp` + `SimAPI` engines; no duplicate job in sync path except documented wrapper. |
| SA-E3 | As a **developer**, I want **`PresentationSceneEngine` to prefer boot def root**, so that profile/registry match sim defs. | After SA-0.1, fallback to `res://data/core/` only if boot path empty (dev convenience). |
| SA-E4 | As a **developer**, I want **`WorldGenFormParser` to honour `world_size`**, so that size preset affects coarse sim sites when UI sends index only. | Parser maps `world_size` → `sim_voronoi_sites` using same table as `WorldGenCardPaths` (or def); documented mapping. |
| SA-E5 | As a **player**, I want **guardrails when Voronoi site count is extreme**, so that accidental 500k sites do not freeze or OOM without warning. | Confirm dialog or soft cap from def/profile; load screen shows estimated cost; optional `max_voronoi_sites` in `game_profile.xml`. |
| SA-E6 | As a **developer**, I want **pre-commit / CI to run moddability + sconstruct lints**, so that platform regressions are caught. | Documented in README; optional CI job listed in this epic’s phases. |

---

## Phases

| Phase | Stories | Deliverable |
|-------|---------|-------------|
| **P0 — Boot & paths** | SA-0.1, SA-1.1–1.2, SA-3.2, SA-E3, SA-E6 | Merged defs; profile-driven menu/load paths. |
| **P1 — Async API** | SA-0.2, SA-E1, SA-E2, SA-4.1–4.3, OW-4.1–4.2 (overworld epic) | Non-blocking load screen with cancel. |
| **P2 — Form contract** | SA-3.1, SA-E4, SA-E5, OW-3.1 (overworld) | Binder-only form; size preset affects sim resolution. |
| **P3 — Polish** | SA-0.3–0.4 | Sync path documented; stub errors clear. |

---

## Technical reference

| Topic | Location |
|-------|----------|
| SimAPI | `godot/autoload/SimAPI.cs`, `godot/autoload/sim/*` |
| GDExtension | `gde/src/sim_api_gdextension.cpp` |
| Job | `gde/sim_core/include/api/WorldGenJob.hpp` |
| Form binder | `godot/ui/menus/world_gen/WorldGenCardFormBinder.gd` |
| Load screen progress | `godot/ui/menus/world_gen/WorldGenLoadScreenProgress.gd` |
| Linter | `tools/moddability_lint.yaml`, `godot_boot_data_root` |

---

## Related epics

- [Critical issues audit](critical_bugs_audit.md) — source findings for SA-* stories.
- [Overworld generation](epic_overworld_generation.md) — pipeline, atlas, preview.
- [Game world streaming](epic_game_world_streaming.md) — chunks and intents (API surface here).
- [Sim correctness & QA](epic_sim_correctness.md) — bench gates and topology validation.
