using Godot;
using System;

/// <summary>
/// Renders the Voronoi sphere from C++ backend data only. All logic (sites, Delaunay, circumcenters, Voronoi cells) is computed in C++; this layer just draws what it receives (sites, triangles, circumcenters, cells).
/// Sim backend uses Z-up (north pole +Z); we convert to Godot Y-up for display.
/// Arrow/WASD: orbit the planet. Shift+Up/Down: dolly zoom. Ctrl releases UI focus if a control captured the keyboard.
/// </summary>
public partial class SpherePreview : Node3D
{
	private const float PointRadius = 0.38f;  // Radius of each Fibonacci point sphere
	private const float SphereRadius = 15.0f;
	private const float OrbitSpeed = 1.35f;
	private const float DollySpeed = 22.0f;
	private const float MinCameraDistance = 8.0f;
	private const float MaxCameraDistance = 90.0f;
	/// <summary>Skip Voronoi debug geometry above this (matches WorldGenPreviewApplierEngine defer threshold).</summary>
	private const int LargePreviewThreshold = 16384;

	private Camera3D _camera;
	private Node3D _focus;
	private Vector3[] _sites;
	private int[] _triangles;
	private Vector3[] _circumcenters;
	private int[][] _cells;
	private int[] _plateRegions;
	private float[] _plateElevation;
	private float[] _plateMoisture;
	private Vector3[] _planetTerrainMeshVertices;
	private Vector3[] _planetTerrainMeshNormals;
	private int[] _planetTerrainMeshIndices;
	private float[] _planetTerrainMeshRiverStrength;
	private Vector3[] _planetTerrainMeshPreErosionVertices;
	private Vector3[] _planetTerrainMeshPreErosionNormals;
	private float[] _planetRegionElevation;
	private float[] _planetRegionMoisture;
	private float[] _planetVertexElevation;
	private float[] _planetVertexMoisture;
	private Vector3[] _planetRiverLineSegmentsSim;
	private Vector3[] _terrainDisplacedVerts;
	private Vector3[] _terrainDisplacedNormals;
	private Vector3[] _terrainSphereVerts;
	private Vector3[] _terrainSphereNormals;
	private int[] _terrainWoundIndices;
	private bool _initialTerrainViewApplied;
	private string _planetPreset = "Earthlike";
	private Vector2 _referenceViewportSize = Vector2.Zero;

	private bool IsLargePreview =>
		(_sites?.Length ?? 0) > LargePreviewThreshold
		|| (_circumcenters?.Length ?? 0) > LargePreviewThreshold
		|| (_planetTerrainMeshVertices?.Length ?? 0) > LargePreviewThreshold;

	public override void _Ready()
	{
		ClearWorldTerrain();
		BuildSphere();
		InitPreviewCamera();
		SoftenPreviewLighting();
		AddOverlay();
		_referenceViewportSize = GetViewport().GetVisibleRect().Size;
		GD.Print("[SpherePreview] ready. Arrows/WASD orbit the planet; Shift+Up/Down dolly zoom.");
	}

	private void AddOverlay()
	{
		var scene = GD.Load<PackedScene>("res://godot/ui/panels/SpherePreviewOverlay.tscn");
		if (scene == null)
		{
			GD.PushWarning("[SpherePreview] could not load SpherePreviewOverlay.tscn");
			return;
		}
		var layer = new CanvasLayer();
		layer.Name = "OverlayLayer";
		AddChild(layer);
		var overlay = scene.Instantiate<SpherePreviewOverlay>();
		layer.AddChild(overlay);
		CallDeferred(MethodName.DeferredConnectOverlay);
	}

	private void DeferredConnectOverlay()
	{
		GetOverlay()?.EnsureLayerTogglesConnected();
	}

	private SpherePreviewOverlay GetOverlay() =>
		GetNodeOrNull<SpherePreviewOverlay>("OverlayLayer/SpherePreviewOverlay");

	/// <summary>Set Fibonacci/Voronoi sites (unit-sphere points in sim Z-up). We convert to Y-up for display.</summary>
	public void SetSites(Vector3[] sites)
	{
		if (sites == null || sites.Length == 0)
		{
			GD.PushWarning("[SpherePreview] SetSites: no sites.");
			return;
		}
		GD.Print("[SpherePreview] SetSites: ", sites.Length, " points.");
		_sites = new Vector3[sites.Length];
		for (int i = 0; i < sites.Length; i++)
			_sites[i] = PresentationCoords.SimToGodot(sites[i]);
		if (IsLargePreview)
			GD.Print("[SpherePreview] Sites layer omitted (", sites.Length, " points; use terrain mesh view).");
		else
			BuildPoints(_sites);
		if (_triangles != null && _triangles.Length >= 3 && !IsLargePreview)
			BuildDelaunayLayer();
		SyncOverlayLayers();
	}

	/// <summary>Set sites from GDScript (accepts Array or marshalled array from apply_world_gen_form).</summary>
	public void SetSites(Godot.Collections.Array sitesArray)
	{
		if (sitesArray == null || sitesArray.Count == 0)
		{
			GD.PushWarning("[SpherePreview] SetSites: no sites.");
			return;
		}
		var arr = new Vector3[sitesArray.Count];
		for (int i = 0; i < sitesArray.Count; i++)
			arr[i] = (Vector3)sitesArray[i];
		SetSites(arr);
	}

	/// <summary>GDScript-callable alias (snake_case).</summary>
	public void set_sites(Variant sites)
	{
		var arr = sites.As<Godot.Collections.Array>();
		if (arr != null && arr.Count > 0)
		{
			SetSites(arr);
			return;
		}
		var vecArr = sites.As<Vector3[]>();
		if (vecArr != null && vecArr.Length > 0)
		{
			SetSites(vecArr);
			return;
		}
		GD.PushWarning("[SpherePreview] set_sites: could not convert to sites array.");
	}

	/// <summary>Set Delaunay triangle indices (flat: i0, i1, i2, i0, i1, i2, ...). Call after or before SetSites; mesh built when both are set.</summary>
	public void SetTriangles(int[] triangles)
	{
		_triangles = triangles;
		GD.Print("[SpherePreview] SetTriangles: ", triangles?.Length ?? 0, " indices; _sites=", _sites?.Length ?? 0);
		if (_sites != null && triangles != null && triangles.Length >= 3 && !IsLargePreview)
			BuildDelaunayLayer();
	}

	/// <summary>GDScript-callable; accepts Array or PackedInt32Array (pass Array(triangles) from GDScript for best compatibility).</summary>
	public void set_triangles(Variant triangles)
	{
		// Prefer int[] (e.g. when marshalled from extension)
		var arr = triangles.As<int[]>();
		if (arr != null && arr.Length >= 3)
		{
			SetTriangles(arr);
			return;
		}
		// Godot.Collections.Array from GDScript Array(triangles)
		var garr = triangles.As<Godot.Collections.Array>();
		if (garr != null && garr.Count >= 3)
		{
			var iarr = new int[garr.Count];
			for (int i = 0; i < garr.Count; i++)
				iarr[i] = (int)garr[i].As<long>();
			SetTriangles(iarr);
			return;
		}
		GD.PushWarning("[SpherePreview] set_triangles: could not convert to int array (VariantType=", (int)triangles.VariantType, ").");
	}

	/// <summary>Voronoi vertices (triangle circumcenters on unit sphere, sim Z-up). We convert to Y-up and scale by SphereRadius.</summary>
	public void SetCircumcenters(Vector3[] circumcenters)
	{
		_circumcenters = circumcenters;
		GD.Print("[SpherePreview] SetCircumcenters: ", circumcenters?.Length ?? 0);
		if (_circumcenters != null && _circumcenters.Length > 0)
		{
			if (!IsLargePreview)
				BuildCircumcentersLayer();
			else
				GD.Print("[SpherePreview] Circumcenters layer omitted (", _circumcenters.Length, " points).");
			if (_cells != null && _cells.Length > 0)
			{
				BuildVoronoiRegionsLayer();
				AfterVoronoiGeometryUpdated();
			}
		}
		SyncOverlayLayers();
	}

	/// <summary>GDScript-callable; accepts Array or Vector3[] from apply_world_gen_form (PackedVector3Array marshals as one of these in C#).</summary>
	public void set_circumcenters(Variant circumcenters)
	{
		var vecArr = circumcenters.As<Vector3[]>();
		if (vecArr != null && vecArr.Length > 0)
		{
			SetCircumcenters(vecArr);
			return;
		}
		var arr = circumcenters.As<Godot.Collections.Array>();
		if (arr != null && arr.Count > 0)
		{
			var vecs = new Vector3[arr.Count];
			for (int i = 0; i < arr.Count; i++)
				vecs[i] = (Vector3)arr[i];
			SetCircumcenters(vecs);
			return;
		}
		GD.PushWarning("[SpherePreview] set_circumcenters: could not convert to Vector3 array (VariantType=", (int)circumcenters.VariantType, ").");
	}

