using Godot;

/// <summary>
/// Layer visibility toggles for SpherePreview. Terrain heatmaps live with diagnostic layers;
/// the terrain mesh section shows the watertight final surface and an optional rivers overlay.
/// </summary>
public partial class SpherePreviewOverlay : Control
{
	private const string LayersVBoxPath = "MarginContainer/Card/CardPadding/ScrollContainer/LayersVBox";
	private const string TerrainSectionPath = LayersVBoxPath + "/PlanetTerrainMeshSection";
	private const string TerrainHeaderPath = TerrainSectionPath + "/PlanetTerrainMeshHeaderRow";
	private const string TerrainSubVBoxPath = TerrainSectionPath + "/PlanetTerrainMeshSubVBox";

	private static readonly (string CheckSuffix, string NodeName)[] TerrainHeatmapLayers =
	{
		("TerrainTemperatureRow/TerrainTemperatureCheck", "TerrainTemperature"),
		("TerrainBiomeRow/TerrainBiomeCheck", "TerrainBiome"),
	};

	/// <summary>Standard diagnostic layers (Voronoi has a dedicated toggle handler).</summary>
	private static readonly (string CheckSuffix, string NodeName)[] DiagnosticLayers =
	{
		("SphereRow/SphereCheck", "Globe"),
		("PointsRow/PointsCheck", "Sites"),
		("DelaunayRow/DelaunayCheck", "Delaunay"),
		("CircumcentersRow/CircumcentersCheck", "Circumcenters"),
		("TectonicPlatesRow/TectonicPlatesCheck", "TectonicPlates"),
		("ElevationRow/ElevationCheck", "Elevation"),
		("MoistureRow/MoistureCheck", "Moisture"),
		("PlateLabelsRow/PlateLabelsCheck", "PlateLabels"),
	};

	private static readonly (string CheckSuffix, string NodeName)[] VoronoiOnlyWhenTerrainMesh =
	{
		("ElevationRow/ElevationCheck", "Elevation"),
		("MoistureRow/MoistureCheck", "Moisture"),
		("PlateLabelsRow/PlateLabelsCheck", "PlateLabels"),
	};

	private bool _terrainMeshExpanded;
	private bool _layerTogglesConnected;

	public override void _Ready()
	{
		ApplyUiAssets();
		DisableKeyboardFocusOnLayerControls();
		EnsureLayerTogglesConnected();
	}

	private void DisableKeyboardFocusOnLayerControls()
	{
		DisableFocusRecursive(this);
	}

	private static void DisableFocusRecursive(Node node)
	{
		if (node is Control control)
			control.FocusMode = FocusModeEnum.None;
		foreach (var child in node.GetChildren())
			DisableFocusRecursive(child);
	}

	private void ApplyUiAssets()
	{
		var card = GetNodeOrNull<PanelContainer>("MarginContainer/Card");
		if (card == null)
			return;
		var simApi = GetNodeOrNull<SimAPI>("/root/SimAPI");
		var tex = simApi?.GetUiAssetTexture("panel_card");
		if (tex != null)
		{
			var style = new StyleBoxTexture { Texture = tex };
			card.AddThemeStyleboxOverride("panel", style);
		}
	}

	public void EnsureLayerTogglesConnected()
	{
		if (_layerTogglesConnected)
			return;

		var vbox = GetNodeOrNull<VBoxContainer>(LayersVBoxPath);
		if (vbox == null)
			return;

		foreach (var (checkSuffix, nodeName) in DiagnosticLayers)
			ConnectCheckToNode(vbox, checkSuffix, nodeName);

		foreach (var (checkSuffix, nodeName) in TerrainHeatmapLayers)
			ConnectCheckToNode(vbox, checkSuffix, nodeName);

		ConnectVoronoiToggle(vbox);
		ConnectTerrainMeshSection();
		_layerTogglesConnected = true;
		SyncAllLayersFromPreview();
	}

	private void ConnectCheckToNode(VBoxContainer vbox, string checkSuffix, string nodeName)
	{
		var cb = vbox.GetNodeOrNull<CheckBox>(checkSuffix);
		if (cb == null)
			return;
		cb.Toggled += (bool on) => SetLayerVisible(GetSpherePreview(), nodeName, on);
	}

	private void ConnectVoronoiToggle(VBoxContainer vbox)
	{
		var voronoiCb = vbox.GetNodeOrNull<CheckBox>("VoronoiRow/VoronoiCheck");
		if (voronoiCb == null)
			return;
		voronoiCb.Toggled += (bool on) => SetVoronoiVisible(GetSpherePreview(), on);
	}

