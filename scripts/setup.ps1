# Growth onboarding - install prerequisites, build native code, launch Godot.
# Run from repo root:  .\scripts\setup.ps1
# Quick checklist + links only:  .\scripts\setup.ps1 -Quick
<#
.SYNOPSIS
  One-shot Growth developer setup (Windows).

.DESCRIPTION
  Checks prerequisites, optionally installs tools via winget, installs Python
  packages, clones godot-cpp, builds the GDExtension, and launches Godot.

.PARAMETER Quick
  Print the checklist and useful links only; no installs or builds.

.PARAMETER SkipWinget
  Do not invoke winget package installs.

.PARAMETER SkipBuild
  Install Python packages and clone godot-cpp only.

.PARAMETER SkipLaunch
  Do not start Godot after setup.

.PARAMETER DownloadGodot
  Download portable Godot 4.6 .NET into .local/godot/ if not found.
#>

[CmdletBinding()]
param(
    [switch]$Quick,
    [switch]$SkipWinget,
    [switch]$SkipBuild,
    [switch]$SkipLaunch,
    [switch]$DownloadGodot,
    [string]$GodotVersion = "4.6-stable"
)

$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$GdeRoot = Join-Path $RepoRoot "gde"
$GodotCppDir = Join-Path $GdeRoot "godot-cpp"
$LocalGodotDir = Join-Path $RepoRoot ".local\godot"
$GodotZipName = "Godot_v${GodotVersion}_mono_win64.zip"
$GodotExeName = "Godot_v${GodotVersion}_mono_win64.exe"
$GodotDownloadUrl = "https://github.com/godotengine/godot/releases/download/$GodotVersion/$GodotZipName"

$Links = [ordered]@{
    "Install guide (this repo)"        = "file:///$($RepoRoot -replace '\\','/')/docs/install.md"
    "Godot 4.6 .NET downloads"       = "https://godotengine.org/download/archive/4.6-stable/"
    "godot-cpp repository"             = "https://github.com/godotengine/godot-cpp"
    ".NET SDK 8"                       = "https://dotnet.microsoft.com/download/dotnet/8.0"
    "Visual Studio Build Tools"        = "https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022"
    "Python downloads"                 = "https://www.python.org/downloads/"
}

function Write-Header([string]$Text) {
    Write-Host ""
    Write-Host "=== $Text ===" -ForegroundColor Cyan
}

function Test-RepoRoot {
    if (-not (Test-Path (Join-Path $RepoRoot "project.godot"))) {
        throw "Run this script from the Growth repository (expected project.godot at $RepoRoot)."
    }
}

function Test-Command([string]$Name) {
    $null -ne (Get-Command $Name -ErrorAction SilentlyContinue)
}

function Invoke-Checked {
    param([scriptblock]$Block, [string]$FailureMessage)
    & $Block
    if ($LASTEXITCODE -ne 0) { throw $FailureMessage }
}

function Open-Links {
    Write-Header "Useful links"
    foreach ($entry in $Links.GetEnumerator()) {
        Write-Host ("  {0,-34} {1}" -f $entry.Key, $entry.Value)
    }
    if ($Quick -and [Environment]::UserInteractive) {
        Write-Host ""
        $open = Read-Host "Open these links in your browser now? [y/N]"
        if ($open -match '^[yY]') {
            foreach ($url in $Links.Values) {
                if ($url -notlike "file://*") {
                    Start-Process $url
                }
            }
            Start-Process (Join-Path $RepoRoot "docs\install.md")
        }
    }
}

function Install-WingetPackage {
    param(
        [string]$Id,
        [string]$Label,
        [string[]]$ExtraArgs = @()
    )
    if ($SkipWinget) { return }
    if (-not (Test-Command "winget")) {
        Write-Host "winget not found - install $Label manually." -ForegroundColor Yellow
        return
    }
    Write-Host "Installing $Label ($Id) via winget..."
    $args = @("install", "-e", "--id", $Id, "--accept-package-agreements", "--accept-source-agreements") + $ExtraArgs
    & winget @args
    if ($LASTEXITCODE -gt 1) {
        Write-Host "winget exit code $LASTEXITCODE for $Id (may already be installed)." -ForegroundColor Yellow
    }
}