	/// <summary>Per-site Voronoi cells: each cell is an ordered list of circumcenter (triangle) indices.</summary>
	public void SetCells(int[][] cells)
	{
		_cells = cells;
		GD.Print("[SpherePreview] SetCells: ", cells?.Length ?? 0, " cells.");
		if (_circumcenters != null && _circumcenters.Length > 0)
		{
			if (!IsLargePreview)
				BuildCircumcentersLayer();
			else
				GD.Print("[SpherePreview] Circumcenters layer omitted (", _circumcenters.Length, " points).");
			if (_cells != null && _cells.Length > 0)
			{
				BuildVoronoiRegionsLayer();
				AfterVoronoiGeometryUpdated();
			}
		}
		SyncOverlayLayers();
	}

	/// <summary>GDScript-callable; accepts Array of Array (each inner Array is PackedInt32Array of circumcenter indices).</summary>
	public void set_cells(Variant cells)
	{
		var arr = cells.As<Godot.Collections.Array>();
		if (arr == null || arr.Count == 0)
		{
			GD.PushWarning("[SpherePreview] set_cells: no cells array.");
			return;
		}
		var cellList = new int[arr.Count][];
		for (int i = 0; i < arr.Count; i++)
		{
			var inner = arr[i].As<Godot.Collections.Array>();
			if (inner != null && inner.Count > 0)
			{
				cellList[i] = new int[inner.Count];
				for (int j = 0; j < inner.Count; j++)
					cellList[i][j] = (int)inner[j].As<long>();
			}
			else
			{
				var packed = arr[i].As<int[]>();
				if (packed != null && packed.Length > 0)
					cellList[i] = packed;
				else
					cellList[i] = Array.Empty<int>();
			}
		}
		SetCells(cellList);
	}

	/// <summary>Plate assignment per Voronoi region: plate_regions[site_idx] = plate id (0 .. num_plates-1).</summary>
	public void SetPlateRegions(int[] plateRegions)
	{
		_plateRegions = plateRegions;
		int cellCount = _cells?.Length ?? 0;
		GD.Print("[SpherePreview] SetPlateRegions: regions=", plateRegions?.Length ?? 0,
			" circumcenters=", _circumcenters?.Length ?? 0, " cells=", cellCount,
			" terrain_verts=", _planetTerrainMeshVertices?.Length ?? 0);
		if (!TryRebuildPlateLayers())
		{
			if (cellCount == 0)
				GD.Print("[SpherePreview] SetPlateRegions: deferred (no Voronoi cell rings — large globes omit cells; will paint terrain mesh when ready)");
			else
				GD.Print("[SpherePreview] SetPlateRegions: deferred (Voronoi geometry incomplete)");
		}
		SyncOverlayLayers();
	}

	/// <summary>GDScript-callable; accepts PackedInt32Array or Array from apply_world_gen_form result.</summary>
	public void set_plate_regions(Variant plateRegions)
	{
		var packed = plateRegions.As<int[]>();
		if (packed != null && packed.Length > 0)
		{
			SetPlateRegions(packed);
			return;
		}
		var arr = plateRegions.As<Godot.Collections.Array>();
		if (arr != null && arr.Count > 0)
		{
			var iarr = new int[arr.Count];
			for (int i = 0; i < arr.Count; i++)
				iarr[i] = (int)arr[i].As<long>();
			SetPlateRegions(iarr);
			return;
		}
		GD.PushWarning("[SpherePreview] set_plate_regions: could not convert to int array.");
	}

	/// <summary>Per-plate elevation (negative = ocean, positive = land). Used to build Elevation layer; colour by plate_regions[site].</summary>
	public void SetPlateElevation(float[] elevation)
	{
		_plateElevation = elevation;
		GD.Print("[SpherePreview] SetPlateElevation: plates=", elevation?.Length ?? 0);
		if (_circumcenters == null || _cells == null || _plateRegions == null || _plateElevation == null || _plateElevation.Length == 0)
			return;
		BuildElevationLayer();
		if (_plateMoisture != null && _plateMoisture.Length > 0)
			BuildPlateLabelsLayer();
	}

	/// <summary>GDScript-callable; accepts Array or float[] (one value per plate).</summary>
	public void set_plate_elevation(Variant elevation)
	{
		var floats = elevation.As<float[]>();
		if (floats != null && floats.Length > 0)
		{
			SetPlateElevation(floats);
			return;
		}
		var arr = elevation.As<Godot.Collections.Array>();
		if (arr != null && arr.Count > 0)
		{
			var farr = new float[arr.Count];
			for (int i = 0; i < arr.Count; i++)
				farr[i] = (float)arr[i].As<double>();
			SetPlateElevation(farr);
			return;
		}
		GD.PushWarning("[SpherePreview] set_plate_elevation: could not convert to float array.");
	}

	/// <summary>Per-plate moisture (0–1). Used to build Moisture layer; colour by plate_regions[site].</summary>
	public void SetPlateMoisture(float[] moisture)
	{
		_plateMoisture = moisture;
		GD.Print("[SpherePreview] SetPlateMoisture: plates=", moisture?.Length ?? 0);
		if (_circumcenters == null || _cells == null || _plateRegions == null || _plateMoisture == null || _plateMoisture.Length == 0)
			return;
		BuildMoistureLayer();
		if (_plateElevation != null && _plateElevation.Length > 0)
			BuildPlateLabelsLayer();
	}

	/// <summary>GDScript-callable; accepts Array or float[] (one value per plate).</summary>
	public void set_plate_moisture(Variant moisture)
	{
		var floats = moisture.As<float[]>();
		if (floats != null && floats.Length > 0)
		{
			SetPlateMoisture(floats);
			return;
		}
		var arr = moisture.As<Godot.Collections.Array>();
		if (arr != null && arr.Count > 0)
		{
			var farr = new float[arr.Count];
			for (int i = 0; i < arr.Count; i++)
				farr[i] = (float)arr[i].As<double>();
			SetPlateMoisture(farr);
			return;
		}
		GD.PushWarning("[SpherePreview] set_plate_moisture: could not convert to float array.");
	}

	/// <summary>Active planet preset id from world-gen form (e.g. Earthlike, FloatingIslands).</summary>
	public void SetPlanetPreset(string preset)
	{
		_planetPreset = string.IsNullOrEmpty(preset) ? "Earthlike" : preset;
		GD.Print("[SpherePreview] SetPlanetPreset: ", _planetPreset);
	}

	/// <summary>GDScript-callable.</summary>
	public void set_planet_preset(string preset) => SetPlanetPreset(preset);

	/// <summary>Restore default preview layer visibility (all diagnostic layers on, terrain mesh off).</summary>
	public void ApplyDefaultWorldGenLayerVisibility()
	{
		_initialTerrainViewApplied = false;
		GetOverlay()?.UpdateTerrainMeshUiMode(false);
		SyncOverlayLayers();
	}

	/// <summary>Whether a watertight planet terrain mesh is loaded (for overlay UI mode).</summary>
	public bool HasActivePlanetTerrainMesh() => HasPlanetTerrainMeshData();

	/// <summary>One-time hide of Voronoi clutter and show final terrain mesh after world-gen apply.</summary>
	public void ApplyInitialTerrainView()
	{
		if (!HasPlanetTerrainMeshData() || _initialTerrainViewApplied)
			return;
		_initialTerrainViewApplied = true;
		HideDiagnosticLayersForTerrainView();
		var terrainRoot = GetNodeOrNull<Node3D>("PlanetTerrainMesh");
		if (terrainRoot != null)
		{
			terrainRoot.Visible = true;
			var finalTerrain = terrainRoot.GetNodeOrNull<Node3D>("FinalTerrain");
			if (finalTerrain != null)
				finalTerrain.Visible = true;
		}
		GetOverlay()?.ApplyInitialTerrainView();
	}

	/// <summary>GDScript-callable.</summary>
	public void apply_default_world_gen_layer_visibility() => ApplyDefaultWorldGenLayerVisibility();

	/// <summary>Set the planet terrain mesh and per-vertex map fields (RBG quad-fold: regions + triangle centroids).</summary>
	private void SetPlanetTerrainMesh(Vector3[] vertices, Vector3[] normals, int[] indices, float[] riverStrength,
		Vector3[] preErosionVertices = null, Vector3[] preErosionNormals = null,
		float[] regionElevation = null, float[] regionMoisture = null, Vector3[] riverLineSegmentsSim = null,
		float[] vertexElevation = null, float[] vertexMoisture = null)
	{
		_planetTerrainMeshVertices = vertices;
		_planetTerrainMeshNormals = normals;
		_planetTerrainMeshIndices = indices;
		_planetTerrainMeshRiverStrength = riverStrength;
		_planetTerrainMeshPreErosionVertices = preErosionVertices;
		_planetTerrainMeshPreErosionNormals = preErosionNormals;
		_planetRegionElevation = regionElevation;
		_planetRegionMoisture = regionMoisture;
		_planetVertexElevation = vertexElevation;
		_planetVertexMoisture = vertexMoisture;
		_planetRiverLineSegmentsSim = riverLineSegmentsSim;
		BuildPlanetTerrainMeshLayer();
		CallDeferred(MethodName.DeferredTryRebuildPlateLayers);
	}

