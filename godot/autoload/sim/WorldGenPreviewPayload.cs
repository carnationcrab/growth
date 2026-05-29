using Godot;

/// <summary>
/// Typed view of a normalised world-gen result dictionary for planet preview presentation.
/// </summary>
public sealed class WorldGenPreviewPayload
{
	public bool UsePlanetTerrainMesh { get; init; }
	public string PlanetPreset { get; init; } = "Earthlike";

	public Variant Sites { get; init; }
	public Variant Triangles { get; init; }
	public Variant Circumcenters { get; init; }
	public Variant Cells { get; init; }
	public Variant PlateRegions { get; init; }
	public Variant PlateElevation { get; init; }
	public Variant PlateMoisture { get; init; }

	public Variant PlanetTerrainMeshVertices { get; init; }
	public Variant PlanetTerrainMeshNormals { get; init; }
	public Variant PlanetTerrainMeshIndices { get; init; }
	public Variant PlanetTerrainMeshRiverStrength { get; init; }
	public Variant PlanetTerrainMeshPreErosionVertices { get; init; }
	public Variant PlanetTerrainMeshPreErosionNormals { get; init; }
	public Variant RegionElevation { get; init; }
	public Variant RegionMoisture { get; init; }
	public Variant PlanetTerrainMeshVertexElevation { get; init; }
	public Variant PlanetTerrainMeshVertexMoisture { get; init; }
	public Variant PlanetTerrainMeshRiverLines { get; init; }

	public static WorldGenPreviewPayload FromDictionary(Godot.Collections.Dictionary result)
	{
		string preset = result.ContainsKey(WorldGenPreviewKeys.PlanetPreset)
			? result[WorldGenPreviewKeys.PlanetPreset].AsString()
			: "Earthlike";
		if (string.IsNullOrEmpty(preset))
			preset = "Earthlike";

		return new WorldGenPreviewPayload
		{
			UsePlanetTerrainMesh = result.ContainsKey(WorldGenPreviewKeys.UsePlanetTerrainMesh)
				&& result[WorldGenPreviewKeys.UsePlanetTerrainMesh].AsBool(),
			PlanetPreset = preset,
			Sites = GetOrNil(result, WorldGenPreviewKeys.Sites),
			Triangles = GetOrNil(result, WorldGenPreviewKeys.Triangles),
			Circumcenters = GetOrNil(result, WorldGenPreviewKeys.Circumcenters),
			Cells = GetOrNil(result, WorldGenPreviewKeys.Cells),
			PlateRegions = GetOrNil(result, WorldGenPreviewKeys.PlateRegions),
			PlateElevation = GetOrNil(result, WorldGenPreviewKeys.PlateElevation),
			PlateMoisture = GetOrNil(result, WorldGenPreviewKeys.PlateMoisture),
			PlanetTerrainMeshVertices = GetOrNil(result, WorldGenPreviewKeys.PlanetTerrainMeshVertices),
			PlanetTerrainMeshNormals = GetOrNil(result, WorldGenPreviewKeys.PlanetTerrainMeshNormals),
			PlanetTerrainMeshIndices = GetOrNil(result, WorldGenPreviewKeys.PlanetTerrainMeshIndices),
			PlanetTerrainMeshRiverStrength = GetOrNil(result, WorldGenPreviewKeys.PlanetTerrainMeshRiverStrength),
			PlanetTerrainMeshPreErosionVertices = GetOrNil(result, WorldGenPreviewKeys.PlanetTerrainMeshPreErosionVertices),
			PlanetTerrainMeshPreErosionNormals = GetOrNil(result, WorldGenPreviewKeys.PlanetTerrainMeshPreErosionNormals),
			RegionElevation = GetOrNil(result, WorldGenPreviewKeys.RegionElevation),
			RegionMoisture = GetOrNil(result, WorldGenPreviewKeys.RegionMoisture),
			PlanetTerrainMeshVertexElevation = GetOrNil(result, WorldGenPreviewKeys.PlanetTerrainMeshVertexElevation),
			PlanetTerrainMeshVertexMoisture = GetOrNil(result, WorldGenPreviewKeys.PlanetTerrainMeshVertexMoisture),
			PlanetTerrainMeshRiverLines = GetOrNil(result, WorldGenPreviewKeys.PlanetTerrainMeshRiverLines),
		};
	}

	public bool HasTerrainMesh =>
		VariantCount(PlanetTerrainMeshVertices) > 0 && VariantCount(PlanetTerrainMeshIndices) > 0;

	public bool WantsTerrainMesh =>
		UsePlanetTerrainMesh || PlanetPreset == "FloatingIslands";

	private static Variant GetOrNil(Godot.Collections.Dictionary dict, string key)
		=> dict.ContainsKey(key) ? dict[key] : default;

	public static int VariantCount(Variant v)
	{
		if (v.VariantType == Variant.Type.Nil)
			return 0;
		if (v.VariantType == Variant.Type.Array)
			return v.AsGodotArray()?.Count ?? 0;
		var vec3Packed = v.As<Vector3[]>();
		if (vec3Packed != null)
			return vec3Packed.Length;
		var intsPacked = v.As<int[]>();
		if (intsPacked != null)
			return intsPacked.Length;
		var floatsPacked = v.As<float[]>();
		if (floatsPacked != null)
			return floatsPacked.Length;
		var vec3 = v.As<Vector3[]>();
		if (vec3 != null)
			return vec3.Length;
		var ints = v.As<int[]>();
		if (ints != null)
			return ints.Length;
		var floats = v.As<float[]>();
		if (floats != null)
			return floats.Length;
		var doubles = v.As<double[]>();
		if (doubles != null)
			return doubles.Length;
		return 0;
	}
}
