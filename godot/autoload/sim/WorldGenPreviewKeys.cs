/// <summary>
/// Dictionary keys for world-gen results marshalled by GrowthSim (see gde/src/WorldGenMarshal.cpp).
/// Keep in sync with WorldGenPreviewKeys.gd (class WorldGenPreviewKeysGd).
/// </summary>
public static class WorldGenPreviewKeys
{
	public const string Ok = "ok";
	public const string Error = "error";
	public const string LogLines = "log_lines";
	public const string PerfLines = "perf_lines";

	public const string Sites = "sites";
	public const string Triangles = "triangles";
	public const string Circumcenters = "circumcenters";
	public const string Cells = "cells";
	public const string PreviewCellRingsIncluded = "preview_cell_rings_included";

	public const string PlateRegions = "plate_regions";
	public const string PlateElevation = "plate_elevation";
	public const string PlateMoisture = "plate_moisture";
	public const string PlanetPreset = "planet_preset";

	public const string RegionElevation = "region_elevation";
	public const string RegionMoisture = "region_moisture";
	public const string TriangleElevation = "triangle_elevation";
	public const string TriangleMoisture = "triangle_moisture";

	public const string UsePlanetTerrainMesh = "use_planet_terrain_mesh";
	public const string TopologyTriangles = "topology_triangles";
	public const string PlanetTerrainMeshNumRegions = "planet_terrain_mesh_num_regions";
	public const string PlanetTerrainMeshVertices = "planet_terrain_mesh_vertices";
	public const string PlanetTerrainMeshNormals = "planet_terrain_mesh_normals";
	public const string PlanetTerrainMeshIndices = "planet_terrain_mesh_indices";
	public const string PlanetTerrainMeshRiverStrength = "planet_terrain_mesh_river_strength";
	public const string PlanetTerrainMeshVertexElevation = "planet_terrain_mesh_vertex_elevation";
	public const string PlanetTerrainMeshVertexMoisture = "planet_terrain_mesh_vertex_moisture";
	public const string PlanetTerrainMeshPreErosionVertices = "planet_terrain_mesh_pre_erosion_vertices";
	public const string PlanetTerrainMeshPreErosionNormals = "planet_terrain_mesh_pre_erosion_normals";
	public const string PlanetTerrainMeshRiverLines = "planet_terrain_mesh_river_lines";
	public const string PlanetTerrainMeshTriangleRegion = "planet_terrain_mesh_triangle_region";

	public const string MeshSBeginR = "mesh_s_begin_r";
	public const string MeshSEndR = "mesh_s_end_r";
	public const string MeshSInnerT = "mesh_s_inner_t";
	public const string MeshSOuterT = "mesh_s_outer_t";
	public const string MeshSNextS = "mesh_s_next_s";
	public const string MeshSTwinS = "mesh_s_twin_s";
}