	private void DeferredTryRebuildPlateLayers()
	{
		TryRebuildPlateLayers();
		SyncOverlayLayers();
	}

	private bool HasPlanetTerrainMeshData() =>
		_planetTerrainMeshVertices != null
		&& _planetTerrainMeshIndices != null
		&& _planetTerrainMeshVertices.Length > 0
		&& _planetTerrainMeshIndices.Length >= 3;

	private void AfterVoronoiGeometryUpdated()
	{
		if (!HasPlanetTerrainMeshData())
			TryRebuildPlateLayers();
		SyncOverlayLayers();
	}

	/// <summary>Callable from GDScript / <see cref="GodotObject.Call"/>; accepts result arrays from apply_world_gen_form.</summary>
	public void set_planet_terrain_mesh(
		Variant vertices,
		Variant normals,
		Variant indices,
		Variant riverStrength,
		Variant preErosionVertices,
		Variant preErosionNormals,
		Variant regionElevation,
		Variant regionMoisture,
		Variant riverLineSegments,
		Variant vertexElevation,
		Variant vertexMoisture)
	{
		Vector3[] vArr = _variant_to_vector3_array(vertices);
		Vector3[] nArr = _variant_to_vector3_array(normals);
		int[] iArr = _variant_to_int_array(indices);
		float[] rArr = _variant_to_float_array(riverStrength);
		Vector3[] preV = preErosionVertices.VariantType != Variant.Type.Nil ? _variant_to_vector3_array(preErosionVertices) : null;
		Vector3[] preN = preErosionNormals.VariantType != Variant.Type.Nil ? _variant_to_vector3_array(preErosionNormals) : null;
		float[] elev = regionElevation.VariantType != Variant.Type.Nil ? _variant_to_float_array(regionElevation) : null;
		float[] moist = regionMoisture.VariantType != Variant.Type.Nil ? _variant_to_float_array(regionMoisture) : null;
		Vector3[] riverSeg = riverLineSegments.VariantType != Variant.Type.Nil ? _variant_to_vector3_array(riverLineSegments) : null;
		float[] vtxElev = vertexElevation.VariantType != Variant.Type.Nil ? _variant_to_float_array(vertexElevation) : null;
		float[] vtxMoist = vertexMoisture.VariantType != Variant.Type.Nil ? _variant_to_float_array(vertexMoisture) : null;
		if (vArr != null && vArr.Length > 0 && iArr != null && iArr.Length >= 3)
			SetPlanetTerrainMesh(vArr, nArr, iArr, rArr, preV, preN, elev, moist, riverSeg, vtxElev, vtxMoist);
		else
			GD.PushWarning("[SpherePreview] set_planet_terrain_mesh: invalid or empty vertices/indices (verts=",
				vArr?.Length ?? 0, " indices=", iArr?.Length ?? 0, " variantTypes=",
				(int)vertices.VariantType, "/", (int)indices.VariantType, ").");
	}

	/// <summary>When true, applies one-shot initial terrain view; when false, re-enables full layer UI.</summary>
	public void SetShowPlanetTerrainMeshOnly(bool only)
	{
		if (only)
			ApplyInitialTerrainView();
		else
		{
			_initialTerrainViewApplied = false;
			GetOverlay()?.UpdateTerrainMeshUiMode(false);
			SyncOverlayLayers();
		}
	}

	/// <summary>GDScript-callable.</summary>
	public void set_show_planet_terrain_mesh_only(bool only)
	{
		SetShowPlanetTerrainMeshOnly(only);
	}

	private static Vector3[] _variant_to_vector3_array(Variant v)
	{
		if (v.VariantType == Variant.Type.Nil)
			return null;
		var arr = v.As<Vector3[]>();
		if (arr != null && arr.Length > 0)
			return arr;
		// PackedVector3Array and similar marshal as Vector3[] via As<> in Godot 4 C#.
		var list = v.As<Godot.Collections.Array>();
		if (list != null && list.Count > 0)
		{
			var outArr = new Vector3[list.Count];
			for (int i = 0; i < list.Count; i++)
				outArr[i] = (Vector3)list[i];
			return outArr;
		}
		return null;
	}

	private static int[] _variant_to_int_array(Variant v)
	{
		if (v.VariantType == Variant.Type.Nil)
			return null;
		var arr = v.As<int[]>();
		if (arr != null && arr.Length > 0)
			return arr;
		var list = v.As<Godot.Collections.Array>();
		if (list != null && list.Count > 0)
		{
			var outArr = new int[list.Count];
			for (int i = 0; i < list.Count; i++)
				outArr[i] = (int)list[i].As<long>();
			return outArr;
		}
		return null;
	}

	private static float[] _variant_to_float_array(Variant v)
	{
		if (v.VariantType == Variant.Type.Nil)
			return null;
		var arr = v.As<float[]>();
		if (arr != null && arr.Length > 0)
			return arr;
		var doubles = v.As<double[]>();
		if (doubles != null && doubles.Length > 0)
		{
			var outArr = new float[doubles.Length];
			for (int i = 0; i < doubles.Length; i++)
				outArr[i] = (float)doubles[i];
			return outArr;
		}
		var list = v.As<Godot.Collections.Array>();
		if (list != null && list.Count > 0)
		{
			var outArr = new float[list.Count];
			for (int i = 0; i < list.Count; i++)
				outArr[i] = (float)list[i].As<double>();
			return outArr;
		}
		return null;
	}

	private void ClearTerrainMeshCache()
	{
		_terrainDisplacedVerts = null;
		_terrainDisplacedNormals = null;
		_terrainSphereVerts = null;
		_terrainSphereNormals = null;
		_terrainWoundIndices = null;
	}

	private void EnsureTerrainMeshCache()
	{
		int nV = _planetTerrainMeshVertices?.Length ?? 0;
		int nI = _planetTerrainMeshIndices?.Length ?? 0;
		if (_planetTerrainMeshVertices == null || _planetTerrainMeshIndices == null || nV == 0 || nI < 3)
			return;
		if (_terrainWoundIndices != null && _terrainWoundIndices.Length > 0 && _terrainDisplacedVerts != null && _terrainDisplacedVerts.Length == nV)
			return;

		const float radiusScale = 1.002f;
		_terrainDisplacedVerts = ScaleTerrainVertices(_planetTerrainMeshVertices, _planetTerrainMeshNormals, radiusScale, out _terrainDisplacedNormals);
		_terrainSphereVerts = BuildUnitSphereDisplayVerts(_planetTerrainMeshVertices, radiusScale, out _terrainSphereNormals);
		_terrainWoundIndices = PresentationCoords.PrepareTerrainMeshIndicesForDisplay(
			_planetTerrainMeshIndices, _terrainDisplacedVerts);
	}

	private Vector3[] BuildUnitSphereDisplayVerts(Vector3[] simVertices, float radiusScale, out Vector3[] godotNormals)
	{
		int nV = simVertices.Length;
		var verts = new Vector3[nV];
		godotNormals = new Vector3[nV];
		float r = SphereRadius * radiusScale;
		for (int i = 0; i < nV; i++)
		{
			Vector3 simDir = simVertices[i];
			if (simDir.LengthSquared() < 1e-12f)
				simDir = new Vector3(0f, 0f, 1f);
			else
				simDir = simDir.Normalized();
			Vector3 godotDir = PresentationCoords.SimDirectionToGodot(simDir);
			verts[i] = godotDir * r;
			godotNormals[i] = godotDir;
		}
		return verts;
	}

	private void RemoveTerrainHeatmapLayers()
	{
		ReadOnlySpan<string> names = new[]
		{
			"TerrainTemperature", "TerrainBiome"
		};
		foreach (string name in names)
		{
			var node = GetNodeOrNull<Node3D>(name);
			if (node != null)
				node.QueueFree();
		}
	}

	private void BuildTerrainHeatmapLayers(Vector3[] simUnitDirs)
	{
		RemoveTerrainHeatmapLayers();
		if (_terrainSphereVerts == null || _terrainWoundIndices == null || _terrainWoundIndices.Length < 3)
			return;

		int nV = _terrainSphereVerts.Length;
		AddChild(BuildTerrainHeatmapNode("TerrainTemperature", BuildTemperatureMapColors(nV, simUnitDirs)));
		AddChild(BuildTerrainHeatmapNode("TerrainBiome", BuildBiomeMapColors(nV, simUnitDirs)));
	}

