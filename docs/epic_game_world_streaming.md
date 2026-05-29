# Epic: Game world streaming

**Status:** Planned  
**Owner:** —  
**Last updated:** 2026-05-28 (stories expanded: intents, preview transition, sim unload)

Living epic for the **game world**: local, high-detail simulation around the player. Chunks are streamed from the sim, generated from **overworld** context, and **mutable** by player intents (dig, build, environment changes).

Depends on **[epic_overworld_generation.md](epic_overworld_generation.md)** (overworld atlas, seed, genome). Scene flow: [game_scene_order.md](game_scene_order.md) steps 0–2 and 7; play mode eventually replaces or follows overworld debug preview (step 5).

**Known gaps (2026-05-28):** [critical_bugs_audit.md](critical_bugs_audit.md) — items 1, 7, 8 (and chunk unload in additional risks).

---

## Terminology

| Term | Meaning |
|------|---------|
| **Overworld** | Macro generated planet (spherical atlas). Read-only baseline; built in the overworld generation epic. |
| **Game world** | Micro generated, streamed world where the player walks and edits the environment. |
| **Planet preview** | Debug view of the overworld (`SpherePreview`), not the game world. |

---

## Vision

The overworld is huge and stored as a **low-resolution spherical atlas**. Where the player stands in the **game world**, the sim runs a **second procedural pass** at metre scale, streams **chunks** in and out, and keeps **authoritative** cell state (heights, surfaces, later layers). Godot instantiates `ChunkNode` scenes from diffs only; player tools send **intents** that mutate the sim and emit `CellChanged` updates.

---

## Goals

| Goal | Notes |
|------|--------|
| Overworld-constrained game world gen | Chunk content seeded from `WorldSeed` + overworld atlas sample at chunk location (plate, elev, moist, biome). |
| Sim-owned detail | `ChunkStore` is source of truth; not Godot arrays. |
| Stream by focus | `WorldViewManager` requests radius around player; unload far chunks. |
| Player edits | Intents change local layers; diffs sync the view. |
| Clear coordinates | Documented map: `ChunkCoord` ↔ point on sphere ↔ overworld region. |
| Preview coexistence | Game world streaming disabled while `SpherePreview` present until “enter game world” story ships. |

---

## Out of scope (this epic)

- Overworld pipeline stages → [overworld generation](epic_overworld_generation.md).
- Full overworld mesh rendering at gameplay LOD.
- Multiplayer sync of chunk edits.
- Advanced ecology / fluid sim (future systems on top of chunk layers).

---

## Architecture (target)

```text
Overworld PlanetSurfaceAtlas (read-only)
    +
WorldSeed + PlanetGenome
    ↓
GameWorldLocalMap (chunk ↔ tangent frame ↔ unit_dir)
    ↓
GameWorldChunkGenerator.generate(coord) → Chunk (layers)
    ↓
ChunkStore (loaded set + edit overlay)
    ↓
DiffQueue: ChunkLoaded | ChunkUnloaded | CellChanged
    ↓
WorldViewManager → ChunkNode.tscn (view)
    ↑
IntentRouter ← apply_intent (dig, …)
```

| Tier | Store | Mutable |
|------|--------|---------|
| Overworld | Atlas + genome | No (baseline) |
| Game world (chunk) | 16×16 cells (+ vertices), multi-layer | Yes |

---

## Dependencies

- **OW-E1–E3** (overworld atlas + `sample_surface` / region locate) from overworld generation epic.
- `GrowthSim`: `request_chunks`, `poll_diffs`, `apply_intent`.
- `game_profile` → `world_view` script `chunk_streaming` → `WorldViewManager.cs`.
- Registry: `chunk_node` → `ChunkNode.tscn`.

---

## User stories by scene

Story ids use prefix **GW-** (game world).

### Scene: `SimAPI` autoload (step 0)

| ID | Story | Acceptance criteria |
|----|--------|---------------------|
| GW-0.1 | As a **developer**, I want **game world streaming and intents only through SimAPI**, so that the same moddability rules as overworld gen apply. | `request_chunks`, `poll_diffs`, `apply_intent` on backend; stub documents missing DLL. |
| GW-0.2 | As a **gameplay system**, I want **`sample_surface` alongside chunk APIs**, so that UI can show overworld biome/climate at the player’s location. | Single call returns overworld context; used by HUD later (optional in first slice). |

### Scene: `Main.tscn` (step 1)

| ID | Story | Acceptance criteria |
|----|--------|---------------------|
| GW-1.1 | As a **player**, I want **the game world to tick in `_process` when not in overworld preview**, so that streaming follows the camera/focus. | `Main.gd` calls `WorldViewManager.update_streaming` when `SpherePreview` absent. |
| GW-1.2 | As a **player**, I want **entering the game world after overworld gen**, so that I walk on detailed terrain, not only the debug sphere. | Story GW-7.0: transition removes preview and starts streaming (new flow). |
| GW-1.3 | As a **developer**, I want **`WorldRoot` and `Focus` stable across menu → gen → play**, so that chunk coords stay consistent. | Same `Main` instance; no scene file change for play. |

