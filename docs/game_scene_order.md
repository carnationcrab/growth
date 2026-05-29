# Game scene order

**Living document.** Keep this file aligned with the real boot and presentation flow whenever you add, remove, or reorder scenes, change `game_profile.xml` bindings, or alter what happens on screen during load.

Known implementation gaps vs this target flow: **[critical_bugs_audit.md](critical_bugs_audit.md)**.

Source of truth for *which* scenes run: `data/core/defs/game_profile.xml` → `scene_registry.xml`. This doc describes the **current default profile** (`profile_id="default"`).

**Terminology:** **Overworld** = generated macro planet (steps 3–6, sim pipeline). **Game world** = streamed local simulation (step 7). See [README.md](README.md).

---

## Boot sequence (summary)

| Step | Scene / node | How loaded | Data (sim / defs) | On screen |
|------|----------------|------------|-------------------|-----------|
| 0 | `SimAPI` autoload | `project.godot` autoload | C++ `GrowthSim` if built, else stub; defs loaded on `boot` (target: merged `res://data/`, see SA-0.1) | — |
| 1 | `Main.tscn` | `run/main_scene` | `SimAPI.boot(data_root)` — manifest, merged defs, profile bindings | Empty 3D world: `WorldRoot`, orbit `Focus` + camera + light, grey environment |
| 2 | `WorldViewManager` (script, not `.tscn`) | Child of `Main` | Registers `chunk_node` path; streaming ready | No chunks yet |
| 3 | `worldGenMenu.tscn` | Instantiated on `UILayer` in `Main._ready` | UI textures from `ui_assets.xml` via `SimAPI` | World-gen form (seed, size, climate sliders, plates, optional terrain mesh) |
| 4 | `worldGenLoadScreen.tscn` | Child of `UILayer` after **Generate** | **Today:** blocking `SimAPI.apply_world_gen_form(form)`. **Target:** `start_world_gen_async` + poll + `take_world_gen_async_result` (SA-4.1, OW-4.6) | Progress bar, stage log, **Continue** (hidden until gen finishes); Cancel (target: OW-4.2) |
| 5 | `SpherePreview.tscn` | Child of `Main` after **Continue** | Marshal dict for debug meshes; **`PlanetSurfaceAtlas` in `SimBridge`** (sites, topology tris, region neighbours, plate/elev/moisture per region) | **Overworld** debug view (not game world); menu removed |
| 6 | `SpherePreviewOverlay.tscn` | Child of `SpherePreview`'s `OverlayLayer` | Layer toggles; **Start** → `commit_overworld_for_play` + `enter_game_world` | Layer panel (left); **Start** button (bottom-right) |
| 7 | `ChunkNode.tscn` | On demand under `WorldRoot` via `WorldViewManager` | `ChunkLoaded` diffs from `SimAPI` (stub or sim) | **Game world** chunks — **disabled while `SpherePreview` is present** |

There is no `ChangeSceneToFile` today: one persistent `Main` scene; UI and preview are **added and removed as children**.

---

## Step-by-step detail

### 0 — Autoload: `SimAPI`

- **Path:** `res://godot/autoload/SimAPI.cs`
- **When:** Before any scene tree node.
- **Data:** Instantiates `GrowthSim` GDExtension when available; otherwise stub backend with XML fallbacks for assets and scene registry.
- **Screen:** Nothing.

### 1 — Main scene

- **Path:** `res://godot/main/Main.tscn` (`Main.gd`)
- **When:** Application start.
- **Data:**
  - `SimAPI.boot(data_root)` — **today** `res://data/core/`; **target** merged `res://data/` (SA-0.1). Active `game_profile.xml`.
  - Each frame: `SimAPI.step(delta, 1.0)`.
- **Screen:** 3D rig (`Focus` / `Camera3D` / `DirectionalLight3D`), `WorldEnvironment`, empty `WorldRoot`.

### 2 — World streaming controller (script)

- **Resolved as:** `script_id` `chunk_streaming` → `res://godot/world/WorldViewManager.cs`
- **When:** `_ready` on `Main`, before entry menu.
- **Data:** `setup(WorldRoot, Focus)`; loads `chunk_node` scene path from registry.
- **Screen:** None until chunks load (step 7).

### 3 — Entry menu (world gen form)

