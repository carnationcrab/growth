# Growth documentation

## Getting started

| Doc | Purpose |
|-----|---------|
| **[install.md](install.md)** | Install Godot, .NET, toolchain, godot-cpp, GDExtension |
| [../README.md](../README.md) | Project overview and `scripts/setup.ps1` |
| [../gde/README.md](../gde/README.md) | Build and update the C++ extension |

---

## Terminology

| Term | Meaning |
|------|---------|
| **Overworld** | Generated macro planet: whole-sphere region atlas (plates, elevation, moisture). Built once per session; read-only baseline. |
| **Game world** | Generated micro simulation around the player: streamed chunks, metre-scale detail, player-editable environment. |
| **Planet preview** | Debug Godot scene (`SpherePreview`) for inspecting overworld data—not the game world. |

---

| Doc | Purpose |
|-----|---------|
| [moddability.md](moddability.md) | Content packs, defs, SimAPI rules, linter |
| [game_scene_order.md](game_scene_order.md) | Boot flow, scenes, on-screen steps (living) |
| [critical_bugs_audit.md](critical_bugs_audit.md) | Top implementation gaps vs epics (living audit) |
| [cool_bugs/](cool_bugs/README.md) | Memorable visual/wiring bugs with screenshots and post-mortems |
| [worldgen.md](worldgen.md) | Form → seed → genome pipeline (legacy overview) |
| [worldgen_modern.md](worldgen_modern.md) | Overworld algorithm stages reference (sim-side) |

## Epics

Product and engineering backlogs. User stories are grouped by **scene** (from `game_profile.xml` / `scene_registry.xml`) or **engineering theme**.

| Epic | Scope | Story prefix |
|------|--------|--------------|
| **[epic_sim_api_platform.md](epic_sim_api_platform.md)** | SimAPI, GrowthSim, boot/data root, async world gen, profile paths, form binder | SA- |
| **[epic_overworld_generation.md](epic_overworld_generation.md)** | Overworld pipeline, atlas, location queries, world gen UI scenes | OW- |
| **[epic_sim_correctness.md](epic_sim_correctness.md)** | Topology validation, bench CI, mesh winding, determinism regression | SC- |
| **[epic_game_world_streaming.md](epic_game_world_streaming.md)** | Game world chunk streaming, local proc gen from overworld, player edits | GW- |

### Suggested cross-epic order

1. **SA P0–P1** — merged boot, async load screen, profile scene paths.  
2. **SC Q0–Q1** + **OW-E18** — topology gates before hydrology work.  
3. **OW G1–G2** — atlas in bridge, `sample_surface`, preview from atlas.  
4. **GW S0b–S3** — preview transition, overworld-shaped chunks, IntentRouter dig.  
5. **OW G3+** / **GW S4+** — climate, persistence, layers.

When you change scene flow, update **game_scene_order.md** and the relevant epic if acceptance criteria shift. When a known gap is fixed, update **[critical_bugs_audit.md](critical_bugs_audit.md)**.
