using Godot;
using System;

/// <summary>
/// Renders the Voronoi sphere from C++ backend data only. All logic (sites, Delaunay, circumcenters, Voronoi cells) is computed in C++; this layer just draws what it receives (sites, triangles, circumcenters, cells).
/// Sim backend uses Z-up (north pole +Z); we convert to Godot Y-up for display.
/// Arrow keys / WASD: orbit. Shift + Arrow/WASD: zoom in/out.
/// </summary>
public partial class SpherePreview : Node3D
{
	/// <summary>Convert sim coords (Z-up, unit sphere) to Godot (Y-up).</summary>
	private static Vector3 SimToGodot(Vector3 sim)
	{
		return new Vector3(sim.X, sim.Z, sim.Y);
	}
	private const float PointRadius = 0.38f;  // Radius of each Fibonacci point sphere
	private const float SphereRadius = 15.0f;
	private const float OrbitSpeed = 1.5f;
	private const float ZoomSpeed = 8.0f;
	private const float MinZoom = 10.0f;
	private const float MaxZoom = 80.0f;

	private float _orbitYaw;
	private float _orbitPitch;
	private float _zoomDistance;
	private Camera3D _camera;
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
	private bool _showPlanetTerrainMeshOnly;

	public override void _Ready()
	{
		ClearWorldTerrain();
		BuildSphere();
		InitOrbitCamera();
		AddOverlay();
		GD.Print("[SpherePreview] ready. Call SetSites(sites) to show points. Left/Right=Y, Up/Down=X, Shift+WASD=zoom.");
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
		var overlay = scene.Instantiate<Control>();
		layer.AddChild(overlay);
	}

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
			_sites[i] = SimToGodot(sites[i]);
		BuildPoints(_sites);
		if (_triangles != null && _triangles.Length >= 3)
			BuildDelaunayLayer();
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
		if (_sites != null && triangles != null && triangles.Length >= 3)
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
			BuildCircumcentersLayer();
			if (_cells != null && _cells.Length > 0)
			{
				BuildVoronoiRegionsLayer();
				if (_plateElevation != null && _plateElevation.Length > 0)
					BuildElevationLayer();
				if (_plateMoisture != null && _plateMoisture.Length > 0)
					BuildMoistureLayer();
			}
		}
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
			BuildCircumcentersLayer();
			if (_cells != null && _cells.Length > 0)
			{
				BuildVoronoiRegionsLayer();
				if (_plateElevation != null && _plateElevation.Length > 0)
					BuildElevationLayer();
				if (_plateMoisture != null && _plateMoisture.Length > 0)
					BuildMoistureLayer();
			}
		}
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
		GD.Print("[SpherePreview] SetPlateRegions: regions=", plateRegions?.Length ?? 0,
			" circumcenters=", _circumcenters?.Length ?? 0, " cells=", _cells?.Length ?? 0);
		if (_circumcenters == null || _cells == null)
		{
			GD.Print("[SpherePreview] SetPlateRegions: skip build (circumcenters or cells not set yet)");
			return;
		}
		if (_plateRegions == null || _plateRegions.Length < _cells.Length)
		{
			GD.Print("[SpherePreview] SetPlateRegions: skip build (plateRegions.Length ", _plateRegions?.Length ?? 0, " < cells.Length ", _cells.Length, ")");
			return;
		}
		BuildTectonicPlatesLayer();
		if (_plateElevation != null && _plateElevation.Length > 0)
			BuildElevationLayer();
		if (_plateMoisture != null && _plateMoisture.Length > 0)
			BuildMoistureLayer();
		if (_plateElevation != null && _plateMoisture != null && _plateElevation.Length > 0 && _plateMoisture.Length > 0)
			BuildPlateLabelsLayer();
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

	/// <summary>Set the planet terrain mesh (quad + river/ridge). Vertices and normals in sim Z-up; we convert to Y-up and scale. Used for world streaming.</summary>
	public void SetPlanetTerrainMesh(Vector3[] vertices, Vector3[] normals, int[] indices, float[] riverStrength)
	{
		_planetTerrainMeshVertices = vertices;
		_planetTerrainMeshNormals = normals;
		_planetTerrainMeshIndices = indices;
		_planetTerrainMeshRiverStrength = riverStrength;
		BuildPlanetTerrainMeshLayer();
	}

	/// <summary>GDScript-callable; accepts result arrays from apply_world_gen_form (planet_terrain_mesh_vertices, planet_terrain_mesh_normals, planet_terrain_mesh_indices, planet_terrain_mesh_river_strength).</summary>
	public void set_planet_terrain_mesh(Variant vertices, Variant normals, Variant indices, Variant riverStrength)
	{
		GD.Print("[SpherePreview] set_planet_terrain_mesh called: vertices type=", vertices.VariantType, " normals type=", normals.VariantType, " indices type=", indices.VariantType);
		Vector3[] vArr = _variant_to_vector3_array(vertices);
		Vector3[] nArr = _variant_to_vector3_array(normals);
		int[] iArr = _variant_to_int_array(indices);
		float[] rArr = _variant_to_float_array(riverStrength);
		int vCount = vArr?.Length ?? 0;
		int iCount = iArr?.Length ?? 0;
		GD.Print("[SpherePreview] set_planet_terrain_mesh converted: vertices=", vCount, " indices=", iCount);
		if (vArr != null && vArr.Length > 0 && iArr != null && iArr.Length >= 3)
		{
			SetPlanetTerrainMesh(vArr, nArr, iArr, rArr);
			GD.Print("[SpherePreview] set_planet_terrain_mesh: SetPlanetTerrainMesh and BuildPlanetTerrainMeshLayer done.");
		}
		else
			GD.PushWarning("[SpherePreview] set_planet_terrain_mesh: invalid or empty vertices/indices (vertices=", vCount, " indices=", iCount, ").");
	}

	/// <summary>When true, only updates overlay so Planet terrain mesh is checked and others unchecked; layer nodes are hidden/shown by visibility, not destroyed.</summary>
	public void SetShowPlanetTerrainMeshOnly(bool only)
	{
		_showPlanetTerrainMeshOnly = only;
		var overlay = GetNodeOrNull<SpherePreviewOverlay>("OverlayLayer/SpherePreviewOverlay");
		if (overlay != null)
			overlay.SetShowPlanetTerrainMeshOnlyMode(only);
	}

	/// <summary>GDScript-callable.</summary>
	public void set_show_planet_terrain_mesh_only(bool only)
	{
		SetShowPlanetTerrainMeshOnly(only);
	}

	private static Vector3[] _variant_to_vector3_array(Variant v)
	{
		var arr = v.As<Vector3[]>();
		if (arr != null && arr.Length > 0) return arr;
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
		var arr = v.As<int[]>();
		if (arr != null && arr.Length > 0) return arr;
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
		if (v.VariantType == Variant.Type.Nil) return null;
		var arr = v.As<float[]>();
		if (arr != null && arr.Length > 0) return arr;
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

	private void BuildPlanetTerrainMeshLayer()
	{
		int nV = _planetTerrainMeshVertices?.Length ?? 0;
		int nI = _planetTerrainMeshIndices?.Length ?? 0;
		GD.Print("[SpherePreview] BuildPlanetTerrainMeshLayer: vertices=", nV, " indices=", nI);
		if (_planetTerrainMeshVertices == null || _planetTerrainMeshIndices == null || _planetTerrainMeshVertices.Length == 0 || _planetTerrainMeshIndices.Length < 3)
		{
			GD.Print("[SpherePreview] BuildPlanetTerrainMeshLayer: skip (insufficient data)");
			return;
		}
		var existing = GetNodeOrNull<MeshInstance3D>("PlanetTerrainMesh");
		if (existing != null)
		{
			GD.Print("[SpherePreview] BuildPlanetTerrainMeshLayer: removing existing PlanetTerrainMesh node");
			existing.QueueFree();
		}

		// Slightly outside globe so the planet terrain mesh reads as a solid 3D ground surface (avoids z-fight).
		const float PlanetTerrainMeshRadiusScale = 1.002f;
		nV = _planetTerrainMeshVertices.Length;
		var verts = new Vector3[nV];
		var normals = new Vector3[nV];
		for (int i = 0; i < nV; i++)
		{
			verts[i] = SimToGodot(_planetTerrainMeshVertices[i]) * (SphereRadius * PlanetTerrainMeshRadiusScale);
			normals[i] = _planetTerrainMeshNormals != null && i < _planetTerrainMeshNormals.Length
				? SimToGodot(_planetTerrainMeshNormals[i]).Normalized()
				: verts[i].Normalized();
		}
		int numTris = _planetTerrainMeshIndices.Length / 3;
		var colors = new Color[nV];
		float maxRiver = 0.001f;
		if (_planetTerrainMeshRiverStrength != null && _planetTerrainMeshRiverStrength.Length >= numTris)
		{
			for (int t = 0; t < numTris; t++)
				if (_planetTerrainMeshRiverStrength[t] > maxRiver) maxRiver = _planetTerrainMeshRiverStrength[t];
		}
		for (int i = 0; i < nV; i++)
			colors[i] = new Color(0.4f, 0.5f, 0.45f); // default terrain tint
		if (_planetTerrainMeshRiverStrength != null && _planetTerrainMeshRiverStrength.Length >= numTris && maxRiver > 0)
		{
			for (int t = 0; t < numTris; t++)
			{
				float r = _planetTerrainMeshRiverStrength[t] / maxRiver;
				int i0 = _planetTerrainMeshIndices[t * 3 + 0];
				int i1 = _planetTerrainMeshIndices[t * 3 + 1];
				int i2 = _planetTerrainMeshIndices[t * 3 + 2];
				var valley = new Color(0.2f + 0.4f * r, 0.35f, 0.6f);
				if (i0 < nV) colors[i0] = colors[i0].Lerp(valley, 0.5f);
				if (i1 < nV) colors[i1] = colors[i1].Lerp(valley, 0.5f);
				if (i2 < nV) colors[i2] = colors[i2].Lerp(valley, 0.5f);
			}
		}
		var arrays = new Godot.Collections.Array();
		arrays.Resize((int)Mesh.ArrayType.Max);
		arrays[(int)Mesh.ArrayType.Vertex] = verts;
		arrays[(int)Mesh.ArrayType.Normal] = normals;
		arrays[(int)Mesh.ArrayType.Color] = colors;
		arrays[(int)Mesh.ArrayType.Index] = _planetTerrainMeshIndices;
		var arrMesh = new ArrayMesh();
		arrMesh.AddSurfaceFromArrays(Mesh.PrimitiveType.Triangles, arrays);
		var mat = new StandardMaterial3D
		{
			ShadingMode = BaseMaterial3D.ShadingModeEnum.PerPixel,
			VertexColorUseAsAlbedo = true,
			CullMode = BaseMaterial3D.CullModeEnum.Disabled
		};
		var meshInstance = new MeshInstance3D
		{
			Name = "PlanetTerrainMesh",
			Mesh = arrMesh,
			MaterialOverride = mat,
			Visible = true
		};
		AddChild(meshInstance);
		meshInstance.Visible = true;
		GD.Print("[SpherePreview] Planet terrain mesh layer added: ", nV, " vertices, ", numTris, " triangles. Node name=", meshInstance.Name, " parent=", meshInstance.GetParent()?.Name ?? "null", " Visible=", meshInstance.Visible);
		var overlay = GetNodeOrNull<SpherePreviewOverlay>("OverlayLayer/SpherePreviewOverlay");
		if (overlay != null)
		{
			overlay.SyncPlanetTerrainMeshCheckbox();
			GD.Print("[SpherePreview] Planet terrain mesh: overlay SyncPlanetTerrainMeshCheckbox called.");
		}
		else
			GD.Print("[SpherePreview] Planet terrain mesh: overlay not found (OverlayLayer/SpherePreviewOverlay).");
	}

	public override void _Process(double delta)
	{
		float dt = (float)delta;
		bool shift = Input.IsKeyPressed(Key.Shift);

		if (shift)
		{
			// Zoom (Z): Shift + WASD/arrows.
			float zoom = 0f;
			if (Input.IsKeyPressed(Key.W) || Input.IsKeyPressed(Key.Up)) zoom -= 1f;
			if (Input.IsKeyPressed(Key.S) || Input.IsKeyPressed(Key.Down)) zoom += 1f;
			if (Input.IsKeyPressed(Key.A) || Input.IsKeyPressed(Key.Left)) zoom -= 1f;
			if (Input.IsKeyPressed(Key.D) || Input.IsKeyPressed(Key.Right)) zoom += 1f;
			if (zoom != 0f)
			{
				_zoomDistance += zoom * ZoomSpeed * dt;
				_zoomDistance = Mathf.Clamp(_zoomDistance, MinZoom, MaxZoom);
			}
		}
		else
		{
			float yaw = 0f;
			float pitch = 0f;
			if (Input.IsKeyPressed(Key.A) || Input.IsKeyPressed(Key.Left)) yaw += 1f;
			if (Input.IsKeyPressed(Key.D) || Input.IsKeyPressed(Key.Right)) yaw -= 1f;
			if (Input.IsKeyPressed(Key.W) || Input.IsKeyPressed(Key.Up)) pitch += 1f;
			if (Input.IsKeyPressed(Key.S) || Input.IsKeyPressed(Key.Down)) pitch -= 1f;
			if (yaw != 0f) _orbitYaw += yaw * OrbitSpeed * dt;
			if (pitch != 0f)
			{
				_orbitPitch += pitch * OrbitSpeed * dt;
				_orbitPitch = Mathf.Clamp(_orbitPitch, -Mathf.Pi / 2f + 0.1f, Mathf.Pi / 2f - 0.1f);
			}
		}

		ApplyOrbitCamera();
	}

	private void InitOrbitCamera()
	{
		Node main = GetParent();
		if (main == null) return;
		Node3D focus = main.GetNodeOrNull<Node3D>("Focus");
		if (focus != null)
			focus.GlobalPosition = Vector3.Zero;
		_camera = main.GetNodeOrNull<Camera3D>("Focus/Camera3D");
		if (_camera == null) return;
		Vector3 pos = _camera.GlobalPosition;
		_zoomDistance = pos.Length();
		if (_zoomDistance < 0.001f) _zoomDistance = 25f;
		_orbitYaw = Mathf.Atan2(pos.X, pos.Z);
		_orbitPitch = Mathf.Asin(Mathf.Clamp(pos.Y / _zoomDistance, -1f, 1f));
		_zoomDistance = Mathf.Clamp(_zoomDistance, MinZoom, MaxZoom);
		ApplyOrbitCamera();
	}

	private void ApplyOrbitCamera()
	{
		if (_camera == null) return;
		float x = _zoomDistance * Mathf.Cos(_orbitPitch) * Mathf.Sin(_orbitYaw);
		float y = _zoomDistance * Mathf.Sin(_orbitPitch);
		float z = _zoomDistance * Mathf.Cos(_orbitPitch) * Mathf.Cos(_orbitYaw);
		_camera.GlobalPosition = new Vector3(x, y, z);
		_camera.LookAt(Vector3.Zero, Vector3.Up);
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
			Vector3 p = SimToGodot(_circumcenters[i]) * SphereRadius;
			multiMesh.SetInstanceTransform(i, new Transform3D(basis, p));
		}
		var dotColor = new Color(0.05f, 0.05f, 0.08f);  // Near-black, like reference
		var multiInstance = new MultiMeshInstance3D
		{
			Multimesh = multiMesh,
			MaterialOverride = new StandardMaterial3D { AlbedoColor = dotColor }
		};
		var container = new Node3D { Name = "Circumcenters" };
		container.AddChild(multiInstance);
		AddChild(container);
		container.Visible = true;
		GD.Print("[SpherePreview] Circumcenters layer added: ", _circumcenters.Length, " dots.");
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
			verts[i] = SimToGodot(_circumcenters[i]) * SphereRadius;

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

		// Distinct colours per plate (repeat if num_plates > palette length).
		var palette = new Color[] {
			new Color(0.85f, 0.35f, 0.25f), new Color(0.35f, 0.65f, 0.85f), new Color(0.55f, 0.75f, 0.35f),
			new Color(0.9f, 0.7f, 0.2f), new Color(0.6f, 0.4f, 0.75f), new Color(0.3f, 0.8f, 0.8f),
			new Color(0.95f, 0.5f, 0.5f), new Color(0.4f, 0.5f, 0.9f), new Color(0.7f, 0.85f, 0.45f),
			new Color(0.8f, 0.6f, 0.2f), new Color(0.5f, 0.35f, 0.7f), new Color(0.25f, 0.7f, 0.7f),
			new Color(0.9f, 0.4f, 0.4f), new Color(0.45f, 0.6f, 0.9f), new Color(0.65f, 0.8f, 0.3f),
			new Color(0.75f, 0.65f, 0.25f), new Color(0.55f, 0.45f, 0.8f), new Color(0.35f, 0.75f, 0.65f),
			new Color(0.8f, 0.45f, 0.45f), new Color(0.5f, 0.55f, 0.85f), new Color(0.7f, 0.9f, 0.5f),
			new Color(0.85f, 0.55f, 0.15f), new Color(0.6f, 0.5f, 0.75f), new Color(0.4f, 0.85f, 0.8f),
			new Color(0.75f, 0.3f, 0.3f), new Color(0.4f, 0.7f, 0.95f), new Color(0.6f, 0.8f, 0.4f),
			new Color(0.9f, 0.75f, 0.35f), new Color(0.65f, 0.4f, 0.7f), new Color(0.3f, 0.65f, 0.6f),
			new Color(0.95f, 0.55f, 0.5f), new Color(0.55f, 0.6f, 0.9f), new Color(0.75f, 0.85f, 0.45f),
			new Color(0.8f, 0.5f, 0.2f), new Color(0.5f, 0.55f, 0.8f), new Color(0.45f, 0.8f, 0.7f),
			new Color(0.85f, 0.4f, 0.35f), new Color(0.45f, 0.65f, 0.85f), new Color(0.65f, 0.75f, 0.35f),
			new Color(0.9f, 0.65f, 0.25f), new Color(0.6f, 0.45f, 0.75f), new Color(0.35f, 0.7f, 0.75f),
			new Color(0.8f, 0.5f, 0.5f), new Color(0.5f, 0.5f, 0.9f), new Color(0.7f, 0.8f, 0.5f),
			new Color(0.75f, 0.6f, 0.2f), new Color(0.55f, 0.5f, 0.7f), new Color(0.4f, 0.8f, 0.65f),
			new Color(0.9f, 0.45f, 0.4f), new Color(0.4f, 0.6f, 0.9f), new Color(0.6f, 0.85f, 0.4f),
			new Color(0.85f, 0.7f, 0.3f), new Color(0.65f, 0.4f, 0.65f)
		};

		var verts = new Vector3[_circumcenters.Length];
		for (int i = 0; i < _circumcenters.Length; i++)
			verts[i] = SimToGodot(_circumcenters[i]) * (SphereRadius * plateRadiusScale);

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
			var col = palette[plateId % palette.Length];
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
			verts[i] = SimToGodot(_circumcenters[i]) * (SphereRadius * layerRadiusScale);

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
			verts[i] = SimToGodot(_circumcenters[i]) * (SphereRadius * layerRadiusScale);

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