	private Node3D BuildTerrainHeatmapNode(string layerName, Color[] colors)
	{
		var layer = new Node3D { Name = layerName, Visible = false };
		layer.AddChild(CreateTerrainTriangleLayer(_terrainSphereVerts, _terrainSphereNormals, colors, "Surface", visible: true));
		return layer;
	}

	private void BuildPlanetTerrainMeshLayer()
	{
		int nV = _planetTerrainMeshVertices?.Length ?? 0;
		int nI = _planetTerrainMeshIndices?.Length ?? 0;
		if (_planetTerrainMeshVertices == null || _planetTerrainMeshIndices == null || nV == 0 || nI < 3)
		{
			GD.PushWarning("[SpherePreview] BuildPlanetTerrainMeshLayer: no mesh data.");
			return;
		}

		ClearTerrainMeshCache();
		var existing = GetNodeOrNull<Node3D>("PlanetTerrainMesh");
		if (existing != null)
			existing.QueueFree();
		var existingPlates = GetNodeOrNull<Node3D>("TectonicPlates");
		if (existingPlates != null)
			existingPlates.QueueFree();
		RemoveTerrainHeatmapLayers();

		EnsureTerrainMeshCache();
		if (_terrainWoundIndices == null || _terrainWoundIndices.Length < 3)
		{
			GD.PushError("[SpherePreview] BuildPlanetTerrainMeshLayer: invalid wound indices.");
			return;
		}

		var simUnitDirs = BuildSimUnitDirections(_planetTerrainMeshVertices, nV);
		var finalColors = BuildFinalTerrainColors(nV, simUnitDirs);
		Color[] plateColors = BuildPlateTerrainColors(nV, nI);

		var root = new Node3D { Name = "PlanetTerrainMesh", Visible = true };
		AddChild(root);

		var finalLayer = new Node3D { Name = "FinalTerrain", Visible = true };
		finalLayer.AddChild(CreateTerrainTriangleLayer(_terrainDisplacedVerts, _terrainDisplacedNormals, finalColors, "Surface", visible: true));
		root.AddChild(finalLayer);

		var riversLayer = new Node3D { Name = "RiversMap", Visible = false };
		var riverLines = CreateRiverLinesLayer();
		if (riverLines != null)
			riversLayer.AddChild(riverLines);
		root.AddChild(riversLayer);

		BuildTerrainHeatmapLayers(simUnitDirs);

		if (plateColors != null)
		{
			var platesRoot = new Node3D { Name = "TectonicPlates", Visible = false };
			platesRoot.AddChild(CreateTerrainTriangleLayer(_terrainDisplacedVerts, _terrainDisplacedNormals, plateColors, "Surface", visible: true));
			AddChild(platesRoot);
		}

		GD.Print("[SpherePreview] Terrain mesh: verts=", nV, " indices=", nI,
			" riverSegments=", _planetRiverLineSegmentsSim?.Length ?? 0);
		SyncOverlayLayers();
	}

	/// <summary>Hide Voronoi-site / circumcenter debug geometry so terrain mesh view is not cluttered by black dots.</summary>
	private void HideDiagnosticLayersForTerrainView()
	{
		ReadOnlySpan<string> names = new[]
		{
			"Globe", "Sites", "Delaunay", "Circumcenters", "Voronoi",
			"TectonicPlates", "Elevation", "Moisture", "PlateLabels",
			"TerrainTemperature", "TerrainBiome"
		};
		foreach (string name in names)
		{
			var node = GetNodeOrNull<Node3D>(name);
			if (node != null)
				node.Visible = false;
		}
		var voronoi = GetNodeOrNull<Node3D>("Voronoi");
		if (voronoi == null)
			return;
		voronoi.Visible = false;
		foreach (var child in voronoi.GetChildren())
		{
			if (child is Node3D n3d)
				n3d.Visible = false;
		}
	}

	private void SyncOverlayLayers()
	{
		var overlay = GetOverlay();
		overlay?.EnsureLayerTogglesConnected();
		overlay?.SyncAllLayersFromPreview();
	}

	private Vector3[] ScaleTerrainVertices(Vector3[] simVertices, Vector3[] simNormals, float radiusScale, out Vector3[] godotNormals)
	{
		int nV = simVertices.Length;
		var verts = new Vector3[nV];
		godotNormals = new Vector3[nV];
		for (int i = 0; i < nV; i++)
		{
			verts[i] = PresentationCoords.SimToGodot(simVertices[i]) * (SphereRadius * radiusScale);
			godotNormals[i] = simNormals != null && i < simNormals.Length
				? PresentationCoords.SimToGodot(simNormals[i]).Normalized()
				: verts[i].Normalized();
		}
		return verts;
	}

	private static StandardMaterial3D CreateTerrainMapMaterial()
	{
		return new StandardMaterial3D
		{
			ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded,
			VertexColorUseAsAlbedo = true,
			CullMode = BaseMaterial3D.CullModeEnum.Back,
			DepthDrawMode = BaseMaterial3D.DepthDrawModeEnum.OpaqueOnly
		};
	}

	private static StandardMaterial3D CreateRiverLineMaterial()
	{
		return new StandardMaterial3D
		{
			ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded,
			VertexColorUseAsAlbedo = true,
			Transparency = BaseMaterial3D.TransparencyEnum.Alpha,
			CullMode = BaseMaterial3D.CullModeEnum.Disabled
		};
	}

	private Vector3[] BuildSimUnitDirections(Vector3[] simVertices, int count)
	{
		var dirs = new Vector3[count];
		for (int i = 0; i < count; i++)
		{
			Vector3 v = simVertices[i];
			dirs[i] = v.Length() > 1e-6f ? v.Normalized() : new Vector3(0f, 0f, 1f);
		}
		return dirs;
	}

	private float RegionElevationAt(int vertex)
	{
		if (_planetVertexElevation != null && vertex < _planetVertexElevation.Length)
			return _planetVertexElevation[vertex];
		if (_planetRegionElevation != null && vertex < _planetRegionElevation.Length)
			return _planetRegionElevation[vertex];
		if (_planetTerrainMeshVertices != null && vertex < _planetTerrainMeshVertices.Length)
		{
			float radius = _planetTerrainMeshVertices[vertex].Length();
			return (radius - 1f) / 0.08f;
		}
		return 0f;
	}

	private float RegionMoistureAt(int vertex)
	{
		if (_planetVertexMoisture != null && vertex < _planetVertexMoisture.Length)
			return _planetVertexMoisture[vertex];
		if (_planetRegionMoisture != null && vertex < _planetRegionMoisture.Length)
			return _planetRegionMoisture[vertex];
		return 0.5f;
	}

	private Color[] BuildTemperatureMapColors(int vertexCount, Vector3[] simUnitDirs)
	{
		var colors = new Color[vertexCount];
		for (int i = 0; i < vertexCount; i++)
		{
			float elev = RegionElevationAt(i);
			float temp = PlanetMapPalette.RegionTemperature(simUnitDirs[i], elev);
			colors[i] = PlanetMapPalette.TemperatureColor(temp);
		}
		return colors;
	}

	private Color[] BuildBiomeMapColors(int vertexCount, Vector3[] simUnitDirs)
	{
		var colors = new Color[vertexCount];
		for (int i = 0; i < vertexCount; i++)
		{
			float elev = RegionElevationAt(i);
			float moist = RegionMoistureAt(i);
			float temp = PlanetMapPalette.RegionTemperature(simUnitDirs[i], elev);
			colors[i] = PlanetMapPalette.BiomeColor(elev, moist, temp);
		}
		return colors;
	}

	private Color[] BuildFinalTerrainColors(int vertexCount, Vector3[] simUnitDirs)
	{
		var colors = new Color[vertexCount];
		for (int i = 0; i < vertexCount; i++)
		{
			float elev = RegionElevationAt(i);
			float moist = RegionMoistureAt(i);
			float temp = PlanetMapPalette.RegionTemperature(simUnitDirs[i], elev);
			var biome = PlanetMapPalette.BiomeColor(elev, moist, temp);
			Vector3 normal = PresentationCoords.SimDirectionToGodot(simUnitDirs[i]);
			colors[i] = PlanetMapPalette.ApplySoftShading(biome, normal);
		}
		return colors;
	}

