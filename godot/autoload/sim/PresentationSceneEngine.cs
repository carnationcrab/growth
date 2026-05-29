using Godot;
using System.Collections.Generic;

/// <summary>
/// Resolves presentation bindings (game_profile.xml) to scene paths (scene_registry.xml).
/// </summary>
public sealed class PresentationSceneEngine
{
	/// <summary>Presentation binding (e.g. planet_preview) -> scene_id from game_profile.xml.</summary>
	private readonly Dictionary<string, string> _presentationSceneIds = new();
	/// <summary>scene_id -> res:// path from scene_registry.xml.</summary>
	private readonly Dictionary<string, string> _scenePaths = new();
	/// <summary>script_id -> res:// path from scene_registry.xml.</summary>
	private readonly Dictionary<string, string> _scriptPaths = new();
	/// <summary>Presentation binding with script_id (e.g. world_view) -> script_id.</summary>
	private readonly Dictionary<string, string> _presentationScriptIds = new();

	public void Load(string defdbBase, string profileId = "default")
	{
		_ = profileId;
		_presentationSceneIds.Clear();
		_presentationScriptIds.Clear();
		_scenePaths.Clear();
		_scriptPaths.Clear();

		string[] bases =
		{
			string.IsNullOrEmpty(defdbBase) ? "" : (defdbBase.EndsWith("/") ? defdbBase : defdbBase + "/"),
			"res://data/core/",
		};
		foreach (string basePath in bases)
		{
			if (string.IsNullOrEmpty(basePath))
				continue;
			if (_scenePaths.Count == 0)
				LoadSceneRegistry(basePath + "defs/scene_registry.xml");
			if (_presentationSceneIds.Count == 0 && _presentationScriptIds.Count == 0)
				LoadGameProfilePresentation(basePath + "defs/game_profile.xml");
			if (_scenePaths.Count > 0 && (_presentationSceneIds.Count > 0 || _presentationScriptIds.Count > 0))
				break;
		}
	}

	public string GetScenePath(string binding)
	{
		if (string.IsNullOrEmpty(binding))
			return "";
		if (_presentationSceneIds.TryGetValue(binding, out var sceneId) && !string.IsNullOrEmpty(sceneId))
		{
			if (_scenePaths.TryGetValue(sceneId, out var path))
				return path;
		}
		return "";
	}

	public string GetScriptPath(string scriptId)
	{
		if (string.IsNullOrEmpty(scriptId))
			return "";
		return _scriptPaths.TryGetValue(scriptId, out var path) ? path : "";
	}

	/// <summary>Resolve scene_registry scene_id directly (e.g. chunk_node).</summary>
	public string GetScenePathForSceneId(string sceneId)
	{
		if (string.IsNullOrEmpty(sceneId))
			return "";
		return _scenePaths.TryGetValue(sceneId, out var path) ? path : "";
	}

	/// <summary>script_id from game_profile world_view binding (e.g. chunk_streaming).</summary>
	public string GetWorldViewScriptId()
	{
		if (_presentationScriptIds.TryGetValue("world_view", out var scriptId))
			return scriptId ?? "";
		return "";
	}

	private void LoadSceneRegistry(string path)
	{
		using var f = FileAccess.Open(path, FileAccess.ModeFlags.Read);
		if (f == null)
			return;
		var text = f.GetAsText();
		var sceneRegex = new System.Text.RegularExpressions.Regex(
			@"scene_id=""([^""]+)""\s+path=""([^""]+)""");
		foreach (System.Text.RegularExpressions.Match m in sceneRegex.Matches(text))
			_scenePaths[m.Groups[1].Value] = m.Groups[2].Value;

		var scriptRegex = new System.Text.RegularExpressions.Regex(
			@"script_id=""([^""]+)""\s+path=""([^""]+)""");
		foreach (System.Text.RegularExpressions.Match m in scriptRegex.Matches(text))
			_scriptPaths[m.Groups[1].Value] = m.Groups[2].Value;
	}

	private void LoadGameProfilePresentation(string path)
	{
		using var f = FileAccess.Open(path, FileAccess.ModeFlags.Read);
		if (f == null)
			return;
		var text = f.GetAsText();
		var sceneBindRegex = new System.Text.RegularExpressions.Regex(
			@"<(\w+)\s+scene_id=""([^""]+)""");
		foreach (System.Text.RegularExpressions.Match m in sceneBindRegex.Matches(text))
		{
			string tag = m.Groups[1].Value;
			if (tag is "scene" or "presentation" or "game_profile")
				continue;
			_presentationSceneIds[tag] = m.Groups[2].Value;
		}

		var scriptBindRegex = new System.Text.RegularExpressions.Regex(
			@"<(\w+)\s+script_id=""([^""]+)""");
		foreach (System.Text.RegularExpressions.Match m in scriptBindRegex.Matches(text))
		{
			string tag = m.Groups[1].Value;
			if (tag is "scene" or "presentation" or "game_profile")
				continue;
			_presentationScriptIds[tag] = m.Groups[2].Value;
		}
	}
}
