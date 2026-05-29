using Godot;

/// <summary>
/// Orchestrates applying a world-gen result to SpherePreview. Uses SimAPI for scene paths only;
/// SpherePreview remains a dumb view with no SimAPI dependency.
/// </summary>
public static class WorldGenPreviewApplierEngine
{
	private const int LargePreviewThreshold = 16384;
	private const string PlanetPreviewBinding = "planet_preview";
	private const string PreviewNodeName = "SpherePreview";

	public static void Apply(Node main, Godot.Collections.Dictionary result)
	{
		if (main == null)
			return;

		var payload = WorldGenPreviewPayload.FromDictionary(result);
		Node preview = EnsurePreview(main);
		if (preview == null)
			return;

		bool showMeshOnly = payload.WantsTerrainMesh && payload.HasTerrainMesh;
		preview.Call("set_planet_preset", payload.PlanetPreset);
		ApplyCoreFields(preview, payload);
		ApplyHeavyOrDeferred(preview, payload, showMeshOnly);
		// SpherePreview._Ready clears WorldRoot children; drop stale chunk refs (GW-7.0c).
		WorldViewManager.ResetStreamingOnMain(main);
	}

	public static void ApplyHeavy(Node preview, Godot.Collections.Dictionary result)
		=> ApplyHeavy(preview, WorldGenPreviewPayload.FromDictionary(result), false);

	private static void ApplyCoreFields(Node preview, WorldGenPreviewPayload payload)
	{
		if (WorldGenPreviewPayload.VariantCount(payload.Sites) > 0)
			preview.Call("set_sites", payload.Sites);
		if (WorldGenPreviewPayload.VariantCount(payload.Triangles) > 0)
			preview.Call("set_triangles", payload.Triangles);
		if (WorldGenPreviewPayload.VariantCount(payload.Circumcenters) > 0)
			preview.Call("set_circumcenters", payload.Circumcenters);
		if (WorldGenPreviewPayload.VariantCount(payload.PlateRegions) > 0)
			preview.Call("set_plate_regions", payload.PlateRegions);
		CallIfPresent(preview, payload.PlateElevation, "set_plate_elevation");
		CallIfPresent(preview, payload.PlateMoisture, "set_plate_moisture");
	}

	private static void ApplyHeavyOrDeferred(Node preview, WorldGenPreviewPayload payload, bool showMeshOnly)
	{
		bool defer = WorldGenPreviewPayload.VariantCount(payload.Sites) > LargePreviewThreshold
			|| WorldGenPreviewPayload.VariantCount(payload.PlanetTerrainMeshVertices) > LargePreviewThreshold
			|| WorldGenPreviewPayload.VariantCount(payload.Cells) > LargePreviewThreshold;

		if (defer)
			ScheduleHeavy(preview, payload, showMeshOnly);
		else
			ApplyHeavy(preview, payload, showMeshOnly);
	}

	private static void ScheduleHeavy(Node preview, WorldGenPreviewPayload payload, bool showMeshOnly)
	{
		SceneTree tree = preview.GetTree();
		if (tree == null)
		{
			ApplyHeavy(preview, payload, showMeshOnly);
			return;
		}

		var timer = tree.CreateTimer(0.0);
		timer.Timeout += () => ApplyHeavy(preview, payload, showMeshOnly);
	}

	private static void ApplyHeavy(Node preview, WorldGenPreviewPayload payload, bool showMeshOnly)
	{
		if (WorldGenPreviewPayload.VariantCount(payload.Cells) > 0)
			preview.Call("set_cells", payload.Cells);

		GD.Print(
			"[WorldGen] Planet terrain mesh: use_planet_terrain_mesh=",
			payload.UsePlanetTerrainMesh,
			" verts=",
			WorldGenPreviewPayload.VariantCount(payload.PlanetTerrainMeshVertices),
			" indices=",
			WorldGenPreviewPayload.VariantCount(payload.PlanetTerrainMeshIndices));

		bool appliedTerrainMesh = false;
		if (payload.WantsTerrainMesh && payload.HasTerrainMesh)
		{
			ApplyTerrainMesh(preview, payload);
			appliedTerrainMesh = true;
		}
		else if (payload.UsePlanetTerrainMesh)
		{
			GD.Print("[WorldGen] Planet terrain mesh was requested but result had no mesh data (vertices or indices missing/empty).");
		}

		if (appliedTerrainMesh)
		{
			GD.Print("[WorldGen] Planet terrain mesh applied");
			if (showMeshOnly && preview is SpherePreview sphere)
				sphere.ApplyInitialTerrainView();
		}
		else if (preview is SpherePreview noMesh)
		{
			noMesh.SetShowPlanetTerrainMeshOnly(false);
		}
	}

	private static void ApplyTerrainMesh(Node preview, WorldGenPreviewPayload payload)
	{
		if (preview is SpherePreview sphere)
		{
			sphere.set_planet_terrain_mesh(
				payload.PlanetTerrainMeshVertices,
				payload.PlanetTerrainMeshNormals,
				payload.PlanetTerrainMeshIndices,
				payload.PlanetTerrainMeshRiverStrength,
				payload.PlanetTerrainMeshPreErosionVertices,
				payload.PlanetTerrainMeshPreErosionNormals,
				payload.RegionElevation,
				payload.RegionMoisture,
				payload.PlanetTerrainMeshRiverLines,
				payload.PlanetTerrainMeshVertexElevation,
				payload.PlanetTerrainMeshVertexMoisture);
			return;
		}

		const string method = "set_planet_terrain_mesh";
		if (!preview.HasMethod(method))
		{
			GD.PushError("[WorldGen] preview node lacks ", method);
			return;
		}

		preview.Call(
			method,
			payload.PlanetTerrainMeshVertices,
			payload.PlanetTerrainMeshNormals,
			payload.PlanetTerrainMeshIndices,
			payload.PlanetTerrainMeshRiverStrength,
			payload.PlanetTerrainMeshPreErosionVertices,
			payload.PlanetTerrainMeshPreErosionNormals,
			payload.RegionElevation,
			payload.RegionMoisture,
			payload.PlanetTerrainMeshRiverLines,
			payload.PlanetTerrainMeshVertexElevation,
			payload.PlanetTerrainMeshVertexMoisture);
	}

	private static Node EnsurePreview(Node main)
	{
		Node preview = main.GetNodeOrNull(PreviewNodeName);
		foreach (Node child in main.GetChildren())
		{
			if (child.Name == PreviewNodeName && child != preview)
				child.QueueFree();
		}

		if (preview != null)
			return preview;

		var simApi = main.GetTree()?.Root?.GetNodeOrNull<SimAPI>("/root/SimAPI");
		string previewPath = simApi?.GetPresentationScenePath(PlanetPreviewBinding) ?? "";
		if (string.IsNullOrEmpty(previewPath))
		{
			GD.PushError("[WorldGen] no planet_preview in game profile");
			return null;
		}

		var previewScene = GD.Load<PackedScene>(previewPath);
		if (previewScene == null)
		{
			GD.PushError("[WorldGen] failed to load planet preview: ", previewPath);
			return null;
		}

		preview = previewScene.Instantiate();
		main.AddChild(preview);
		GD.Print("[WorldGen] added planet preview from profile: ", previewPath);
		return preview;
	}

	private static void CallIfPresent(Node preview, Variant data, string method)
	{
		if (WorldGenPreviewPayload.VariantCount(data) > 0 && preview.HasMethod(method))
			preview.Call(method, data);
	}
}