	private void ConnectTerrainMeshSection()
	{
		var expandBtn = GetNodeOrNull<Button>(TerrainHeaderPath + "/PlanetTerrainMeshExpandBtn");
		var subVBox = GetNodeOrNull<VBoxContainer>(TerrainSubVBoxPath);
		if (expandBtn != null && subVBox != null)
		{
			SetTerrainMeshExpanded(_terrainMeshExpanded, expandBtn, subVBox);
			expandBtn.Pressed += () =>
			{
				_terrainMeshExpanded = !_terrainMeshExpanded;
				SetTerrainMeshExpanded(_terrainMeshExpanded, expandBtn, subVBox);
			};
		}

		var masterCb = GetNodeOrNull<CheckBox>(TerrainHeaderPath + "/PlanetTerrainMeshCheck");
		if (masterCb != null)
		{
			masterCb.Toggled += (bool on) =>
			{
				var preview = GetSpherePreview();
				var terrainRoot = preview?.GetNodeOrNull<Node3D>("PlanetTerrainMesh");
				if (terrainRoot == null)
					return;
				terrainRoot.Visible = on;
				var finalTerrain = terrainRoot.GetNodeOrNull<Node3D>("FinalTerrain");
				if (finalTerrain != null)
					finalTerrain.Visible = on;
				if (!on)
				{
					var rivers = terrainRoot.GetNodeOrNull<Node3D>("RiversMap");
					if (rivers != null)
						rivers.Visible = false;
					SetCheckBlocked(GetNodeOrNull<CheckBox>(TerrainSubVBoxPath + "/RiversMapRow/RiversMapCheck"), false);
				}
			};
		}

		var riversCb = GetNodeOrNull<CheckBox>(TerrainSubVBoxPath + "/RiversMapRow/RiversMapCheck");
		if (riversCb != null)
		{
			riversCb.Toggled += (bool on) =>
			{
				var preview = GetSpherePreview();
				var terrainRoot = preview?.GetNodeOrNull<Node3D>("PlanetTerrainMesh");
				var rivers = terrainRoot?.GetNodeOrNull<Node3D>("RiversMap");
				if (rivers == null)
					return;
				rivers.Visible = on;
				if (on && terrainRoot != null)
				{
					terrainRoot.Visible = true;
					var finalTerrain = terrainRoot.GetNodeOrNull<Node3D>("FinalTerrain");
					if (finalTerrain != null)
						finalTerrain.Visible = true;
					SetCheckBlocked(masterCb, true);
				}
			};
		}
	}

	private static void SetTerrainMeshExpanded(bool expanded, Button expandBtn, VBoxContainer subVBox)
	{
		subVBox.Visible = expanded;
		expandBtn.Text = expanded ? "▾" : "▸";
	}

	/// <summary>One-time default when watertight terrain mesh arrives: hide clutter, show final mesh.</summary>
	public void ApplyInitialTerrainView()
	{
		EnsureLayerTogglesConnected();
		var preview = GetSpherePreview();
		var vbox = GetNodeOrNull<VBoxContainer>(LayersVBoxPath);
		if (preview == null || vbox == null)
			return;

		foreach (var (_, nodeName) in DiagnosticLayers)
			SetLayerVisible(preview, nodeName, false);
		SetVoronoiVisible(preview, false);
		SetTerrainHeatmapLayersVisible(preview, false);
		SetLayerVisible(preview, "PlanetTerrainMesh", true);
		SetFinalTerrainVisible(preview, true);

		var terrainRoot = preview.GetNodeOrNull<Node3D>("PlanetTerrainMesh");
		var rivers = terrainRoot?.GetNodeOrNull<Node3D>("RiversMap");
		if (rivers != null)
			rivers.Visible = false;

		UpdateTerrainMeshUiMode(true);
		SyncAllLayersFromPreview();
	}

	/// <summary>Re-enable Voronoi-only rows when returning to legacy circumcenter preview.</summary>
	public void UpdateTerrainMeshUiMode(bool hasTerrainMesh)
	{
		var vbox = GetNodeOrNull<VBoxContainer>(LayersVBoxPath);
		if (vbox == null)
			return;

		var preview = GetSpherePreview();
		foreach (var (checkSuffix, nodeName) in VoronoiOnlyWhenTerrainMesh)
		{
			var cb = vbox.GetNodeOrNull<CheckBox>(checkSuffix);
			if (cb == null)
				continue;
			cb.Disabled = hasTerrainMesh;
			if (hasTerrainMesh)
			{
				SetCheckBlocked(cb, false);
				if (preview != null)
					SetLayerVisible(preview, nodeName, false);
			}
		}
	}

