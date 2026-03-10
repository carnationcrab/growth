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
        $dest = "bin\$name"
        $copied = $false
        foreach ($attempt in 1..3) {
            try {
                Copy-Item $dll $dest -Force -ErrorAction Stop
                $copied = $true
                break
            } catch [System.IO.IOException] {
                if ($attempt -lt 3) {
                    Write-Host "DLL in use, retrying in 2s... (close Godot to avoid this)"
                    Start-Sleep -Seconds 2
                } else {
                    Write-Host ""
                    Write-Host "Copy failed: $name is locked. Close the Godot editor and any running game, then re-run .\build.ps1" -ForegroundColor Yellow
                    exit 1
                }
            }
        }
        if ($copied) { Write-Host "Copied to bin\$name" }
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
