# Generate source files from templates/ with placeholder substitution.
#
# Usage (from repo root):
#   .\tools\new_from_template.ps1 <layer> <template> <Name> [-Force]
#   .\tools\new_from_template.ps1 -List
#
# Examples:
#   .\tools\new_from_template.ps1 world StaticClass RegionSmoothing
#   .\tools\new_from_template.ps1 api ServiceClass CustomRunner

param(
    [Parameter(Position = 0)]
    [string] $Layer,

    [Parameter(Position = 1)]
    [string] $Template,

    [Parameter(Position = 2)]
    [string] $Name,

    [switch] $Force,
    [switch] $List
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
$TemplatesRoot = Join-Path $RepoRoot "templates"

function ConvertTo-SnakeCase([string] $text) {
    if ([string]::IsNullOrWhiteSpace($text)) { return $text }
    $s = [regex]::Replace($text, '(.)([A-Z][a-z]+)', '$1_$2')
    $s = [regex]::Replace($s, '([a-z0-9])([A-Z])', '$1_$2')
    return $s.ToLowerInvariant()
}

function Get-DefaultSubstitutions([string] $className) {
    $snake = ConvertTo-SnakeCase $className
    @{
        ClassName           = $className
        StructName          = $className
        ComponentName       = $className
        Module              = $className
        AdapterName         = $className
        MenuName            = $className
        EntityKind          = $className
        snake_name          = $snake
        snake_payload_a     = "${snake}_loaded"
        snake_payload_b     = "${snake}_unloaded"
        snake_map_name      = "${snake}s_"
        snake_def_kind      = $snake
        BriefDescription    = "TODO: describe $className."
        DependencyStruct    = "SphereTopology"
        OutputType          = "SphereHalfEdgeMesh"
        static_method       = "compute"
        build_method        = "build_$snake"
        ParsedFormHeader    = "ParsedWorldGenForm"
        ParsedFormStruct    = "ParsedWorldGenForm"
        ProductType         = "PlanetGenome"
        GuardrailsStruct    = "PlanetGuardrails"
        DefType             = "UiAssetDef"
        lookup_method       = "ui_asset"
        path_method         = "ui_asset_path"
        PayloadName         = $className
        SecondPayload       = "${className}Alt"
        DiffA               = $className
        DiffB               = "${className}Alt"
        field_name          = "samples"
        parse_function      = "parse_$snake"
        form_key            = $snake
        member              = $snake
        EventA              = $className
        EventB              = "${className}Alt"
        FileSummary         = "TODO: file purpose for $className."
        include_path        = "world/$className.hpp"
        root_element        = $snake
        child_element       = "${snake}_def"
        schema_file         = $snake
        id_attr             = "${snake}_id"
        mod_pack_id         = $snake
        def_file            = $snake
        scene_id            = $snake
        script_id           = $snake
        scene_subpath       = "ui/panels"
        menu_script_subpath = "ui/menus"
        SceneFile           = $className
        ScriptFile          = $className
        entry_binding       = "entry_menu"
        entry_scene_id      = "entry_menu"
        world_script_id     = "chunk_streaming"
        optional_binding    = "planet_preview"
        pipeline_id         = "default_globe"
        next_scene_binding  = "world_gen_load_screen"
        VBoxName            = "LayersVBox"
        LabelName           = "TitleLabel"
        Title               = $className
        ParentScene         = "SpherePreview"
        HostNode            = "SpherePreview"
        MapName             = "LayerCheckPaths"
        KeyA                = "LayerA"
        KeyB                = "LayerB"
        NodePathA           = "RowA/CheckA"
        NodePathB           = "RowB/CheckB"
        ButtonName          = "ActionBtn"
        button_handler      = "action"
        CONST_IDS           = "OPTION_IDS"
        CONST_LABELS        = "OPTION_LABELS"
        IdA                 = "id_a"
        IdB                 = "id_b"
        LabelA              = "Option A"
        LabelB              = "Option B"
        CONST_NAME          = "CHUNK_SIZE"
        value               = "16"
        private_field       = "_terrain"
        ChildNode           = "Terrain"
        Type                = "MeshInstance3D"
        public_method       = "apply_data"
        args                = "data: Array"
        uid_placeholder     = "uid://growth_template"
        script_subpath      = "ui/panels"
        new_def_file        = $snake
        example_id          = "example"
        FileName            = $className
        name                = "pi"
        function_name       = "clamp"
        Dimension           = "2"
        Alias               = "Vec2"
    }
}

function Expand-TemplateText([string] $content, [hashtable] $vars) {
    $result = $content
    foreach ($key in $vars.Keys) {
        $result = $result -replace [regex]::Escape("{{$key}}"), [string] $vars[$key]
    }
    if ($result -match '\{\{[^}]+\}\}') {
        $unresolved = [regex]::Matches($result, '\{\{[^}]+\}\}') | ForEach-Object { $_.Value } | Select-Object -Unique
        Write-Warning "Unresolved placeholders: $($unresolved -join ', ')"
    }
    return $result
}

function Write-GeneratedFile([string] $destPath, [string] $templateRel, [hashtable] $vars) {
    $srcPath = Join-Path $TemplatesRoot $templateRel
    if (-not (Test-Path $srcPath)) {
        throw "Template not found: $templateRel"
    }
    $destFull = Join-Path $RepoRoot $destPath
    $destDir = Split-Path -Parent $destFull
    if (-not (Test-Path $destDir)) {
        New-Item -ItemType Directory -Path $destDir -Force | Out-Null
    }
    if ((Test-Path $destFull) -and -not $Force) {
        throw "Already exists: $destPath (use -Force to overwrite)"
    }
    $raw = Get-Content -Path $srcPath -Raw -Encoding UTF8
    $text = Expand-TemplateText $raw $vars
    [System.IO.File]::WriteAllText($destFull, $text)
    Write-Host "  wrote $destPath"
}

# Registry: key = "layer/Template" (case-insensitive)
$Registry = @{
    "world/StaticClass" = @{
        Outputs = @(
            @{ Template = "sim_core/world/StaticClass.hpp.template"; Dest = "gde/sim_core/include/world/{0}.hpp" }
            @{ Template = "sim_core/world/StaticClass.cpp.template"; Dest = "gde/sim_core/src/world/{0}.cpp" }
        )
        SConstruct = "sim_core/src/world/{0}.cpp"
        Hints      = @(
            "Register stage in data/core/defs/world_gen_pipelines.xml"
            "Add stage function in gde/sim_core/src/world/PlanetGlobePipeline.cpp"
            "Optional: godot/debug/SpherePreview.cs preview layer"
            "See templates/README.md — checklist: sim_core/world/StaticClass"
        )
    }
    "world/StructWithFreeFunctions" = @{
        Outputs = @(
            @{ Template = "sim_core/world/StructWithFreeFunctions.hpp.template"; Dest = "gde/sim_core/include/world/{0}.hpp" }
        )
        SConstruct = "sim_core/src/world/{0}.cpp"
        Hints      = @(
            "Add matching .cpp (or use world/Implementation) and SConstruct entry"
            "Wire from PlanetGlobePipeline or PlanetGlobeGenerator"
        )
    }
    "world/Implementation" = @{
        Outputs = @(
            @{ Template = "sim_core/world/Implementation.cpp.template"; Dest = "gde/sim_core/src/world/{0}.cpp" }
        )
        SConstruct = "sim_core/src/world/{0}.cpp"
        Hints      = @("Requires existing header; register in PlanetGlobePipeline.cpp")
    }
    "api/PlainStruct" = @{
        Outputs = @(
            @{ Template = "sim_core/api/PlainStruct.hpp.template"; Dest = "gde/sim_core/include/api/{0}.hpp" }
        )
        Hints = @(
            "Extend gde/src/WorldGenFormParser.cpp for new fields"
            "Update Godot form menus (WorldGenMenu.gd)"
        )
    }
    "api/ServiceClass" = @{
        Outputs = @(
            @{ Template = "sim_core/api/ServiceClass.hpp.template"; Dest = "gde/sim_core/include/api/{0}.hpp" }
            @{ Template = "sim_core/api/ServiceClass.cpp.template"; Dest = "gde/sim_core/src/api/{0}.cpp" }
        )
        SConstruct = "sim_core/src/api/{0}.cpp"
        Hints      = @("Wire caller (WorldGenRunner, SimBridge, etc.)")
    }
    "gen/GeneratorClass" = @{
        Outputs = @(
            @{ Template = "sim_core/gen/GeneratorClass.hpp.template"; Dest = "gde/sim_core/include/gen/{0}.hpp" }
        )
        Hints = @("Add .cpp if needed and SConstruct entry")
    }
    "bridge/DiffStruct" = @{
        Outputs = @(
            @{ Template = "sim_core/bridge/DiffStruct.hpp.template"; Dest = "gde/sim_core/include/bridge/{0}.hpp" }
        )
        Hints = @("Extend gde/src/DiffConverter.cpp and SimAPI diff constants")
    }
    "ecs/ComponentStruct" = @{
        Outputs = @(
            @{ Template = "sim_core/ecs/ComponentStruct.hpp.template"; Dest = "gde/sim_core/include/ecs/{0}.hpp" }
        )
        Hints = @("Register with Registry at use site; header-only")
    }
    "base/StdlibWrapper" = @{
        Outputs = @(
            @{ Template = "sim_core/base/gateway/StdlibWrapper.hpp.template"; Dest = "gde/sim_core/include/base/gateway/{0}.hpp" }
        )
        Hints = @(
            "Set {{std_header}} in template or edit generated file (e.g. cstdio)"
            "Add #include ""{0}.hpp"" to gde/sim_core/include/base/gateway/Gateway.hpp"
            "See .cursor/rules/growth-stdlib-wrappers.mdc"
            "See templates/README.md — checklist: sim_core/base/gateway/StdlibWrapper"
        )
        ExtraVars = @{ std_header = "TODO_header" }
    }
    "gde/GodotAdapter" = @{
        Outputs = @(
            @{ Template = "gde/GodotAdapter.hpp.template"; Dest = "gde/src/{0}.hpp" }
            @{ Template = "gde/GodotAdapter.cpp.template"; Dest = "gde/src/{0}.cpp" }
        )
        SConstruct = "src/{0}.cpp"
        Hints      = @(
            "Add src/{0}.cpp and .hpp to gde/SConstruct sources"
            "Invoke from sim_api_gdextension.cpp if exposing new API"
        )
    }
    "godot/UiPanel" = @{
        Outputs = @(
            @{ Template = "godot/csharp/UiPanel.cs.template"; Dest = "godot/ui/panels/{0}.cs" }
            @{ Template = "godot/scenes/UiPanelCard.tscn.template"; Dest = "godot/ui/panels/{0}.tscn" }
        )
        Hints = @(
            "Register scene_id in data/core/defs/scene_registry.xml"
            "Parent scene must instance or load via SimAPI"
            "Canonical UI: godot/ui/panels/SpherePreviewOverlay.tscn"
        )
    }
    "godot/EntityComponentView" = @{
        Outputs = @(
            @{ Template = "godot/csharp/EntityComponentView.cs.template"; Dest = "godot/entities/components/{0}.cs" }
        )
        Hints = @("Attach to entity scene; no direct GrowthSim calls")
    }
    "godot/UiMenu" = @{
        Outputs = @(
            @{ Template = "godot/gdscript/UiMenu.gd.template"; Dest = "godot/ui/menus/{0}.gd" }
        )
        Hints = @(
            "Create matching .tscn in same folder"
            "scene_registry.xml + game_profile.xml presentation binding"
        )
    }
    "godot/WorldNode3D" = @{
        Outputs = @(
            @{ Template = "godot/gdscript/WorldNode3D.gd.template"; Dest = "godot/world/{0}.gd" }
        )
        Hints = @("Pair with .tscn; wire chunk streaming if sim-driven")
    }
}

if ($List) {
    Write-Host "Available templates (layer/template):"
    $Registry.Keys | Sort-Object | ForEach-Object { Write-Host "  $_" }
    exit 0
}

if (-not $Layer -or -not $Template) {
    Write-Host @"
Usage:
  .\tools\new_from_template.ps1 <layer> <template> <Name> [-Force]
  .\tools\new_from_template.ps1 -List

Example:
  .\tools\new_from_template.ps1 world StaticClass RegionSmoothing
"@
    exit 1
}

$key = "$Layer/$Template"
$entry = $Registry[$key]
if (-not $entry) {
    $ci = $Registry.Keys | Where-Object { $_ -ieq $key } | Select-Object -First 1
    if ($ci) { $entry = $Registry[$ci]; $key = $ci }
}
if (-not $entry) {
    Write-Error "Unknown template '$key'. Use -List."
}

$className = if ($Name) { $Name } else { $Template }
$vars = Get-DefaultSubstitutions $className
if ($entry.ExtraVars) {
    foreach ($ek in $entry.ExtraVars.Keys) {
        $vars[$ek] = $entry.ExtraVars[$ek]
    }
}
$vars["include_path"] = switch -Regex ($key) {
    "^api/" { "api/$className.hpp" }
    "^base/" { "base/$className.hpp" }
    "^gen/" { "gen/$className.hpp" }
    "^bridge/" { "bridge/$className.hpp" }
    "^ecs/" { "ecs/$className.hpp" }
    "^gde/" { "src/$className.hpp" }
    default { "world/$className.hpp" }
}

Write-Host "Generating $key -> $className"
foreach ($out in $entry.Outputs) {
    $dest = $out.Dest -f $className
    Write-GeneratedFile $dest $out.Template $vars
}

Write-Host ""
Write-Host "Next steps:"
if ($entry.SConstruct) {
    $sc = $entry.SConstruct -f $className
    Write-Host "  1. Add to gde/SConstruct sources: `"$sc`","
}
$step = if ($entry.SConstruct) { 2 } else { 1 }
foreach ($hint in $entry.Hints) {
    Write-Host ("  {0}. {1}" -f $step, $hint)
    $step++
}
Write-Host "  $step. python tools/lint_sconstruct_sources.py"
Write-Host "  $($step + 1). python tools/lint_moddability.py"
Write-Host "  See templates/README.md for full checklists."
