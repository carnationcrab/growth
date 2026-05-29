class_name WorldGenCardPaths
extends RefCounted
## Node paths for the world-gen menu card layout (relative to menu root unless noted).

const CARD_VBOX: String = "MarginContainer/CenterContainer/Card/CardPadding/CardVBox"
const CARD_PANEL: String = "MarginContainer/CenterContainer/Card"

const SEED_EDIT: String = "SeedRow/SeedEdit"
const RANDOM_SEED_BTN: String = "SeedRow/RandomSeedBtn"
const PRESET_OPT: String = "PresetRow/PresetOpt"
const SIZE_OPT: String = "SizeRow/SizeOpt"
const MODE_OPT: String = "ModeRow/ModeOpt"
const USE_SIMD_CHECK: String = "SimdRow/UseSimdCheckBox"
const PLANET_TERRAIN_MESH_CHECK: String = "PlanetTerrainMeshRow/PlanetTerrainMeshCheckBox"
const GENERATE_BTN: String = "BottomRow/GenerateBtn"
const BACK_BTN: String = "BottomRow/BackBtn"

const TEMP_SLIDER: String = "TempBlock/TempSlider"
const TEMP_VALUE: String = "TempBlock/TempTopRow/TempValue"
const PRECIP_SLIDER: String = "PrecipBlock/PrecipSlider"
const PRECIP_VALUE: String = "PrecipBlock/PrecipTopRow/PrecipValue"
const POINTS_SLIDER: String = "PointsBlock/PointsSlider"
const POINTS_VALUE: String = "PointsBlock/PointsTopRow/PointsValue"
const PLATES_SLIDER: String = "PlatesBlock/PlatesSlider"
const PLATES_VALUE: String = "PlatesBlock/PlatesTopRow/PlatesValue"
const JITTER_SLIDER: String = "JitterBlock/JitterSlider"
const JITTER_VALUE: String = "JitterBlock/JitterTopRow/JitterValue"

const PLANET_PRESET_IDS: Array = ["Earthlike", "OceanWorld", "Dry", "FloatingIslands"]
const PLANET_PRESET_LABELS: Array = ["Earthlike", "Ocean world", "Dry", "Floating islands"]
const SIM_SITES_BY_SIZE: Array = [4096, 8192, 16384]
const WORLD_GEN_MODE_IDS: Array = ["max_performance", "max_determinism"]
const WORLD_GEN_MODE_LABELS: Array = ["Max performance", "Max determinism"]
const LARGE_PREVIEW_THRESHOLD: int = 16384
const MAX_VORONOI_SITES: int = 500000
const DEFAULT_VORONOI_SITES: int = 16384
const DEFAULT_NUM_PLATES: int = 32
const DEFAULT_WORLD_GEN_MODE: String = "max_performance"
const DEFAULT_PLANET_PRESET: String = "Earthlike"
const DEFAULT_SIM_SITES: int = 8192