- **Binding:** `entry_menu` → `scene_id` `world_gen_menu` → `res://godot/ui/menus/worldGenMenu.tscn`
- **When:** End of `Main._ready` on `CanvasLayer` named `UILayer`.
- **Data:** Form fields only in UI until Generate; assets from `ui_assets.xml`.
- **Screen:** Centred card: seed, world size, temperature / precipitation %, Voronoi points, jitter, plate count, “planet terrain mesh” checkbox, **Generate** / **Back** (Back not wired to another scene).

### 4 — World gen load screen

- **Binding:** `world_gen_load_screen` → `world_gen_load_screen` → `res://godot/ui/menus/worldGenLoadScreen.tscn`
- **When:** User presses **Generate** on the menu (`WorldGenMenu.gd`).
- **Data:**
  - Form dict from `WorldGenCardFormBinder` (target: OW-3.4) → async world gen on worker thread; poll on UI thread (SA-4.1).
  - C++: parse form → `WorldGenRunner` + `PlanetGlobePipeline` (`default_globe` or `deterministic_globe` from form).
  - On success: seed + genome + **`PlanetSurfaceAtlas`** in bridge (OW-E1/E2). Marshal dict for preview only.
- **Screen:** “Generating world…” → log → **Continue**.

### 5 — Planet preview (post–world gen)

- **Binding:** `planet_preview` → `sphere_preview` → `res://godot/debug/SpherePreview.tscn`
- **When:** User presses **Continue** on load screen; load screen freed; world gen menu removed.
- **Data:** `WorldGenPreviewApplier.apply` pushes result into preview. Clears any prior `SpherePreview` sibling.
- **Screen:** Unit-sphere debug meshes, orbit camera. `ClearWorldTerrain()` removes any `WorldRoot` children; **target** `WorldViewManager.reset_streaming()` (GW-7.0c).

### 6 — Preview overlay (nested)

- **Registry:** `sphere_preview_overlay` → `res://godot/ui/panels/SpherePreviewOverlay.tscn`
- **When:** `SpherePreview._Ready` → `AddOverlay()`.
- **Data:** Toggle visibility on preview layers.
- **Screen:** Layer control panel (left); **Start** anchored bottom-right.

### 5b — Start → game world

- **When:** User presses **Start** on `SpherePreviewOverlay`.
- **Data:**
  - `SimAPI.commit_overworld_for_play()` (session flag; idempotent).
  - `SimAPI.enter_game_world(main)` → removes `SpherePreview`, `WorldViewManager.reset_streaming()`, initial `request_chunks` at focus.
- **Screen:** Preview and overlay gone; orbit camera on `Main` remains; chunks stream under `WorldRoot` (step 7).

### 7 — Chunk streaming (parallel lifecycle, suppressed in preview)

- **Registry:** `chunk_node` → `res://godot/world/ChunkNode.tscn`
- **When:** Every `Main._process` while `SpherePreview` is **absent** and `WorldViewManager.update_streaming(focus)` runs.
- **Data:** `request_chunks` around focus; `poll_diffs` → chunk diffs.
- **Screen:** Instanced terrain chunks under `WorldRoot`. **Not updated while step 5 is active**.

---

## Scene tree after world gen (typical)

```
Main (Main.tscn)
├── WorldRoot
├── Focus
│   ├── Camera3D
│   └── DirectionalLight3D
├── WorldEnvironment
├── WorldViewManager (script)
├── SpherePreview
│   ├── … 3D debug layers …
│   └── OverlayLayer
│       └── SpherePreviewOverlay
└── (UILayer may be empty after menu removed)
```

---

## Registered but not in default boot path

| Scene | Notes |
|-------|--------|
| `ui/menus/mainMenu.tscn` | Legacy / unused; entry is world gen menu |
| `ui/hud.tscn`, `ui/menus/pauseMenu.tscn`, `ui/menus/settingsMenu.tscn` | Gameplay UI — not wired |

---

## Related docs

- [Critical issues audit](critical_bugs_audit.md)
- [Moddability architecture](moddability.md)
- [Modern procedural planet generation](worldgen_modern.md)
- [Epic: Sim API & platform](epic_sim_api_platform.md)
- [Epic: Overworld generation](epic_overworld_generation.md)
- [Epic: Sim correctness & QA](epic_sim_correctness.md)
- [Epic: Game world streaming](epic_game_world_streaming.md)