### Scene: `WorldViewManager` script — `chunk_streaming` (step 2)

| ID | Story | Acceptance criteria |
|----|--------|---------------------|
| GW-2.1 | As a **developer**, I want **streaming setup from profile `world_view`**, so that script path is not hardcoded in `Main.gd`. | `setup(world_root, focus)` after profile resolves `chunk_streaming` script. |
| GW-2.2 | As a **player**, I want **chunks loaded in a radius around my focus**, so that I only simulate nearby game world terrain. | `request_chunks(center, StreamRadiusChunks)` each frame or throttled; matches sim loaded set. |
| GW-2.3 | As a **player**, I want **distant chunk meshes removed**, so that frame cost stays bounded. | `UnloadFarChunks` removes nodes; sim emits `ChunkUnloaded` when sim policy unloads (GW-E9). |
| GW-2.4 | As a **developer**, I want **streaming state reset when entering or leaving overworld preview**, so that stale chunk references do not break play. | `WorldViewManager` exposes `reset_streaming()`; called when `SpherePreview` added/removed; `_chunks` cleared; sim optional unload all. *(GW-7.0c.)* |
| GW-2.5 | As a **developer**, I want **Main to gate streaming consistently**, so that preview and game world never tick chunks together. | `update_streaming` skipped when preview present; entering play (GW-7.0) calls reset then enables streaming. |

### Scene: `chunk_node` → `ChunkNode.tscn` (step 7)

| ID | Story | Acceptance criteria |
|----|--------|---------------------|
| GW-7.1 | As a **player**, I want **terrain meshes from sim height grids**, so that what I see matches simulation. | `ChunkLoaded` → `apply_height_grid(samples, coord)` builds mesh; 17×17 samples for 16×16 cells. |
| GW-7.2 | As a **player**, I want **chunk placement consistent with world coords**, so that chunks tile without gaps. | World position = `chunk_coord * chunk_size * cell_size` (same constants as `WorldViewManager`). |
| GW-7.3 | As a **developer**, I want **incremental mesh updates on `CellChanged`**, so that digging does not reload entire chunk from scratch. | `ApplyCellChanged` patches height/layer (today no-op; implement with GW-E4). |

### Scene: `sphere_preview` (step 5 — transition)

| ID | Story | Acceptance criteria |
|----|--------|---------------------|
| GW-7.0 | As a **player**, I want **Play / Enter game world after overworld preview (or skip preview)**, so that I leave the debug globe and stream local terrain. | Button or profile flag: remove `SpherePreview`, restore `WorldRoot` streaming; overworld atlas already in sim. |
| GW-7.0b | As a **developer**, I want **overworld preview and game world streaming mutually exclusive in Main**, so that we do not double-sim or leak chunk nodes. | `ClearWorldTerrain` clears `WorldRoot` children; GW-2.4 clears view cache; documented in scene order. |
| GW-7.0c | As a **developer**, I want **preview entry to notify the streaming controller**, so that chunk nodes are not left in a dangling dictionary. | `SpherePreview` or applier calls `WorldViewManager.reset_streaming` after clearing terrain. |

### Scenes: gameplay UI (registered, not in default profile)

| ID | Story | Acceptance criteria |
|----|--------|---------------------|
| GW-UI.1 | As a **player**, I want **tools to send dig/build intents**, so that I alter the game world environment. | `apply_intent` with `interaction_id` from `interactions.xml`; routed by `IntentRouter` (GW-E10, GW-E11). |
| GW-UI.2 | As a **player**, I want **inspect panel to show overworld + game world cell info**, so that I understand where I am. | Uses `sample_surface` + local cell query API (when exposed). |

*Wire `inspectPanel.tscn`, `buildMenu.tscn`, etc. when added to `game_profile`.*

---

## Engineering stories (sim / data)

