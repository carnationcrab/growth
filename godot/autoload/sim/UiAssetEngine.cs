using Godot;
using System.Collections.Generic;

/// <summary>
/// Resolves UI asset paths from GrowthSim or stub ui_assets.xml; caches loaded textures.
/// </summary>
public sealed class UiAssetEngine
{
	private Dictionary<string, string> _uiAssetPaths = new();
	private readonly Dictionary<string, Texture2D> _cachedUiTextures = new();

	public void LoadFallback()
	{
		_uiAssetPaths.Clear();
		using var f = FileAccess.Open("res://data/core/defs/ui_assets.xml", FileAccess.ModeFlags.Read);
		if (f == null) return;
		string text = f.GetAsText();
		// Minimal parse: find asset_id="..." path="..."
		var idRegex = new System.Text.RegularExpressions.Regex(@"asset_id=""([^""]+)""\s+path=""([^""]+)""");
		foreach (System.Text.RegularExpressions.Match m in idRegex.Matches(text))
			_uiAssetPaths[m.Groups[1].Value] = m.Groups[2].Value;
	}

	public string GetPath(GrowthSimBackend backend, string assetId)
	{
		if (backend.IsCpp)
			return backend.GetUiAssetPath(assetId);
		return _uiAssetPaths.TryGetValue(assetId, out var p) ? p : "";
	}

	public Texture2D GetTexture(GrowthSimBackend backend, string assetId)
	{
		if (_cachedUiTextures.TryGetValue(assetId, out var cached))
			return cached;
		string path = GetPath(backend, assetId);
		if (string.IsNullOrEmpty(path))
			return null;
		var tex = GD.Load<Texture2D>(path);
		if (tex != null)
			_cachedUiTextures[assetId] = tex;
		return tex;
	}
}
