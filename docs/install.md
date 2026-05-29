# Installing Growth

This guide covers everything you need to clone, build, and run **Growth** on your machine. The project targets **Godot 4.6** with **C#** and an optional **C++ GDExtension** sim backend.

For day-to-day commands after setup, see the root [README.md](../README.md) and [gde/README.md](../gde/README.md).

---

## Quick start (Windows)

From the repository root in PowerShell:

```powershell
.\scripts\setup.ps1
```

That script checks prerequisites, can install common tools via `winget`, fetches **godot-cpp**, builds the native extension, and optionally launches Godot against this project. Use `-Quick` for a checklist and browser links only; run `Get-Help .\scripts\setup.ps1 -Detailed` for all switches.

On Linux or macOS, use [scripts/setup.sh](../scripts/setup.sh) for the native build path (Godot install remains manual).

---

## What you need (by goal)

| Goal | Required | Optional |
|------|----------|----------|
| **Open the project in the editor** | Godot **4.6** .NET build, **.NET SDK 8** | — |
| **Run the game (UI flow, stub sim)** | Same as above | C++ extension not required |
| **Full sim (terrain, world gen, C++)** | Above + **Git**, **Python 3**, **SCons**, **C++ toolchain**, **godot-cpp** | Visual Studio 2022 Build Tools (Windows) |
| **Contribute (lint / hooks)** | **Python 3**, `pip install -r tools/requirements-lint.txt` | `pre-commit install` |