function Ensure-PythonPackages {
    Write-Header "Python packages"
    $pip = if (Test-Command "py") { @("py", "-m", "pip") } elseif (Test-Command "python") { @("python", "-m", "pip") } else {
        throw "Python not found. Install Python 3.10+ and rerun."
    }
    Invoke-Checked { & @pip @("install", "--upgrade", "pip") } "pip upgrade failed"
    Invoke-Checked { & @pip @("install", "scons") } "scons install failed"
    $req = Join-Path $RepoRoot "tools\requirements-lint.txt"
    if (Test-Path $req) {
        Invoke-Checked { & @pip @("install", "-r", $req) } "lint requirements install failed"
    }
    Write-Host "Optional: pip install pre-commit; pre-commit install" -ForegroundColor DarkGray
}

function Ensure-GodotCpp {
    Write-Header "godot-cpp"
    if (-not (Test-Path $GodotCppDir)) {
        if (-not (Test-Command "git")) { throw "git not found - install Git and rerun." }
        Push-Location $GdeRoot
        try {
            Write-Host "Cloning godot-cpp (branch 4.x)..."
            git clone -b 4.x https://github.com/godotengine/godot-cpp godot-cpp
            if ($LASTEXITCODE -ne 0) { throw "git clone failed" }
            Push-Location godot-cpp
            try {
                git submodule update --init
                if ($LASTEXITCODE -ne 0) { throw "git submodule update failed" }
            } finally {
                Pop-Location
            }
        } finally {
            Pop-Location
        }
    } else {
        Write-Host "godot-cpp already present at $GodotCppDir"
    }
}

function Invoke-SCons {
    param([string[]]$SconsArgs)
    if (Test-Command "py") {
        & py -m SCons @SconsArgs
    } elseif (Test-Command "scons") {
        & scons @SconsArgs
    } else {
        throw "Neither 'py -m SCons' nor 'scons' is available. Run: pip install scons"
    }
}

function Build-Native {
    if ($SkipBuild) { return }
    Write-Header "Building godot-cpp (template_debug)"
    if (-not (Test-Path (Join-Path $GodotCppDir "SConstruct"))) {
        throw "godot-cpp missing. Rerun without -SkipBuild or clone manually (see docs/install.md)."
    }
    Push-Location $GodotCppDir
    try {
        Invoke-SCons @("platform=windows", "target=template_debug")
        if ($LASTEXITCODE -ne 0) { throw "godot-cpp build failed. Use a VS Developer shell if MSVC is missing." }
    } finally {
        Pop-Location
    }

    Write-Header "Building growth_sim"
    $buildScript = Join-Path $GdeRoot "build.ps1"
    if (-not (Test-Path $buildScript)) { throw "Missing $buildScript" }
    & $buildScript
    if ($LASTEXITCODE -ne 0) { throw "growth_sim build failed." }

    $extDisabled = Join-Path $GdeRoot "growth_sim.gdextension.disabled"
    $extEnabled = Join-Path $GdeRoot "growth_sim.gdextension"
    if ((Test-Path $extDisabled) -and -not (Test-Path $extEnabled)) {
        Rename-Item $extDisabled $extEnabled
        Write-Host "Enabled growth_sim.gdextension"
    }
}

function Get-GodotExecutable {
    if (Test-Path (Join-Path $LocalGodotDir $GodotExeName)) {
        return (Join-Path $LocalGodotDir $GodotExeName)
    }
    $candidates = @(
        (Join-Path $env:LOCALAPPDATA "Programs\Godot\Godot_mono_win64.exe"),
        (Join-Path $env:LOCALAPPDATA "Programs\Godot\Godot.exe")
    )
    foreach ($path in $candidates) {
        if (Test-Path $path) { return $path }
    }
    $cmd = Get-Command "godot*" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($cmd) { return $cmd.Source }
    return $null
}

