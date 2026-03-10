using Godot;

/// <summary>
/// Layer visibility toggles for SpherePreview. Uses SimAPI UI asset cache (panel_card).
/// Expects parent hierarchy: SpherePreviewOverlay -> CanvasLayer -> SpherePreview.
/// </summary>
public partial class SpherePreviewOverlay : Control
{
	public override void _Ready()
	{
		ApplyUiAssets();
		ConnectLayerToggles();
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

	private void ConnectLayerToggles()
	{
		var preview = GetSpherePreview();
		if (preview == null)
			return;
		var globe = preview.GetNodeOrNull<MeshInstance3D>("Globe");
		// Sites is created when SetSites() runs (after overlay is ready), so resolve on toggle
		var sitesNow = preview.GetNodeOrNull<MultiMeshInstance3D>("Sites");

		var vbox = GetNodeOrNull<VBoxContainer>("MarginContainer/Card/CardPadding/LayersVBox");
		if (vbox == null)
			return;
		var sphereCb = vbox.GetNodeOrNull<CheckBox>("SphereRow/SphereCheck");
		var pointsCb = vbox.GetNodeOrNull<CheckBox>("PointsRow/PointsCheck");
		var delaunayCb = vbox.GetNodeOrNull<CheckBox>("DelaunayRow/DelaunayCheck");
		var delaunayNow = preview.GetNodeOrNull<Node3D>("Delaunay");
		var circumcentersCb = vbox.GetNodeOrNull<CheckBox>("CircumcentersRow/CircumcentersCheck");
		var circumcentersNow = preview.GetNodeOrNull<Node3D>("Circumcenters");
		var voronoiCb = vbox.GetNodeOrNull<CheckBox>("VoronoiRow/VoronoiCheck");
		var voronoiNow = preview.GetNodeOrNull<Node3D>("Voronoi");
		var tectonicCb = vbox.GetNodeOrNull<CheckBox>("TectonicPlatesRow/TectonicPlatesCheck");
		var tectonicNow = preview.GetNodeOrNull<Node3D>("TectonicPlates");
		var elevationCb = vbox.GetNodeOrNull<CheckBox>("ElevationRow/ElevationCheck");
		var elevationNow = preview.GetNodeOrNull<Node3D>("Elevation");
		var moistureCb = vbox.GetNodeOrNull<CheckBox>("MoistureRow/MoistureCheck");
		var moistureNow = preview.GetNodeOrNull<Node3D>("Moisture");
		var plateLabelsCb = vbox.GetNodeOrNull<CheckBox>("PlateLabelsRow/PlateLabelsCheck");
		var plateLabelsNow = preview.GetNodeOrNull<Node3D>("PlateLabels");

		if (sphereCb != null && globe != null)
		{
			sphereCb.ButtonPressed = globe.Visible;
			sphereCb.Toggled += (bool on) => globe.Visible = on;
		}
		if (pointsCb != null)
		{
			if (sitesNow != null)
				pointsCb.ButtonPressed = sitesNow.Visible;
			pointsCb.Toggled += (bool on) =>
			{
				var sites = GetSpherePreview()?.GetNodeOrNull<MultiMeshInstance3D>("Sites");
				if (sites != null)
					sites.Visible = on;
			};
		}
		if (delaunayCb != null)
		{
			if (delaunayNow != null)
				delaunayCb.ButtonPressed = delaunayNow.Visible;
			delaunayCb.Toggled += (bool on) =>
			{
				var delaunay = GetSpherePreview()?.GetNodeOrNull<Node3D>("Delaunay");
				if (delaunay != null)
					delaunay.Visible = on;
			};
		}
		if (circumcentersCb != null)
		{
			if (circumcentersNow != null)
				circumcentersCb.ButtonPressed = circumcentersNow.Visible;
			circumcentersCb.Toggled += (bool on) =>
			{
				var circumcenters = GetSpherePreview()?.GetNodeOrNull<Node3D>("Circumcenters");
				if (circumcenters != null)
					circumcenters.Visible = on;
			};
		}
		if (voronoiCb != null)
		{
			if (voronoiNow != null)
				voronoiCb.ButtonPressed = voronoiNow.Visible;
			voronoiCb.Toggled += (bool on) =>
			{
				var previewNode = GetSpherePreview() as Node;
				if (previewNode == null) return;
				var voronoi = previewNode.GetNodeOrNull<Node3D>("Voronoi");
				if (voronoi != null)
				{
					voronoi.Visible = on;
					foreach (var child in voronoi.GetChildren())
					{
						if (child is Node3D n3d)
							n3d.Visible = on;
					}
				}
			};
		}
		if (tectonicCb != null)
		{
			if (tectonicNow != null)
				tectonicCb.ButtonPressed = tectonicNow.Visible;
			tectonicCb.Toggled += (bool on) =>
			{
				var tectonic = GetSpherePreview()?.GetNodeOrNull<Node3D>("TectonicPlates");
				if (tectonic != null)
					tectonic.Visible = on;
			};
		}
		if (elevationCb != null)
		{
			if (elevationNow != null)
				elevationCb.ButtonPressed = elevationNow.Visible;
			elevationCb.Toggled += (bool on) =>
			{
				var elevation = GetSpherePreview()?.GetNodeOrNull<Node3D>("Elevation");
				if (elevation != null)
					elevation.Visible = on;
			};
		}
		if (moistureCb != null)
		{
			if (moistureNow != null)
				moistureCb.ButtonPressed = moistureNow.Visible;
			moistureCb.Toggled += (bool on) =>
			{
				var moisture = GetSpherePreview()?.GetNodeOrNull<Node3D>("Moisture");
				if (moisture != null)
					moisture.Visible = on;
			};
		}
		if (plateLabelsCb != null)
		{
			if (plateLabelsNow != null)
				plateLabelsCb.ButtonPressed = plateLabelsNow.Visible;
			plateLabelsCb.Toggled += (bool on) =>
			{
				var plateLabels = GetSpherePreview()?.GetNodeOrNull<Node3D>("PlateLabels");
				if (plateLabels != null)
					plateLabels.Visible = on;
			};
		}
	}

	private Node3D GetSpherePreview()
	{
		var layer = GetParent();
		var preview = layer?.GetParent();
		return preview as Node3D;
	}
}
