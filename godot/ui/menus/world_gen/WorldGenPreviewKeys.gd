class_name WorldGenPreviewKeysGd
extends RefCounted
## Dictionary keys for world-gen preview payloads (GrowthSim → WorldGenMarshal.cpp).
## Keep in sync with WorldGenPreviewKeys.cs in godot/autoload/sim/.

const OK: String = "ok"
const ERROR: String = "error"
const LOG_LINES: String = "log_lines"
const PERF_LINES: String = "perf_lines"

const SITES: String = "sites"
const TRIANGLES: String = "triangles"
const CIRCUMCENTERS: String = "circumcenters"
const CELLS: String = "cells"
const PREVIEW_CELL_RINGS_INCLUDED: String = "preview_cell_rings_included"

const PLATE_REGIONS: String = "plate_regions"
const PLATE_ELEVATION: String = "plate_elevation"
const PLATE_MOISTURE: String = "plate_moisture"
const PLANET_PRESET: String = "planet_preset"

const REGION_ELEVATION: String = "region_elevation"
const REGION_MOISTURE: String = "region_moisture"
const TRIANGLE_ELEVATION: String = "triangle_elevation"
const TRIANGLE_MOISTURE: String = "triangle_moisture"

const USE_PLANET_TERRAIN_MESH: String = "use_planet_terrain_mesh"
const TOPOLOGY_TRIANGLES: String = "topology_triangles"
const PLANET_TERRAIN_MESH_NUM_REGIONS: String = "planet_terrain_mesh_num_regions"
const PLANET_TERRAIN_MESH_VERTICES: String = "planet_terrain_mesh_vertices"
const PLANET_TERRAIN_MESH_NORMALS: String = "planet_terrain_mesh_normals"
const PLANET_TERRAIN_MESH_INDICES: String = "planet_terrain_mesh_indices"
const PLANET_TERRAIN_MESH_RIVER_STRENGTH: String = "planet_terrain_mesh_river_strength"
const PLANET_TERRAIN_MESH_VERTEX_ELEVATION: String = "planet_terrain_mesh_vertex_elevation"
const PLANET_TERRAIN_MESH_VERTEX_MOISTURE: String = "planet_terrain_mesh_vertex_moisture"
const PLANET_TERRAIN_MESH_PRE_EROSION_VERTICES: String = "planet_terrain_mesh_pre_erosion_vertices"
const PLANET_TERRAIN_MESH_PRE_EROSION_NORMALS: String = "planet_terrain_mesh_pre_erosion_normals"
const PLANET_TERRAIN_MESH_RIVER_LINES: String = "planet_terrain_mesh_river_lines"
const PLANET_TERRAIN_MESH_TRIANGLE_REGION: String = "planet_terrain_mesh_triangle_region"

const MESH_S_BEGIN_R: String = "mesh_s_begin_r"
const MESH_S_END_R: String = "mesh_s_end_r"
const MESH_S_INNER_T: String = "mesh_s_inner_t"
const MESH_S_OUTER_T: String = "mesh_s_outer_t"
const MESH_S_NEXT_S: String = "mesh_s_next_s"
const MESH_S_TWIN_S: String = "mesh_s_twin_s"