| ID | Story | Acceptance criteria |
|----|--------|---------------------|
| GW-E1 | As a **developer**, I want **`GameWorldLocalMap` defining chunk ↔ unit_dir`**, so that overworld samples align with flat streaming coords. | One module; anchor or region-local frame documented; used by generator and `sample_surface` at player position. |
| GW-E2 | As a **gameplay system**, I want **`GameWorldChunkGenerator` using overworld atlas + seed per coord`**, so that game world terrain is deterministic and overworld-shaped. | Replacing zeroed `Chunk` on first load; height reflects elev/moist/biome at sample. |
| GW-E3 | As a **developer**, I want **loaded chunks to survive regen unless invalidated**, so that player edits are not wiped on re-request. | Policy: if chunk exists in store, do not regenerate; edits persist in memory. |
| GW-E4 | As a **player**, I want **dig intents to lower height and emit `CellChanged`**, so that the view updates. | `apply_intent_dig` mutates `ChunkStore`; diff consumed by `WorldViewManager`. |
| GW-E5 | As a **gameplay system**, I want **cell layers beyond height** (surface, moisture override), so that environment edits are rich. | `CellChangedDiff.layer` used; chunk struct holds per-layer data or sparse map. |
| GW-E6 | As a **gameplay system**, I want **game world gen rules in defs** (biome spawn tables), so that mods change detail without C++ recompile. | Generator reads `biomes.xml` / future `game_world_gen_rules.xml` via `DefDatabase`. |
| GW-E7 | As a **player**, I want **save/load of game world chunk edit blobs**, so that alterations survive sessions. | Save format: seed + genome + overworld atlas id + per-chunk deltas or region patches. |
| GW-E8 | As a **developer**, I want **optional `GameWorldPatch` per overworld region**, so that km-scale variation exists before 16 m chunks. | Intermediate tier if single region scalar is too coarse; chunks sample patch not one elev value. |
| GW-E9 | As a **developer**, I want **`ChunkStore` to unload far chunks in sim**, so that memory and loaded-set match the view radius. | `request_chunks` unloads coords outside radius + margin; emits `ChunkUnloaded`; policy preserves edited chunks (GW-E3). |
| GW-E10 | As a **developer**, I want **`IntentRouter` wired from GDExtension intents**, so that mod interaction defs choose handlers. | `IntentHandler` builds `IntentRequest`, calls `IntentRouter::apply`; no hardcoded `"dig"` only path. |
| GW-E11 | As a **developer**, I want **dig handler registered on `IntentRouter`**, so that `apply_intent_dig` is not a separate dead code path. | Handler mutates height in `ChunkStore`, pushes `CellChanged`; replaces empty `SimBridge::apply_intent_dig` body. |
| GW-E12 | As a **gameplay system**, I want **chunk height generation to read overworld atlas**, so that local terrain reflects macro elevation. | `GameWorldChunkGenerator` calls `sample_surface` at chunk anchor/corners; GW-E2 depends on OW-E3. |
| GW-E13 | As a **developer**, I want **`WorldViewManager` resolved from profile `world_view`**, so that `Main.gd` does not hardcode the C# script path. | `get_presentation_script_path` or script_id from profile; SA epic alignment. |

---

## Phases (suggested sprint order)

| Phase | Stories | Deliverable |
|-------|---------|-------------|
| **S0 — Skeleton** | GW-0.1, GW-1.1, GW-2.1–2.3, GW-7.1–7.2 | Current stub streaming visible (flat zero terrain). |
| **S0b — Preview coexistence** | GW-2.4–2.5, GW-7.0b–7.0c | No stale chunk cache; streaming gated. |
| **S1 — Local map + gen** | GW-E1, GW-E2, GW-E12, OW-E1–E3 (dependency) | Game world chunks generated from overworld; walkable height. |
| **S2 — Play transition** | GW-7.0, GW-1.2, GW-E13 | Leave overworld preview; stream game world around focus. |
| **S3 — Edits** | GW-E3, GW-E4, GW-E10, GW-E11, GW-7.3, GW-UI.1 | Dig works end-to-end via IntentRouter. |
| **S3b — Sim unload** | GW-E9 | Sim and view loaded sets aligned. |
| **S4 — Layers & content** | GW-E5, GW-E6, GW-UI.2 | Surface/moisture layers; inspect uses sim queries. |
| **S5 — Persistence** | GW-E7 | Save/load edited game world chunks. |
| **S6 — Patch tier (optional)** | GW-E8 | Region-scale intermediate field if needed. |

---

## Coordinate and scale notes

| Constant | Value today | Implication |
|----------|-------------|-------------|
| `CHUNK_SIZE` | 16 cells | |
| `WorldViewManager` cell size | 1 m | 256 m × 256 m per chunk |
| Overworld regions | 1k–65k sites | One region ≠ one chunk; generator must sample corners or patch |

---

## Technical reference

| Topic | Location |
|-------|----------|
| Chunk sim (game world) | `Chunk.hpp`, `ChunkStore.cpp`, `SimBridge.cpp` |
| Diffs | `Diff.hpp`, `DiffConverter.cpp` |
| Intents | `IntentRouter.hpp`, `gde/src/IntentHandler.cpp` |
| Godot stream | `godot/world/WorldViewManager.cs`, `ChunkNode` scene |
| Preview gate | `godot/main/Main.gd` (skip game world streaming if `SpherePreview` present) |

---

## Related epics

- **[Critical issues audit](critical_bugs_audit.md)** — intents and streaming findings.
- **[Overworld generation](epic_overworld_generation.md)** — overworld atlas and world gen scenes.
- **[Sim API & platform](epic_sim_api_platform.md)** — boot, async gen, profile paths.
- **[Game scene order](game_scene_order.md)** — update when “enter game world” or new play scene is added.
- **[Moddability](moddability.md)** — intents and defs.