	private MeshInstance3D CreateRiverLinesLayer()
	{
		if (_planetRiverLineSegmentsSim == null || _planetRiverLineSegmentsSim.Length < 2)
			return null;

		const float radiusScale = 1.002f;
		const float lift = 0.04f;
		int segmentCount = _planetRiverLineSegmentsSim.Length / 2;
		var verts = new Vector3[segmentCount * 2];
		var colors = new Color[segmentCount * 2];
		var lineColor = PlanetMapPalette.RiverLineColor;
		for (int i = 0; i < segmentCount; i++)
		{
			Vector3 a = PresentationCoords.SimToGodot(_planetRiverLineSegmentsSim[i * 2]) * (SphereRadius * radiusScale);
			Vector3 b = PresentationCoords.SimToGodot(_planetRiverLineSegmentsSim[i * 2 + 1]) * (SphereRadius * radiusScale);
			a += a.Normalized() * lift;
			b += b.Normalized() * lift;
			verts[i * 2] = a;
			verts[i * 2 + 1] = b;
			colors[i * 2] = lineColor;
			colors[i * 2 + 1] = lineColor;
		}

		var arrays = new Godot.Collections.Array();
		arrays.Resize((int)Mesh.ArrayType.Max);
		arrays[(int)Mesh.ArrayType.Vertex] = verts;
		arrays[(int)Mesh.ArrayType.Color] = colors;
		var arrMesh = new ArrayMesh();
		arrMesh.AddSurfaceFromArrays(Mesh.PrimitiveType.Lines, arrays);

		return new MeshInstance3D
		{
			Name = "Lines",
			Mesh = arrMesh,
			MaterialOverride = CreateRiverLineMaterial(),
			Visible = true
		};
	}

	private void SoftenPreviewLighting()
	{
		Node main = GetParent();
		var light = main?.GetNodeOrNull<DirectionalLight3D>("Focus/DirectionalLight3D");
		if (light != null)
		{
			light.LightEnergy = 0.18f;
			light.ShadowEnabled = false;
		}
		var worldEnv = main?.GetNodeOrNull<WorldEnvironment>("WorldEnvironment");
		if (worldEnv?.Environment != null)
		{
			worldEnv.Environment.AmbientLightSource = Godot.Environment.AmbientSource.Color;
			worldEnv.Environment.AmbientLightColor = new Color(0.58f, 0.6f, 0.64f);
			worldEnv.Environment.AmbientLightEnergy = 1.1f;
		}
	}

	private MeshInstance3D CreateTerrainTriangleLayer(Vector3[] verts, Vector3[] normals, Color[] colors, string layerName, bool visible)
	{
		if (verts == null || normals == null || colors == null || verts.Length == 0
		    || _terrainWoundIndices == null || _terrainWoundIndices.Length < 3)
			return new MeshInstance3D { Name = layerName, Visible = false };

		var arrays = new Godot.Collections.Array();
		arrays.Resize((int)Mesh.ArrayType.Max);
		arrays[(int)Mesh.ArrayType.Vertex] = verts;
		arrays[(int)Mesh.ArrayType.Normal] = normals;
		arrays[(int)Mesh.ArrayType.Color] = colors;
		arrays[(int)Mesh.ArrayType.Index] = _terrainWoundIndices;
		var arrMesh = new ArrayMesh();
		arrMesh.AddSurfaceFromArrays(Mesh.PrimitiveType.Triangles, arrays);
		return new MeshInstance3D
		{
			Name = layerName,
			Mesh = arrMesh,
			MaterialOverride = CreateTerrainMapMaterial(),
			Visible = visible
		};
	}

	private static bool IsCtrlKey(InputEventKey key)
	{
		return key.CtrlPressed || key.Keycode == Key.Ctrl;
	}

	private static bool IsCameraMotionKey(Key keycode)
	{
		return keycode is Key.Up or Key.Down or Key.Left or Key.Right
			or Key.W or Key.A or Key.S or Key.D;
	}

	private static bool IsZoomKey(Key keycode)
	{
		return keycode is Key.Up or Key.Down or Key.W or Key.S;
	}

	public override void _Input(InputEvent @event)
	{
		if (@event is InputEventKey keyEvent && keyEvent.Pressed && !keyEvent.Echo)
		{
			if (IsCtrlKey(keyEvent) || IsCameraMotionKey(keyEvent.Keycode))
				GetViewport().GuiReleaseFocus();
		}

		if (@event is not InputEventKey motionKey || !motionKey.Pressed || !IsCameraMotionKey(motionKey.Keycode))
			return;

		bool shift = Input.IsKeyPressed(Key.Shift);
		if ((shift && IsZoomKey(motionKey.Keycode)) || !shift)
			GetViewport().SetInputAsHandled();
	}

	public override void _Process(double delta)
	{
		Vector2 viewportSize = GetViewport().GetVisibleRect().Size;
		if (_referenceViewportSize != viewportSize)
		{
			AdaptCameraToViewportSize(viewportSize, _referenceViewportSize);
			_referenceViewportSize = viewportSize;
		}

		if (_camera == null || _focus == null)
			return;

		float dt = (float)delta;
		bool shift = Input.IsKeyPressed(Key.Shift);

		if (shift)
		{
			float dolly = 0f;
			if (Input.IsKeyPressed(Key.W) || Input.IsKeyPressed(Key.Up)) dolly -= 1f;
			if (Input.IsKeyPressed(Key.S) || Input.IsKeyPressed(Key.Down)) dolly += 1f;
			if (dolly != 0f)
				DollyCamera(dolly * DollySpeed * dt);
		}
		else
		{
			float yaw = 0f;
			float pitch = 0f;
			if (Input.IsKeyPressed(Key.A) || Input.IsKeyPressed(Key.Left)) yaw += 1f;
			if (Input.IsKeyPressed(Key.D) || Input.IsKeyPressed(Key.Right)) yaw -= 1f;
			if (Input.IsKeyPressed(Key.W) || Input.IsKeyPressed(Key.Up)) pitch += 1f;
			if (Input.IsKeyPressed(Key.S) || Input.IsKeyPressed(Key.Down)) pitch -= 1f;
			if (yaw != 0f || pitch != 0f)
			{
				Vector3 pivot = GlobalPosition;
				if (yaw != 0f)
					OrbitFocusAround(pivot, Vector3.Up, yaw * OrbitSpeed * dt);
				if (pitch != 0f)
					OrbitFocusAround(pivot, _camera.GlobalTransform.Basis.X, pitch * OrbitSpeed * dt);
			}
		}
	}

	private void OrbitFocusAround(Vector3 pivot, Vector3 axis, float angleRad)
	{
		if (_focus == null || axis.LengthSquared() < 1e-8f || Mathf.Abs(angleRad) < 1e-8f)
			return;
		axis = axis.Normalized();
		var rot = new Basis(axis, angleRad);
		_focus.GlobalPosition = pivot + rot * (_focus.GlobalPosition - pivot);
		_focus.GlobalBasis = rot * _focus.GlobalBasis;
	}

	private void DollyCamera(float amount)
	{
		Vector3 forward = -_camera.GlobalTransform.Basis.Z;
		Vector3 next = _camera.GlobalPosition + forward * amount;
		Vector3 anchor = _focus.GlobalPosition;
		float dist = next.DistanceTo(anchor);
		if (dist < MinCameraDistance || dist > MaxCameraDistance)
		{
			next = anchor + (next - anchor).Normalized() * Mathf.Clamp(dist, MinCameraDistance, MaxCameraDistance);
		}
		_camera.GlobalPosition = next;
	}

	private void AdaptCameraToViewportSize(Vector2 size, Vector2 previousSize)
	{
		if (_camera == null || _focus == null)
			return;
		if (previousSize.X <= 0f || previousSize.Y <= 0f || size.X <= 0f || size.Y <= 0f)
			return;

		float scale = Mathf.Max(size.X / previousSize.X, size.Y / previousSize.Y);
		if (Mathf.Abs(scale - 1f) < 0.001f)
			return;

		// Keep apparent planet size when the window grows or shrinks.
		Vector3 toCamera = _camera.GlobalPosition - _focus.GlobalPosition;
		float dist = toCamera.Length();
		if (dist < 0.001f)
			return;
		float targetDist = Mathf.Clamp(dist / scale, MinCameraDistance, MaxCameraDistance);
		_camera.GlobalPosition = _focus.GlobalPosition + toCamera.Normalized() * targetDist;
	}

	private void InitPreviewCamera()
	{
		Node main = GetParent();
		if (main == null) return;
		_focus = main.GetNodeOrNull<Node3D>("Focus");
		_camera = main.GetNodeOrNull<Camera3D>("Focus/Camera3D");
		if (_focus == null || _camera == null)
		{
			CallDeferred(MethodName.InitPreviewCamera);
			return;
		}
		if (!_camera.Current)
			_camera.Current = true;
		// Start framed on the planet, but pan/dolly never re-centre with LookAt.
		_focus.GlobalPosition = Vector3.Zero;
	}

	private void BuildSphere()
	{
		var meshInstance = new MeshInstance3D { Name = "Globe" };
		var sphereMesh = new SphereMesh
		{
			Radius = SphereRadius,
			Height = SphereRadius * 2.0f
		};
		var mat = new StandardMaterial3D
		{
			AlbedoColor = new Color(0.2f, 0.35f, 0.6f, 0.5f),
			Transparency = BaseMaterial3D.TransparencyEnum.Alpha,
			ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded,
			CullMode = BaseMaterial3D.CullModeEnum.Disabled
		};
		sphereMesh.Material = mat;
		meshInstance.Mesh = sphereMesh;
		AddChild(meshInstance);
	}

