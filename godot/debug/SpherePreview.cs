using Godot;
using System;

/// <summary>
/// Visualises the Voronoi sphere: unit sphere + Fibonacci sites as points.
/// Sim backend uses Z-up (north pole +Z); we convert to Godot Y-up when storing sites.
/// Arrow keys / WASD: orbit. Shift + Arrow/WASD: zoom in/out.
/// </summary>
public partial class SpherePreview : Node3D
{
	/// <summary>Convert sim coords (Z-up, unit sphere) to Godot (Y-up).</summary>
	private static Vector3 SimToGodot(Vector3 sim)
	{
		return new Vector3(sim.X, sim.Z, sim.Y);
	}
	private const float PointRadius = 0.55f;  // Radius of each Fibonacci point sphere
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

		// Unique edges from triangles; only include edges whose indices are valid for _sites.
		GD.Print("[SpherePreview] BuildDelaunayLayer: ", _triangles.Length / 3, " triangles, ", siteCount, " sites.");
		var edgeSet = new System.Collections.Generic.HashSet<(int, int)>();
		int invalidCount = 0;
		for (int i = 0; i < _triangles.Length; i += 3)
		{
			int a = _triangles[i], b = _triangles[i + 1], c = _triangles[i + 2];
			if (a < 0 || a > maxIndex || b < 0 || b > maxIndex || c < 0 || c > maxIndex)
			{
				invalidCount++;
				continue;
			}
			AddEdge(edgeSet, a, b);
			AddEdge(edgeSet, b, c);
			AddEdge(edgeSet, c, a);
		}
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