The game runs without the GDExtension: **SimAPI** falls back to a stub and logs `[SimAPI] GrowthSim not found; using stub backend`. For real overworld generation and sim behaviour, build and enable the extension (see [Native extension (GDExtension)](#native-extension-gdextension)).

---

## Godot 4.6 (.NET / C#)

Growth uses C# gameplay scripts (`config/features` includes `"C#"`, `growth.csproj` targets **net8.0**).

1. Download the **.NET** editor for your OS (not the plain “standard” build unless you only want the stub path without C#):
   - [Godot 4.6 stable downloads](https://godotengine.org/download/archive/4.6-stable/) — choose **Windows – .NET – x86_64** (or Linux/macOS .NET variant).
   - Direct Windows x64: [Godot_v4.6-stable_mono_win64.zip](https://github.com/godotengine/godot/releases/download/4.6-stable/Godot_v4.6-stable_mono_win64.zip)

2. Extract or install the editor. On Windows, `winget` can install the Mono editor:
   ```powershell
   winget install -e --id GodotEngine.GodotEngine.Mono
   ```
   Pin a 4.6.x version if winget offers a choice; **4.6.x** is compatible with this repo.

3. Open the project:
   - **Project → Import** and select this repository folder, or
   - Run Godot with the project path, e.g. `Godot_v4.6-stable_mono_win64.exe --path C:\path\to\growth`

4. First open may download / restore .NET dependencies for `growth.csproj`. Ensure the **.NET SDK 8** is installed:
   ```powershell
   winget install -e --id Microsoft.DotNet.SDK.8
   ```

5. Press **F5** or **Play**. Main scene: `res://godot/main/Main.tscn`.

---

## .NET SDK

| Item | Value |
|------|--------|
| SDK | **8.x** (`net8.0` in `growth.csproj`) |
| Godot package | `Godot.NET.Sdk/4.6.0` |

Verify:

```bash
dotnet --version
```

---

## Git

Required to clone **godot-cpp** and the repo itself.

- Windows: [https://git-scm.com/download/win](https://git-scm.com/download/win) or `winget install -e --id Git.Git`
- Linux: `sudo apt install git` / distro equivalent
- macOS: Xcode Command Line Tools or Homebrew `git`

---

## Python and SCons

Used to run **SCons** (godot-cpp and `growth_sim` builds) and repo linters.

1. Install **Python 3.10+** ([python.org](https://www.python.org/downloads/) or `winget install -e --id Python.Python.3.12`).
2. On Windows, prefer the **py** launcher or ensure `python` is on `PATH`.
3. Install Python packages from the repo root:

   ```bash
   pip install scons
   pip install -r tools/requirements-lint.txt
   ```

   Optional hooks:

   ```bash
   pip install pre-commit
   pre-commit install
   ```

---

## C++ toolchain

### Windows

Install **Visual Studio 2022 Build Tools** with the **Desktop development with C++** workload (MSVC v143, Windows SDK).

- Installer: [Visual Studio Build Tools](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022)
- Or winget (may take a while):
  ```powershell
  winget install -e --id Microsoft.VisualStudio.2022.BuildTools
  ```

Open a **“x64 Native Tools Command Prompt”** or **Developer PowerShell** if a plain shell cannot find `cl.exe`. The setup script uses normal PowerShell; if SCons fails with “compiler not found”, run builds from Developer PowerShell.

### Linux

```bash
sudo apt install build-essential scons pkg-config libx11-dev libxcursor-dev libxinerama-dev libgl1-mesa-dev libxi-dev libxrandr-dev
```

### macOS

Xcode Command Line Tools: `xcode-select --install`

---

## Native extension (GDExtension)

Detailed API and rebuild notes: [gde/README.md](../gde/README.md).

### 1. Clone godot-cpp

From `gde/` (branch **4.x**, same major as Godot 4):

```bash
cd gde
git clone -b 4.x https://github.com/godotengine/godot-cpp godot-cpp
cd godot-cpp
git submodule update --init
```

### 2. Build godot-cpp

From `gde/godot-cpp`:

```bash
# Windows (debug — matches editor template_debug)
py -m SCons platform=windows target=template_debug
py -m SCons platform=windows target=template_release

# Linux
scons platform=linux target=template_debug
scons platform=linux target=template_release

# macOS
scons platform=macos target=template_debug
scons platform=macos target=template_release
```

### 3. Build growth_sim

**Windows (recommended):**

```powershell
cd gde
.\build.ps1
```

**Manual (any platform):** from `gde/`:

```bash
scons platform=windows target=template_debug
```

Godot expects a platform-specific name under `gde/bin/` (e.g. `growth_sim.windows.template_debug.x86_64.dll`). `build.ps1` copies `bin/growth_sim.dll` to that name on Windows.

### 4. Enable the extension

- Config: `gde/growth_sim.gdextension` (shipped enabled when the DLL exists).
- If you use `growth_sim.gdextension.disabled`, rename it to `growth_sim.gdextension` after a successful build.
- DLL must live in `gde/bin/` with the name declared in the `.gdextension` file.

Close Godot before rebuilding if the DLL is locked.

### 5. Verify in the editor

Run the game and check the output console:

- **C++ backend:** `[SimAPI] using C++ GrowthSim backend`
- **Stub:** `[SimAPI] GrowthSim not found; using stub backend`

---

## Optional: worldgen bench gate

Developers validating sim mesh quality:

```bash
python tools/run_worldgen_bench_gate.py
```

This builds `gde/bin/worldgen_bench` via SCons if missing, then runs the gate preset. Also available as a manual pre-commit hook: `pre-commit run worldgen-bench-sc-e2 --all-files`.

---

## Troubleshooting

| Symptom | What to try |
|---------|-------------|
| Godot cannot load C# / missing SDK | Install .NET SDK 8; reopen project; build solution once in editor |
| `GrowthSim not found` | Build extension; ensure `growth_sim.gdextension` and `gde/bin/*.dll` exist |
| SCons / `cl` not found (Windows) | Install VS Build Tools; use Developer PowerShell |
| `godot-cpp not found` | Clone into `gde/godot-cpp` and run `git submodule update --init` |
| DLL copy failed on build | Close Godot and any running game; rerun `gde/build.ps1` |
| Wrong Godot major | Use **4.6** .NET build; rebuild godot-cpp on 4.x branch |

---

## Links

| Resource | URL |
|----------|-----|
| Godot 4.6 downloads | https://godotengine.org/download/archive/4.6-stable/ |
| godot-cpp | https://github.com/godotengine/godot-cpp |
| .NET SDK 8 | https://dotnet.microsoft.com/download/dotnet/8.0 |
| Project docs index | [docs/README.md](README.md) |
| Moddability / lint | [moddability.md](moddability.md) |
