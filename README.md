# Growth

Godot **4.6** game with a **C#** front end and an optional **C++ GDExtension** sim backend (terrain, overworld generation, chunk streaming). You can run the editor with a stub sim only, or build the native extension for the full simulation.

## New here?

| Step | Action |
|------|--------|
| 1 | **[Install guide](docs/install.md)** — Godot, .NET, Python, MSVC, godot-cpp |
| 2 | **Windows onboarding** — `.\scripts\setup.ps1` (installs, builds, can launch Godot) |
| 3 | **Linux / macOS** — `./scripts/setup.sh` (native build; install Godot manually) |
| 4 | **Docs index** — [docs/README.md](docs/README.md) |

### Setup script (Windows)

```powershell
.\scripts\setup.ps1              # winget deps (optional), pip, godot-cpp, build.ps1, launch Godot
.\scripts\setup.ps1 -Quick       # checklist + open install links only
.\scripts\setup.ps1 -SkipWinget   # skip package manager
.\scripts\setup.ps1 -SkipBuild    # prerequisites only
.\scripts\setup.ps1 -DownloadGodot  # fetch portable Godot 4.6 .NET into .local/godot/
```

Requires PowerShell 5.1+ (default on Windows 10/11). See [docs/install.md](docs/install.md) for manual steps and troubleshooting.

## Running the project

1. **Open in Godot 4.6 (.NET)**  
   Import this folder or open `project.godot`. Use the **.NET / Mono** editor build, not the standard build without C# support.

2. **Run** — main scene `res://godot/main/Main.tscn` (**F5**). The world gen menu appears first; use **Generate** to continue.

3. **Stub vs C++ backend**
   - **Without the extension:** If `gde/growth_sim.gdextension` is missing or disabled, **SimAPI** uses a stub (`[SimAPI] GrowthSim not found; using stub backend`). UI and flow work; sim logic is a no-op.
   - **With the extension:** After building (below), you should see `[SimAPI] using C++ GrowthSim backend`.

## Native extension (C++)

Code lives under **`gde/`**. Full build instructions: **[gde/README.md](gde/README.md)**.

```powershell
cd gde
.\build.ps1    # after godot-cpp is built; see docs/install.md
```

- Enable **`gde/growth_sim.gdextension`** and place the DLL in **`gde/bin/`**.
- New `.cpp` files under `gde/sim_core/src/` or `gde/src/` must be listed in **`gde/SConstruct`**.

## Development

| Task | Command |
|------|---------|
| Moddability lint | `python tools/lint_moddability.py` |
| SConstruct sources | `python tools/lint_sconstruct_sources.py` |
| Pre-commit hooks | `pip install pre-commit && pre-commit install` |
| New C++ / templates | [templates/README.md](templates/README.md) |

Architecture and content packs: **[docs/moddability.md](docs/moddability.md)**.

## Repository layout

| Path | Purpose |
|------|---------|
| `godot/` | Scenes, C# UI, autoload **SimAPI** |
| `gde/` | GDExtension + **sim_core** (C++) |
| `data/` | Def packs, manifests, game profile |
| `docs/` | Design docs, epics, install guide |
| `tools/` | Linters, templates, bench gate |
| `scripts/` | Onboarding (`setup.ps1`, `setup.sh`) |
