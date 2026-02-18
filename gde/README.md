# Growth Sim GDExtension

The config file is named `growth_sim.gdextension.disabled` by default so Godot does not try to load the missing DLL. After building the extension, rename it to `growth_sim.gdextension` and put the DLL in `gde/bin/`.

C++ sim contract: `boot`, `step`, `request_chunks`, `poll_diffs`, `apply_intent`.  
The Godot autoload **SimAPI** (GDScript) is the single door; it uses the C++ **GrowthSim** class when the extension is built.

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
   Use the same build system as godot-cpp (SCons) or your own. The extension must:
   - Include `sim_core/include` and `godot-cpp/include`, `src/`
   - Compile `src/register_types.cpp`, `src/sim_api_gdextension.cpp`, and all `sim_core/src` .cpp files
   - Link godot-cpp and produce `growth_sim.windows.template_debug.x86_64.dll` (and release) in `gde/bin/`

4. **Run the game**: ensure `res://gde/growth_sim.gdextension` and the DLL in `res://gde/bin/` are present.  
   If the extension is not built, SimAPI falls back to the GDScript stub.

## Diffs (contract)

- **ChunkLoaded**: `{ type: "ChunkLoaded", coord: Vector2i, height_samples: Array (17*17) }`
- **ChunkUnloaded**: `{ type: "ChunkUnloaded", coord: Vector2i }`
- **CellChanged**: `{ type: "CellChanged", chunk_coord, local_xz, layer, new_value }`