	public void SyncAllLayersFromPreview()
	{
		EnsureLayerTogglesConnected();
		var preview = GetSpherePreview();
		var vbox = GetNodeOrNull<VBoxContainer>(LayersVBoxPath);
		if (preview == null || vbox == null)
			return;

		foreach (var (checkSuffix, nodeName) in DiagnosticLayers)
		{
			var cb = vbox.GetNodeOrNull<CheckBox>(checkSuffix);
			if (cb == null || cb.Disabled)
				continue;
			var node = preview.GetNodeOrNull<Node3D>(nodeName);
			if (node != null)
				SetCheckBlocked(cb, node.Visible);
		}

		foreach (var (checkSuffix, nodeName) in TerrainHeatmapLayers)
		{
			var node = preview.GetNodeOrNull<Node3D>(nodeName);
			if (node != null)
				SetCheckBlocked(vbox.GetNodeOrNull<CheckBox>(checkSuffix), node.Visible);
		}

		var voronoi = preview.GetNodeOrNull<Node3D>("Voronoi");
		if (voronoi != null)
			SetCheckBlocked(vbox.GetNodeOrNull<CheckBox>("VoronoiRow/VoronoiCheck"), voronoi.Visible);

		var terrainRoot = preview.GetNodeOrNull<Node3D>("PlanetTerrainMesh");
		if (terrainRoot == null)
		{
			if (preview is SpherePreview sphere)
				UpdateTerrainMeshUiMode(sphere.HasActivePlanetTerrainMesh());
			return;
		}

		SetCheckBlocked(GetNodeOrNull<CheckBox>(TerrainHeaderPath + "/PlanetTerrainMeshCheck"), terrainRoot.Visible);
		var finalTerrain = terrainRoot.GetNodeOrNull<Node3D>("FinalTerrain");
		if (finalTerrain != null)
			SetCheckBlocked(GetNodeOrNull<CheckBox>(TerrainHeaderPath + "/PlanetTerrainMeshCheck"), finalTerrain.Visible);

		var rivers = terrainRoot.GetNodeOrNull<Node3D>("RiversMap");
		if (rivers != null)
			SetCheckBlocked(GetNodeOrNull<CheckBox>(TerrainSubVBoxPath + "/RiversMapRow/RiversMapCheck"), rivers.Visible);

		if (preview is SpherePreview spherePreview)
			UpdateTerrainMeshUiMode(spherePreview.HasActivePlanetTerrainMesh());
	}

	public void SyncPlanetTerrainMeshCheckbox() => SyncAllLayersFromPreview();

	private static void SetCheckBlocked(CheckBox cb, bool pressed)
	{
		if (cb == null)
			return;
		cb.SetBlockSignals(true);
		cb.ButtonPressed = pressed;
		cb.SetBlockSignals(false);
	}

	private static void SetTerrainHeatmapLayersVisible(Node3D preview, bool visible)
	{
		foreach (var (_, nodeName) in TerrainHeatmapLayers)
			SetLayerVisible(preview, nodeName, visible);
	}

	private static void SetFinalTerrainVisible(Node3D preview, bool visible)
	{
		var terrainRoot = preview?.GetNodeOrNull<Node3D>("PlanetTerrainMesh");
		var finalTerrain = terrainRoot?.GetNodeOrNull<Node3D>("FinalTerrain");
		if (finalTerrain != null)
			finalTerrain.Visible = visible;
	}

	private static void SetVoronoiVisible(Node3D preview, bool visible)
	{
		if (preview == null)
			return;
		var voronoi = preview.GetNodeOrNull<Node3D>("Voronoi");
		if (voronoi == null)
			return;
		voronoi.Visible = visible;
		foreach (var child in voronoi.GetChildren())
		{
			if (child is Node3D n3d)
				n3d.Visible = visible;
		}
	}

	private static void SetLayerVisible(Node3D preview, string name, bool visible)
	{
		if (preview == null)
			return;
		var node = preview.GetNodeOrNull<Node3D>(name);
		if (node == null)
			return;
		node.Visible = visible;
	}

	private SpherePreview GetSpherePreview()
	{
		if (GetParent() is CanvasLayer layer)
		{
			if (layer.GetParent() is SpherePreview onPreview)
				return onPreview;
		}

		Node n = this;
		while (n != null)
		{
			if (n is SpherePreview preview)
				return preview;
			n = n.GetParent();
		}
		return null;
	}
}