	private void BuildPoints(Vector3[] sites)
	{
		var existing = GetNodeOrNull<MultiMeshInstance3D>("Sites");
		if (existing != null)
			existing.QueueFree();

		// Each Fibonacci point is its own little sphere; one contrasting color so they read clearly.
		float pointScale = PointRadius * 2.0f;
		var basis = new Basis(new Vector3(pointScale, 0, 0), new Vector3(0, pointScale, 0), new Vector3(0, 0, pointScale));

		var multiMesh = new MultiMesh
		{
			TransformFormat = MultiMesh.TransformFormatEnum.Transform3D,
			Mesh = MakePointMesh(),
			InstanceCount = sites.Length
		};
		for (int i = 0; i < sites.Length; i++)
		{
			Vector3 p = sites[i] * SphereRadius;
			multiMesh.SetInstanceTransform(i, new Transform3D(basis, p));
		}
		var pointColor = new Color(1.0f, 0.75f, 0.2f);  // Warm orange, distinct from blue globe
		var multiInstance = new MultiMeshInstance3D
		{
			Name = "Sites",
			Multimesh = multiMesh,
			MaterialOverride = new StandardMaterial3D { AlbedoColor = pointColor }
		};
		AddChild(multiInstance);
	}

	private static Mesh MakePointMesh()
	{
		return new SphereMesh { Radius = 0.5f, Height = 1.0f };
	}

	private void BuildDelaunayLayer()
	{
		if (_sites == null || _triangles == null || _triangles.Length < 3) return;
		var existing = GetNodeOrNull<Node3D>("Delaunay");
		if (existing != null)
			existing.QueueFree();

		int siteCount = _sites.Length;
		int maxIndex = siteCount - 1;
		int capIndex = siteCount - 1; // Cap (hole-fill) vertex is last site when present

		// Unique edges from triangles; only include edges whose indices are valid for _sites.
		GD.Print("[SpherePreview] BuildDelaunayLayer: ", _triangles.Length / 3, " triangles, ", siteCount, " sites.");
		var edgeSet = new System.Collections.Generic.HashSet<(int, int)>();
		int invalidCount = 0;
		int capTriCount = 0;
		int capTriSkipped = 0;
		for (int i = 0; i < _triangles.Length; i += 3)
		{
			int a = _triangles[i], b = _triangles[i + 1], c = _triangles[i + 2];
			bool referencesCap = (a == capIndex || b == capIndex || c == capIndex);
			if (referencesCap) capTriCount++;
			if (a < 0 || a > maxIndex || b < 0 || b > maxIndex || c < 0 || c > maxIndex)
			{
				invalidCount++;
				if (referencesCap) capTriSkipped++;
				continue;
			}
			AddEdge(edgeSet, a, b);
			AddEdge(edgeSet, b, c);
			AddEdge(edgeSet, c, a);
		}
		if (capTriCount > 0)
			GD.Print("[SpherePreview] Cap triangulation: ", capTriCount, " triangles reference cap index ", capIndex, " (skipped ", capTriSkipped, ").");
		if (invalidCount > 0)
			GD.PushWarning($"[SpherePreview] Skipped {invalidCount} triangles with out-of-range indices (sites length {siteCount}).");

		// Draw wireframe as explicit line segments so geometry exactly matches computed mesh.
		// Same scale as points: unit-sphere sites * SphereRadius.
		var imm = new ImmediateMesh();
		imm.SurfaceBegin(Mesh.PrimitiveType.Lines);
		var mat = new StandardMaterial3D
		{
			AlbedoColor = new Color(0.35f, 0.95f, 0.5f),
			ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded
		};
		foreach (var (ia, ib) in edgeSet)
		{
			Vector3 a = _sites[ia] * SphereRadius;
			Vector3 b = _sites[ib] * SphereRadius;
			imm.SurfaceSetColor(mat.AlbedoColor);
			imm.SurfaceAddVertex(a);
			imm.SurfaceAddVertex(b);
		}
		imm.SurfaceEnd();

		var container = new Node3D { Name = "Delaunay" };
		var meshInstance = new MeshInstance3D
		{
			Mesh = imm,
			MaterialOverride = mat
		};
		container.AddChild(meshInstance);
		AddChild(container);
		container.Visible = true;
		GD.Print("[SpherePreview] Delaunay layer added: ", edgeSet.Count, " edges (lines).");
	}

	private static void AddEdge(System.Collections.Generic.HashSet<(int, int)> set, int a, int b)
	{
		if (a > b) (a, b) = (b, a);
		set.Add((a, b));
	}

	/// <summary>One small dot at each triangle circumcenter (centre of each Voronoi region), matching reference style.</summary>
	private void BuildCircumcentersLayer()
	{
		if (_circumcenters == null || _circumcenters.Length == 0) return;
		var existing = GetNodeOrNull<Node3D>("Circumcenters");
		if (existing != null)
			existing.QueueFree();

		// Small dot radius so they read as centres of each region (smaller than site points)
		const float CircumcenterDotRadius = 0.08f;
		float pointScale = CircumcenterDotRadius * 2.0f;
		var basis = new Basis(new Vector3(pointScale, 0, 0), new Vector3(0, pointScale, 0), new Vector3(0, 0, pointScale));

		var multiMesh = new MultiMesh
		{
			TransformFormat = MultiMesh.TransformFormatEnum.Transform3D,
			Mesh = MakePointMesh(),
			InstanceCount = _circumcenters.Length
		};
		for (int i = 0; i < _circumcenters.Length; i++)
		{
			Vector3 p = PresentationCoords.SimToGodot(_circumcenters[i]) * SphereRadius;
			multiMesh.SetInstanceTransform(i, new Transform3D(basis, p));
		}
		var dotColor = new Color(0.05f, 0.05f, 0.08f);  // Near-black, like reference
		var multiInstance = new MultiMeshInstance3D
		{
			Multimesh = multiMesh,
			MaterialOverride = new StandardMaterial3D { AlbedoColor = dotColor }
		};
		var container = new Node3D { Name = "Circumcenters", Visible = false };
		container.AddChild(multiInstance);
		AddChild(container);
		GD.Print("[SpherePreview] Circumcenters layer added: ", _circumcenters.Length, " dots (hidden until layer toggle).");
	}

	/// <summary>Voronoi regions: for each site, polygon formed by connecting circumcenters of triangles touching that site.</summary>
	private void BuildVoronoiRegionsLayer()
	{
		if (_circumcenters == null || _cells == null || _circumcenters.Length == 0 || _cells.Length == 0) return;
		var existing = GetNodeOrNull<Node3D>("Voronoi");
		if (existing != null)
			existing.QueueFree();

		var verts = new Vector3[_circumcenters.Length];
		for (int i = 0; i < _circumcenters.Length; i++)
			verts[i] = PresentationCoords.SimToGodot(_circumcenters[i]) * SphereRadius;

		int maxIdx = verts.Length - 1;
		int edgeCount = 0;
		var imm = new ImmediateMesh();
		imm.SurfaceBegin(Mesh.PrimitiveType.Lines);
		var mat = new StandardMaterial3D
		{
			AlbedoColor = new Color(0.95f, 0.5f, 0.35f),
			ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded
		};
		foreach (var cell in _cells)
		{
			if (cell == null || cell.Length < 2) continue;
			for (int j = 0; j < cell.Length; j++)
			{
				int idxA = cell[j];
				int idxB = cell[(j + 1) % cell.Length];
				if (idxA < 0 || idxA > maxIdx || idxB < 0 || idxB > maxIdx) continue;
				imm.SurfaceSetColor(mat.AlbedoColor);
				imm.SurfaceAddVertex(verts[idxA]);
				imm.SurfaceAddVertex(verts[idxB]);
				edgeCount++;
			}
		}
		imm.SurfaceEnd();

		var container = new Node3D { Name = "Voronoi" };
		container.AddChild(new MeshInstance3D
		{
			Mesh = imm,
			MaterialOverride = mat
		});
		AddChild(container);
		container.Visible = true;
		GD.Print("[SpherePreview] Voronoi regions layer added: ", edgeCount, " edges (cells=", _cells.Length, ").");
	}

