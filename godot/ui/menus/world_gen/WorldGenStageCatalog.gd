class_name WorldGenStageCatalog
extends RefCounted
## Human-readable names and hints for world-gen pipeline stage ids from sim_core.


const _STAGES: Dictionary = {
	"parse_form": {
		"title": "Parsing settings",
		"detail": "Reading your world-gen form and pipeline selection.",
	},
	"world_seed_and_genome": {
		"title": "Seed and planet genome",
		"detail": "Deriving the world seed and building the planet genome from preset and climate.",
	},
	"topology": {
		"title": "Icosphere topology",
		"detail": "Generating Voronoi sites on the sphere and building region topology (often the longest step at high site counts).",
	},
	"half_edge_mesh": {
		"title": "Mesh connectivity",
		"detail": "Building the half-edge mesh and triangle adjacency on the sphere.",
	},
	"region_neighbours": {
		"title": "Region neighbours",
		"detail": "Computing which regions border each other.",
	},
	"coarse_sim_upsample": {
		"title": "Coarse simulation and upsample",
		"detail": "Running a lower-resolution simulation pass, then upsampling fields to full resolution.",
	},
	"tectonic_plates": {
		"title": "Tectonic plates",
		"detail": "Partitioning the surface into plate regions.",
	},
	"plate_properties": {
		"title": "Plate properties",
		"detail": "Assigning plate type, age, and motion-related attributes.",
	},
	"elevation": {
		"title": "Elevation",
		"detail": "Assigning height from plates, boundaries, and noise.",
	},
	"moisture": {
		"title": "Moisture",
		"detail": "Assigning moisture from climate, elevation, and coast distance.",
	},
	"triangle_values": {
		"title": "Triangle values",
		"detail": "Interpolating per-triangle fields used by rivers and rendering.",
	},
	"priority_flood": {
		"title": "Priority flood",
		"detail": "Filling region depressions and carving saddle spill points so land drains to the ocean before rivers.",
	},
	"river_downflow": {
		"title": "River downflow",
		"detail": "Computing downhill flow directions between regions.",
	},
	"river_flow": {
		"title": "River flow",
		"detail": "Moisture-driven flow accumulation along the drainage network (mapgen4 / Red Blob Games style).",
	},
	"erosion": {
		"title": "Hydraulic erosion",
		"detail": "Lowering downstream triangle elevation where water flows downhill (1843-planet-generation assignFlow).",
	},
	"river_carve": {
		"title": "River carve",
		"detail": "Carves region elevation along high-flow edges after hydraulic erosion, then smooths and refreshes triangle heights for the terrain mesh.",
	},
	"terrain_mesh": {
		"title": "Terrain mesh",
		"detail": "Building the render mesh from dual cells (can take many minutes at 100k+ regions). Preview HUD subdivides this into displaced vertices, triangle shell, and river shading.",
	},
	"terrain_mesh_vertices": {
		"title": "Terrain mesh — displaced vertices",
		"detail": "Region centres pushed radially by elevation (dual-folded vertex pass).",
	},
	"terrain_mesh_indices": {
		"title": "Terrain mesh — triangle shell",
		"detail": "Delaunay triangle indices on displaced vertices, flat shading.",
	},
	"terrain_mesh_rivers": {
		"title": "Terrain mesh — river shading",
		"detail": "Per-triangle river flow blended into vertex colours.",
	},
	"terrain_mesh_post": {
		"title": "Terrain mesh (floating islands)",
		"detail": "Generating the floating-islands style terrain mesh.",
	},
	"marshal": {
		"title": "Preparing preview data",
		"detail": "Packaging topology and mesh data for Godot.",
	},
}


static func title_for(stage_id: String) -> String:
	if stage_id.is_empty():
		return "Initialising"
	var entry: Variant = _STAGES.get(stage_id)
	if entry is Dictionary:
		return str(entry.get("title", stage_id))
	return stage_id.replace("_", " ").capitalize()


static func detail_for(stage_id: String) -> String:
	var entry: Variant = _STAGES.get(stage_id)
	if entry is Dictionary:
		return str(entry.get("detail", ""))
	return ""


static func step_label(stage_index: int, stage_count: int) -> String:
	var count: int = maxi(stage_count, 1)
	var index: int = clampi(stage_index + 1, 1, count)
	return "Step %d/%d" % [index, count]
