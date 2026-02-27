# Growth Sim GDExtension

The config file is named `growth_sim.gdextension.disabled` by default so Godot does not try to load the missing DLL. After building the extension, rename it to `growth_sim.gdextension` and put the DLL in `gde/bin/`.

C++ sim contract: `boot`, `step`, `request_chunks`, `poll_diffs`, `apply_intent`.  
The Godot autoload **SimAPI** (GDScript) is the single door; it uses the C++ **GrowthSim** class when the extension is built.

**ECS (sim_core):** Encapsulated storage: `EntityId`, `Registry` (create/destroy, has/get/emplace/remove, each&lt;A,B,C&gt;(f)), `IComponentStorage` + `ComponentStorage&lt;T&gt;` (sparse set + SoA). Swap backend later by replacing the implementation behind the interface. Entry: `#include "ecs/ECS.hpp"`.

**Util (sim_core):** Encapsulated helpers: `util/Memory` (allocate/deallocate, Buffer), `util/Random` (deterministic RNG + `random_seed()` for empty seed field), `util/String` (hash, mix_u64). Entry: `#include "util/Util.hpp"`.

**World seed:** `boot(defdb_path, form_dict)` takes an optional form dictionary. If provided, string keys with integer/float values are passed to the seed generator to produce a deterministic `WorldSeed` (value, height_scale, octaves, frequency). Known keys: `"seed"`, `"height_scale"`, `"octaves"`, `"frequency"`. Omit `form_dict` or pass `{}` to use default seed 0.

**Planet genome (gen/):** Stored canonical state: `PlanetGenome` (seed, schema_version, L_star, a, e, S_eff, M_p, R_p, P_rot, obliquity, p0, albedo, greenhouse, water_fraction, precipitation, material_tags bitfield). Phenotype derived via `Planet` (g, P_orb, T_eq, T_surf_estimate, scale_height, S_eff, precipitation, material_tags). **Blueprint layer:** `PlanetGenBlueprint` holds DistSpecs (Constant/Uniform/LogUniform/Normal/Beta), `HabitabilityConstraintsSpec`, policies (RadiusMode, StarMode), `PlanetStreamIds`. All settable. `set_blueprint_to_preset(blueprint, "Earthlike"|"OceanWorld"|"Dry")` or `set_blueprint_to_random(blueprint, rng)`; `PlanetGenerator::generate_from_blueprint(blueprint)` samples and applies constraints. Presets documented in `data/core/defs/planet_presets.xml` (schema: `schemas/planet_presets.xsd`). Compile `sim_core/src/gen/PlanetGenerator.cpp` and `sim_core/src/gen/PlanetBlueprint.cpp`.

## Build (Windows, 4.x)

**Prerequisite:** SCons is required to build godot-cpp. Install it with Python:

```bash
pip install scons
```

1. **Get godot-cpp** (same major as your Godot):

   ```bash
   cd gde
   git clone -b 4.x https://github.com/godotengine/godot-cpp godot-cpp
   cd godot-cpp
   git submodule update --init
   ```

2. **Build godot-cpp** (from `gde/godot-cpp`):  
   Use `py -m SCons` if `scons` is not on PATH (common on Windows after `pip install scons`):

   ```bash
   py -m SCons platform=windows target=template_debug
   py -m SCons platform=windows target=template_release
   ```

3. **Build growth_sim** (from `gde`):  
   **Easiest:** run the build script so the DLL is built and copied to the name Godot expects:
   ```powershell
   cd gde
   .\build.ps1
   ```
   This runs SCons for `template_debug` and copies `bin/growth_sim.dll` to `bin/growth_sim.windows.template_debug.x86_64.dll`. Use `.\build.ps1 -Release` for release, or `.\build.ps1 -All` for both.  
   **Manual:** use SCons directly; the extension must produce the DLL in `gde/bin/` with the name matching `growth_sim.gdextension` (e.g. `growth_sim.windows.template_debug.x86_64.dll`).

4. **Run the game**: ensure `res://gde/growth_sim.gdextension` and the DLL in `res://gde/bin/` are present.  
   If the extension is not built, SimAPI falls back to the GDScript stub.

## Updating the GDExtension

When you change C++ or GDExtension code:

1. **Rebuild** from the `gde/` directory. Easiest: run `.\build.ps1` (Windows). It builds and copies the DLL to the name Godot expects. For release: `.\build.ps1 -Release`; for both: `.\build.ps1 -All`.  
   Manual: `py -m SCons platform=windows target=template_debug`, then copy `bin/growth_sim.dll` to `bin/growth_sim.windows.template_debug.x86_64.dll`.

2. **No need to “reinstall”** — the DLL is loaded from `gde/bin/` when Godot runs. Restart the game (or run again from the editor) to pick up the new binary.

3. **If you add new source files:**
   - New `.cpp` under `gde/src/` or `gde/sim_core/src/` must be added to the `sources` list in **gde/SConstruct**, or they won’t be compiled or linked.
   - After editing SConstruct, run the same SCons command from `gde/` again.

4. **If Godot reports the DLL missing:** ensure `growth_sim.gdextension` exists (not only `.disabled`) and that `gde/bin/growth_sim.windows.template_debug.x86_64.dll` (or the matching name for your platform/target) exists. To run without the extension, rename to `growth_sim.gdextension.disabled` so Godot uses the stub and doesn’t try to load the DLL.

## Diffs (contract)

- **ChunkLoaded**: `{ type: "ChunkLoaded", coord: Vector2i, height_samples: Array (17*17) }`
- **ChunkUnloaded**: `{ type: "ChunkUnloaded", coord: Vector2i }`
- **CellChanged**: `{ type: "CellChanged", chunk_coord, local_xz, layer, new_value }`