function Ensure-PortableGodot {
  if (-not $DownloadGodot) { return $null }
  $exe = Join-Path $LocalGodotDir $GodotExeName
  if (Test-Path $exe) { return $exe }
  Write-Header "Downloading portable Godot ($GodotVersion .NET)"
  New-Item -ItemType Directory -Force -Path $LocalGodotDir | Out-Null
  $zipPath = Join-Path $LocalGodotDir $GodotZipName
  Write-Host "From $GodotDownloadUrl"
  Invoke-WebRequest -Uri $GodotDownloadUrl -OutFile $zipPath
  Expand-Archive -Path $zipPath -DestinationPath $LocalGodotDir -Force
  if (Test-Path $exe) { return $exe }
  $found = Get-ChildItem -Path $LocalGodotDir -Filter "Godot*.exe" -Recurse | Select-Object -First 1
  if ($found) { return $found.FullName }
  throw "Godot executable not found after extract."
}

function Launch-Godot {
    if ($SkipLaunch) { return }
    $godot = Get-GodotExecutable
    if (-not $godot) { $godot = Ensure-PortableGodot }
    if (-not $godot) {
        Write-Host "Godot not found. Install Godot 4.6 .NET or rerun with -DownloadGodot." -ForegroundColor Yellow
        return
    }
    Write-Header "Launching Godot"
    Write-Host $godot
    Start-Process -FilePath $godot -ArgumentList @("--path", $RepoRoot)
}

function Show-PrerequisiteReport {
    Write-Header "Prerequisite check"
    $rows = @(
        @{ Name = "git";       Required = "Clone godot-cpp" },
        @{ Name = "py";        Required = "Python / SCons (or python)" },
        @{ Name = "dotnet";    Required = "C# / Godot .NET" },
        @{ Name = "winget";    Required = "Optional auto-install" }
    )
    foreach ($row in $rows) {
        $ok = Test-Command $row.Name
        if ($row.Name -eq "py" -and -not $ok) { $ok = Test-Command "python" }
        $status = if ($ok) { "OK" } else { "MISSING" }
        $colour = if ($ok) { "Green" } else { "Yellow" }
        Write-Host ("  [{0}] {1,-8} - {2}" -f $status, $row.Name, $row.Required) -ForegroundColor $colour
    }
    if (-not (Test-Command "cl")) {
        Write-Host "  [??] cl       - MSVC (install VS 2022 Build Tools with C++ workload)" -ForegroundColor Yellow
    } else {
        Write-Host "  [OK] cl       - MSVC compiler on PATH" -ForegroundColor Green
    }
}

# --- main ---
Write-Host @"

  Growth setup
  Repo: $RepoRoot

"@ -ForegroundColor White

Test-RepoRoot
Show-PrerequisiteReport
Open-Links

if ($Quick) {
    Write-Host ""
    Write-Host "Quick mode: no installs or builds. Run without -Quick when ready." -ForegroundColor Cyan
    exit 0
}

if (-not $SkipWinget) {
    Write-Header "Optional winget installs"
    Write-Host "Skipping any package that is already installed. Cancel with Ctrl+C if you prefer manual installs."
    Install-WingetPackage "Git.Git" "Git"
    Install-WingetPackage "Python.Python.3.12" "Python 3.12"
    Install-WingetPackage "Microsoft.DotNet.SDK.8" ".NET SDK 8"
    Install-WingetPackage "GodotEngine.GodotEngine.Mono" "Godot Engine (Mono)"
    Install-WingetPackage "Microsoft.VisualStudio.2022.BuildTools" "VS 2022 Build Tools" @(
        "--override", "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
    )
}

Ensure-PythonPackages
Ensure-GodotCpp
Build-Native
Launch-Godot

Write-Header "Done"
Write-Host "  Next steps:"
Write-Host "    1. Open docs/install.md for platform-specific notes."
Write-Host "    2. In Godot, press F5 to run (main scene: godot/main/Main.tscn)."
Write-Host "    3. After C++ changes:  cd gde; .\build.ps1"
Write-Host "    4. Lint (optional):  python tools/lint_moddability.py"
Write-Host ""
