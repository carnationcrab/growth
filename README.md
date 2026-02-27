# Growth

Godot 4 project with an optional C++ GDExtension backend for the sim. You can run the game with or without building the extension.

## Running the project

1. **Open in Godot 4.x**  
   Open the project folder in the Godot editor (use **Project → Import** and select this directory, or open `project.godot` if present).

2. **Run the main scene**  
   Set the main scene to the one that boots the sim (e.g. the scene that uses `Main.gd`), then press **Play** or **F5**.  
   The world gen menu appears first; use **Generate** to apply settings and continue.

3. **Stub vs C++ backend**
   - **Without the extension:** If `gde/growth_sim.gdextension` is missing or disabled (e.g. you use `growth_sim.gdextension.disabled`), Godot does not load the C++ DLL. The **SimAPI** autoload uses a stub: the game runs but the sim is a no-op (no real terrain, planet gen, etc.). You’ll see `[SimAPI] GrowthSim not found; using stub backend` in the output.
   - **With the extension:** After building the GDExtension and enabling it (see **gde/README.md**), the DLL is loaded and SimAPI uses the C++ **GrowthSim** class. You’ll see `[SimAPI] using C++ GrowthSim backend`.

So: you can always run the project; building the extension is only needed for the real sim backend.

## When you change the GDExtension

Code for the extension lives under **gde/** (C++ and GDExtension glue). For a concise build and update workflow, see **gde/README.md**. In short:

- **Build the extension** from `gde/`: run **`gde/build.ps1`** (Windows) to build and copy the DLL to the name Godot expects; see **gde/README.md** for prerequisites (godot-cpp, SCons).
- **Enable it** by using `growth_sim.gdextension` (and having the DLL in `gde/bin/`); disable by renaming to `growth_sim.gdextension.disabled` if you want to run without the DLL.
- **After C++ changes:** run `.\build.ps1` again; no need to restart Godot (the new DLL is loaded on next run).
- **If you add new `.cpp` files** under `gde/sim_core/src/` or `gde/src/`, add them to the `sources` list in **gde/SConstruct** so they are compiled and linked.

Details, prerequisites, and platform notes: **gde/README.md**.