	private Color[] BuildPlateTerrainColors(int vertexCount, int indexCount)
	{
		int numRegions = _plateRegions?.Length ?? 0;
		if (numRegions == 0 || vertexCount == 0 || indexCount < 3 || _planetTerrainMeshIndices == null)
			return null;

		var colors = new Color[vertexCount];
		int paintedTris = 0;
		for (int t = 0; t + 2 < indexCount; t += 3)
		{
			int i0 = _planetTerrainMeshIndices[t];
			int i1 = _planetTerrainMeshIndices[t + 1];
			int i2 = _planetTerrainMeshIndices[t + 2];
			int plateId = PlateIdForTerrainTriangle(i0, i1, i2, numRegions, _plateRegions);
			if (plateId < 0)
				continue;
			Color col = PlatePaletteColor(plateId);
			if (i0 >= 0 && i0 < vertexCount) colors[i0] = col;
			if (i1 >= 0 && i1 < vertexCount) colors[i1] = col;
			if (i2 >= 0 && i2 < vertexCount) colors[i2] = col;
			paintedTris++;
		}
		return paintedTris > 0 ? colors : null;
	}

	private static Color PlatePaletteColor(int plateId)
	{
		ReadOnlySpan<Color> palette = stackalloc Color[]
		{
			new(0.85f, 0.35f, 0.25f), new(0.35f, 0.65f, 0.85f), new(0.55f, 0.75f, 0.35f),
			new(0.9f, 0.7f, 0.2f), new(0.6f, 0.4f, 0.75f), new(0.3f, 0.8f, 0.8f),
			new(0.95f, 0.5f, 0.5f), new(0.4f, 0.5f, 0.9f), new(0.7f, 0.85f, 0.45f),
			new(0.8f, 0.6f, 0.2f), new(0.5f, 0.35f, 0.7f), new(0.25f, 0.7f, 0.7f),
			new(0.9f, 0.4f, 0.4f), new(0.45f, 0.6f, 0.9f), new(0.65f, 0.8f, 0.3f),
			new(0.75f, 0.65f, 0.25f), new(0.55f, 0.45f, 0.8f), new(0.35f, 0.75f, 0.65f),
			new(0.8f, 0.45f, 0.45f), new(0.5f, 0.55f, 0.85f), new(0.7f, 0.9f, 0.5f),
			new(0.85f, 0.55f, 0.15f), new(0.6f, 0.5f, 0.75f), new(0.4f, 0.85f, 0.8f),
			new(0.75f, 0.3f, 0.3f), new(0.4f, 0.7f, 0.95f), new(0.6f, 0.8f, 0.4f),
			new(0.9f, 0.75f, 0.35f), new(0.65f, 0.4f, 0.7f), new(0.3f, 0.65f, 0.6f),
			new(0.95f, 0.55f, 0.5f), new(0.55f, 0.6f, 0.9f), new(0.75f, 0.85f, 0.45f),
			new(0.8f, 0.5f, 0.2f), new(0.5f, 0.55f, 0.8f), new(0.45f, 0.8f, 0.7f),
			new(0.85f, 0.4f, 0.35f), new(0.45f, 0.65f, 0.85f), new(0.65f, 0.75f, 0.35f),
			new(0.9f, 0.65f, 0.25f), new(0.6f, 0.45f, 0.75f), new(0.35f, 0.7f, 0.75f),
			new(0.8f, 0.5f, 0.5f), new(0.5f, 0.5f, 0.9f), new(0.7f, 0.8f, 0.5f),
			new(0.75f, 0.6f, 0.2f), new(0.55f, 0.5f, 0.7f), new(0.4f, 0.8f, 0.65f),
			new(0.9f, 0.45f, 0.4f), new(0.4f, 0.6f, 0.9f), new(0.6f, 0.85f, 0.4f),
			new(0.85f, 0.7f, 0.3f), new(0.65f, 0.4f, 0.65f)
		};
		if (plateId < 0)
			return Colors.Transparent;
		return palette[plateId % palette.Length];
	}

	private static int PlateIdForRegionVertex(int vertexIndex, int numRegions, int[] plateRegions)
	{
		if (vertexIndex < 0 || vertexIndex >= numRegions || plateRegions == null)
			return -1;
		return plateRegions[vertexIndex];
	}

	private static int PlateIdForTerrainTriangle(int i0, int i1, int i2, int numRegions, int[] plateRegions)
	{
		int p = PlateIdForRegionVertex(i0, numRegions, plateRegions);
		if (p >= 0)
			return p;
		p = PlateIdForRegionVertex(i1, numRegions, plateRegions);
		if (p >= 0)
			return p;
		return PlateIdForRegionVertex(i2, numRegions, plateRegions);
	}

	/// <summary>Fallback when plate_regions arrive after terrain mesh was already built.</summary>
	private bool BuildTectonicPlatesTerrainMeshLayer()
	{
		if (_planetTerrainMeshVertices == null || _planetTerrainMeshIndices == null)
			return false;
		if (GetNodeOrNull<Node3D>("TectonicPlates") != null)
			return true;

		EnsureTerrainMeshCache();
		if (_terrainWoundIndices == null || _terrainWoundIndices.Length < 3)
			return false;

		int nV = _planetTerrainMeshVertices.Length;
		int nI = _planetTerrainMeshIndices.Length;
		Color[] colors = BuildPlateTerrainColors(nV, nI);
		if (colors == null)
			return false;

		var container = new Node3D { Name = "TectonicPlates", Visible = false };
		container.AddChild(CreateTerrainTriangleLayer(_terrainDisplacedVerts, _terrainDisplacedNormals, colors, "Surface", visible: true));
		AddChild(container);
		GD.Print("[SpherePreview] Tectonic plates (terrain mesh) added after plate_regions.");
		return true;
	}

	/// <summary>Build plate-coloured debug layers when Voronoi cells or terrain mesh data is available.</summary>
	/// <returns>True if at least the tectonic plates layer was built.</returns>
	private bool TryRebuildPlateLayers()
	{
		if (_plateRegions == null || _plateRegions.Length == 0)
			return false;

		// Voronoi circumcenter layers sit slightly outside the watertight terrain shell and use
		// double-sided materials — they read as hairy clipping if left visible on top of it.
		if (HasPlanetTerrainMeshData())
			return BuildTectonicPlatesTerrainMeshLayer();

		if (_circumcenters != null && _cells != null && _cells.Length > 0
		    && _plateRegions.Length >= _cells.Length)
		{
			BuildTectonicPlatesLayer();
			if (_plateElevation != null && _plateElevation.Length > 0)
				BuildElevationLayer();
			if (_plateMoisture != null && _plateMoisture.Length > 0)
				BuildMoistureLayer();
			if (_plateElevation != null && _plateMoisture != null && _plateElevation.Length > 0 && _plateMoisture.Length > 0)
				BuildPlateLabelsLayer();
			return true;
		}

		if (GetNodeOrNull<Node3D>("TectonicPlates") != null)
			return true;
		return BuildTectonicPlatesTerrainMeshLayer();
	}

	/// <summary>Tectonic plates: each Voronoi cell filled with a colour by plate id (palette of up to 50).</summary>
	private void BuildTectonicPlatesLayer()
	{
		if (_circumcenters == null || _cells == null || _plateRegions == null || _cells.Length > _plateRegions.Length)
		{
			GD.Print("[SpherePreview] BuildTectonicPlatesLayer: skip (missing data or _cells.Length ", _cells?.Length ?? 0, " > _plateRegions.Length ", _plateRegions?.Length ?? 0, ")");
			return;
		}
		var existing = GetNodeOrNull<Node3D>("TectonicPlates");
		if (existing != null)
			existing.QueueFree();

		// Slightly outside sphere so plates render on top of the globe (avoid z-fight / occlusion).
		const float plateRadiusScale = 1.002f;

		var verts = new Vector3[_circumcenters.Length];
		for (int i = 0; i < _circumcenters.Length; i++)
			verts[i] = PresentationCoords.SimToGodot(_circumcenters[i]) * (SphereRadius * plateRadiusScale);

		var imm = new ImmediateMesh();
		imm.SurfaceBegin(Mesh.PrimitiveType.Triangles);
		int maxIdx = verts.Length - 1;
		int triCount = 0;
		for (int site = 0; site < _cells.Length && site < _plateRegions.Length; site++)
		{
			var cell = _cells[site];
			if (cell == null || cell.Length < 3) continue;
			int plateId = _plateRegions[site];
			if (plateId < 0) continue;
			var col = PlatePaletteColor(plateId);
			for (int j = 1; j < cell.Length - 1; j++)
			{
				int i0 = cell[0], i1 = cell[j], i2 = cell[j + 1];
				if (i0 < 0 || i0 > maxIdx || i1 < 0 || i1 > maxIdx || i2 < 0 || i2 > maxIdx) continue;
				imm.SurfaceSetColor(col);
				imm.SurfaceAddVertex(verts[i0]);
				imm.SurfaceSetColor(col);
				imm.SurfaceAddVertex(verts[i1]);
				imm.SurfaceSetColor(col);
				imm.SurfaceAddVertex(verts[i2]);
				triCount++;
			}
		}
		imm.SurfaceEnd();

		var mat = new StandardMaterial3D
		{
			ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded,
			Transparency = BaseMaterial3D.TransparencyEnum.Disabled,
			VertexColorUseAsAlbedo = true,
			CullMode = BaseMaterial3D.CullModeEnum.Disabled
		};
		var container = new Node3D { Name = "TectonicPlates" };
		container.AddChild(new MeshInstance3D
		{
			Mesh = imm,
			MaterialOverride = mat
		});
		AddChild(container);
		container.Visible = true;
		GD.Print("[SpherePreview] Tectonic plates layer added: triangles=", triCount, " cells=", _cells.Length, " radius_scale=", plateRadiusScale);
	}

