# Build growth_sim GDExtension and copy DLL(s) to the names Godot expects.
# Run from gde/: .\build.ps1   or   .\build.ps1 -Release   or   .\build.ps1 -All
# Requires: godot-cpp built first (see README.md), and: pip install scons

param(
    [switch]$Release,
    [switch]$All
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

$usePy = Get-Command py -ErrorAction SilentlyContinue
function Build-Target {
    param([string]$Target)
    $name = "growth_sim.windows.$Target.x86_64.dll"
    Write-Host "Building $Target..."
    if ($usePy) {
        py -m SCons platform=windows "target=$Target"
    } else {
        scons platform=windows "target=$Target"
    }
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    $dll = "bin\growth_sim.dll"
    if (Test-Path $dll) {
        Copy-Item $dll "bin\$name" -Force
        Write-Host "Copied to bin\$name"
    }
}

if ($All) {
    Build-Target "template_debug"
    Build-Target "template_release"
} elseif ($Release) {
    Build-Target "template_release"
} else {
    Build-Target "template_debug"
}

Write-Host "Done."