	/// <summary>Elevation layer: colour by plate elevation (ocean blue, land green→brown→grey).</summary>
	private void BuildElevationLayer()
	{
		if (_circumcenters == null || _cells == null || _plateRegions == null || _plateElevation == null || _plateElevation.Length == 0)
			return;
		var existing = GetNodeOrNull<Node3D>("Elevation");
		if (existing != null)
			existing.QueueFree();

		const float layerRadiusScale = 1.003f;
		var verts = new Vector3[_circumcenters.Length];
		for (int i = 0; i < _circumcenters.Length; i++)
			verts[i] = PresentationCoords.SimToGodot(_circumcenters[i]) * (SphereRadius * layerRadiusScale);

		var deepBlue = new Color(0.05f, 0.15f, 0.45f);
		var shallowBlue = new Color(0.35f, 0.55f, 0.85f);
		var green = new Color(0.2f, 0.6f, 0.25f);
		var brown = new Color(0.45f, 0.35f, 0.2f);
		var grey = new Color(0.5f, 0.5f, 0.5f);

		var imm = new ImmediateMesh();
		imm.SurfaceBegin(Mesh.PrimitiveType.Triangles);
		int maxIdx = verts.Length - 1;
		int triCount = 0;
		for (int site = 0; site < _cells.Length && site < _plateRegions.Length; site++)
		{
			var cell = _cells[site];
			if (cell == null || cell.Length < 3) continue;
			int plateId = _plateRegions[site];
			if (plateId < 0 || plateId >= _plateElevation.Length) continue;
			float elev = _plateElevation[plateId];
			Color col;
			if (elev < 0f)
			{
				float t = Mathf.Clamp((elev + 1f), 0f, 1f);
				col = deepBlue.Lerp(shallowBlue, t);
			}
			else
			{
				float t = Mathf.Clamp(elev, 0f, 1f);
				col = t < 0.5f ? green.Lerp(brown, t * 2f) : brown.Lerp(grey, (t - 0.5f) * 2f);
			}
			for (int j = 1; j < cell.Length - 1; j++)
			{
				int i0 = cell[0], i1 = cell[j], i2 = cell[j + 1];
				if (i0 < 0 || i0 > maxIdx || i1 < 0 || i1 > maxIdx || i2 < 0 || i2 > maxIdx) continue;
				imm.SurfaceSetColor(col);
				imm.SurfaceAddVertex(verts[i0]);
				imm.SurfaceSetColor(col);
				imm.SurfaceAddVertex(verts[i1]);
				imm.SurfaceSetColor(col);
				imm.SurfaceAddVertex(verts[i2]);
				triCount++;
			}
		}
		imm.SurfaceEnd();

		var mat = new StandardMaterial3D
		{
			ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded,
			Transparency = BaseMaterial3D.TransparencyEnum.Disabled,
			VertexColorUseAsAlbedo = true,
			CullMode = BaseMaterial3D.CullModeEnum.Disabled
		};
		var container = new Node3D { Name = "Elevation" };
		container.AddChild(new MeshInstance3D { Mesh = imm, MaterialOverride = mat });
		AddChild(container);
		container.Visible = true;
		GD.Print("[SpherePreview] Elevation layer added: triangles=", triCount);
	}

	/// <summary>Moisture layer: colour by plate moisture (0 = dry brown, 1 = wet green).</summary>
	private void BuildMoistureLayer()
	{
		if (_circumcenters == null || _cells == null || _plateRegions == null || _plateMoisture == null || _plateMoisture.Length == 0)
			return;
		var existing = GetNodeOrNull<Node3D>("Moisture");
		if (existing != null)
			existing.QueueFree();

		const float layerRadiusScale = 1.004f;
		var verts = new Vector3[_circumcenters.Length];
		for (int i = 0; i < _circumcenters.Length; i++)
			verts[i] = PresentationCoords.SimToGodot(_circumcenters[i]) * (SphereRadius * layerRadiusScale);

		var dryColor = new Color(0.55f, 0.4f, 0.25f);
		var wetColor = new Color(0.2f, 0.6f, 0.3f);

		var imm = new ImmediateMesh();
		imm.SurfaceBegin(Mesh.PrimitiveType.Triangles);
		int maxIdx = verts.Length - 1;
		int triCount = 0;
		for (int site = 0; site < _cells.Length && site < _plateRegions.Length; site++)
		{
			var cell = _cells[site];
			if (cell == null || cell.Length < 3) continue;
			int plateId = _plateRegions[site];
			if (plateId < 0 || plateId >= _plateMoisture.Length) continue;
			float m = Mathf.Clamp(_plateMoisture[plateId], 0f, 1f);
			Color col = dryColor.Lerp(wetColor, m);
			for (int j = 1; j < cell.Length - 1; j++)
			{
				int i0 = cell[0], i1 = cell[j], i2 = cell[j + 1];
				if (i0 < 0 || i0 > maxIdx || i1 < 0 || i1 > maxIdx || i2 < 0 || i2 > maxIdx) continue;
				imm.SurfaceSetColor(col);
				imm.SurfaceAddVertex(verts[i0]);
				imm.SurfaceSetColor(col);
				imm.SurfaceAddVertex(verts[i1]);
				imm.SurfaceSetColor(col);
				imm.SurfaceAddVertex(verts[i2]);
				triCount++;
			}
		}
		imm.SurfaceEnd();

		var mat = new StandardMaterial3D
		{
			ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded,
			Transparency = BaseMaterial3D.TransparencyEnum.Disabled,
			VertexColorUseAsAlbedo = true,
			CullMode = BaseMaterial3D.CullModeEnum.Disabled
		};
		var container = new Node3D { Name = "Moisture" };
		container.AddChild(new MeshInstance3D { Mesh = imm, MaterialOverride = mat });
		AddChild(container);
		container.Visible = true;
		GD.Print("[SpherePreview] Moisture layer added: triangles=", triCount);
	}

	/// <summary>Per-plate labels: elevation and moisture numbers at the centre of each plate.</summary>
	private void BuildPlateLabelsLayer()
	{
		if (_sites == null || _plateRegions == null || _plateElevation == null || _plateMoisture == null ||
			_plateElevation.Length == 0 || _plateMoisture.Length == 0)
			return;
		var existing = GetNodeOrNull<Node3D>("PlateLabels");
		if (existing != null)
			existing.QueueFree();

		const float labelRadiusScale = 1.008f;
		int numPlates = Mathf.Min(_plateElevation.Length, _plateMoisture.Length);
		var container = new Node3D { Name = "PlateLabels" };

		for (int plateId = 0; plateId < numPlates; plateId++)
		{
			Vector3 sum = Vector3.Zero;
			int count = 0;
			for (int i = 0; i < _plateRegions.Length && i < _sites.Length; i++)
			{
				if (_plateRegions[i] == plateId)
				{
					sum += _sites[i];
					count++;
				}
			}
			if (count == 0) continue;
			Vector3 dir = (sum / count).Normalized();
			if (dir.LengthSquared() < 0.0001f) continue;
			Vector3 pos = dir * (SphereRadius * labelRadiusScale);

			float elev = _plateElevation[plateId];
			float moist = _plateMoisture[plateId];
			var lightBlue = new Color(0.6f, 0.85f, 1.0f);

			var elevLabel = new Label3D
			{
				Text = elev.ToString("F2"),
				FontSize = 48,
				PixelSize = 0.025f,
				OutlineSize = 12,
				Modulate = Colors.White,
				Billboard = BaseMaterial3D.BillboardModeEnum.Enabled,
				Position = pos
			};
			container.AddChild(elevLabel);

			var moistLabel = new Label3D
			{
				Text = moist.ToString("F2"),
				FontSize = 48,
				PixelSize = 0.025f,
				OutlineSize = 12,
				Modulate = lightBlue,
				Billboard = BaseMaterial3D.BillboardModeEnum.Enabled,
				Position = pos + new Vector3(0f, -0.5f, 0f)
			};
			container.AddChild(moistLabel);
		}

		AddChild(container);
		container.Visible = true;
		GD.Print("[SpherePreview] Plate labels added: ", container.GetChildCount(), " plates");
	}

	private void ClearWorldTerrain()
	{
		Node main = GetParent();
		if (main == null) return;
		Node worldRoot = main.GetNodeOrNull("WorldRoot");
		if (worldRoot == null) return;
		foreach (Node child in worldRoot.GetChildren())
			child.QueueFree();
	}
}
