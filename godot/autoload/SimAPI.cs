using Godot;
using System.Collections.Generic;

/// <summary>
/// Single door into C++ sim. No other scripts talk to GDExtension directly.
/// Uses C++ GrowthSim when the extension is built; otherwise falls back to stub.
/// </summary>
public partial class SimAPI : Node
{
	public const string DIFF_CHUNK_LOADED = "ChunkLoaded";
	public const string DIFF_CHUNK_UNLOADED = "ChunkUnloaded";
	public const string DIFF_CELL_CHANGED = "CellChanged";

	private RefCounted _backend;
	private readonly List<Godot.Collections.Dictionary> _stubDiffQueue = new();
	private readonly Dictionary<Vector2I, bool> _stubRequested = new();
	private Dictionary<string, string> _uiAssetPaths = new();
	private readonly Dictionary<string, Texture2D> _cachedUiTextures = new();

	public override void _Ready()
	{
		if (ClassDB.ClassExists("GrowthSim"))
			_backend = ClassDB.Instantiate("GrowthSim").As<RefCounted>();

		if (_backend != null)
			GD.Print("[SimAPI] using C++ GrowthSim backend");
		else
		{
			GD.Print("[SimAPI] GrowthSim not found; using stub backend");
			LoadUiAssetsFallback();
		}
	}

	private void LoadUiAssetsFallback()
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

	/// <summary>Initialise the sim (path only). No generation; call ApplyWorldGenForm when the user submits the world gen form.</summary>
	public void boot(string defdbPath)
	{
		if (_backend != null)
		{
			_backend.Call("boot", defdbPath);
			return;
		}
		_stubDiffQueue.Clear();
		_stubRequested.Clear();
		GD.Print("[SimAPI] boot defdb_path=", defdbPath);
	}

	/// <summary>Run world gen from form; returns Dictionary with "sites" (direct ref) and "triangles" (copied to Array so GDScript receives it; PackedInt32Array does not survive C#→GDScript otherwise).</summary>
	public Godot.Collections.Dictionary ApplyWorldGenForm(Godot.Collections.Dictionary formDict)
	{
		if (_backend != null)
		{
			var v = _backend.Call("apply_world_gen_form", formDict);
			var dict = v.As<Godot.Collections.Dictionary>();
			if (dict != null)
			{
				var result = new Godot.Collections.Dictionary();
				result["sites"] = dict["sites"];
				// Fetch triangles via direct method return (PackedInt32Array marshals as int[]); copy to Array for GDScript
				var triRet = _backend.Call("get_last_world_gen_triangles");
				var triInts = triRet.As<int[]>();
				if (triInts != null && triInts.Length > 0)
				{
					var arr = new Godot.Collections.Array();
					foreach (var x in triInts) arr.Add(x);
					result["triangles"] = arr;
				}
				else
				{
					var triArr = triRet.As<Godot.Collections.Array>();
					result["triangles"] = triArr != null ? triArr : new Godot.Collections.Array();
				}
				// Voronoi: copy circumcenters into Array so C#/GDScript receive it reliably (same as triangles)
				if (dict.ContainsKey("circumcenters"))
				{
					var cc = dict["circumcenters"].As<Vector3[]>();
					if (cc != null && cc.Length > 0)
					{
						var ccArr = new Godot.Collections.Array();
						foreach (var vec in cc) ccArr.Add(vec);
						result["circumcenters"] = ccArr;
					}
					else
						result["circumcenters"] = dict["circumcenters"];
				}
				if (dict.ContainsKey("cells"))
					result["cells"] = dict["cells"];
				if (dict.ContainsKey("plate_regions"))
				{
					var pr = dict["plate_regions"].As<int[]>();
					if (pr != null && pr.Length > 0)
					{
						var prArr = new Godot.Collections.Array();
						foreach (var x in pr) prArr.Add(x);
						result["plate_regions"] = prArr;
						GD.Print("[SimAPI] apply_world_gen_form: plate_regions forwarded size=", pr.Length);
					}
					else
					{
						result["plate_regions"] = dict["plate_regions"];
						GD.Print("[SimAPI] apply_world_gen_form: plate_regions forwarded (raw)");
					}
				}
				else
					GD.Print("[SimAPI] apply_world_gen_form: no plate_regions in result");
				if (dict.ContainsKey("log_lines"))
					result["log_lines"] = dict["log_lines"];
				if (dict.ContainsKey("plate_elevation"))
				{
					var elev = dict["plate_elevation"].As<float[]>();
					if (elev != null && elev.Length > 0)
					{
						var elevArr = new Godot.Collections.Array();
						foreach (var x in elev) elevArr.Add(x);
						result["plate_elevation"] = elevArr;
					}
					else
						result["plate_elevation"] = dict["plate_elevation"];
				}
				if (dict.ContainsKey("plate_moisture"))
				{
					var moist = dict["plate_moisture"].As<float[]>();
					if (moist != null && moist.Length > 0)
					{
						var moistArr = new Godot.Collections.Array();
						foreach (var x in moist) moistArr.Add(x);
						result["plate_moisture"] = moistArr;
					}
					else
						result["plate_moisture"] = dict["plate_moisture"];
				}
				// Per-region and per-triangle terrain (for streaming / detailed map)
				ForwardFloatArray(dict, result, "region_elevation");
				ForwardFloatArray(dict, result, "region_moisture");
				ForwardFloatArray(dict, result, "triangle_elevation");
				ForwardFloatArray(dict, result, "triangle_moisture");
				ForwardIntArray(dict, result, "mesh_s_begin_r");
				ForwardIntArray(dict, result, "mesh_s_end_r");
				ForwardIntArray(dict, result, "mesh_s_inner_t");
				ForwardIntArray(dict, result, "mesh_s_outer_t");
				ForwardIntArray(dict, result, "mesh_s_next_s");
				ForwardIntArray(dict, result, "mesh_s_twin_s");
				return result;
			}
		}
		GD.Print("[SimAPI] apply_world_gen_form (stub) ", formDict);
		var empty = new Godot.Collections.Dictionary();
		empty["sites"] = System.Array.Empty<Vector3>();
		empty["triangles"] = System.Array.Empty<int>();
		return empty;
	}

	private static void ForwardFloatArray(Godot.Collections.Dictionary dict, Godot.Collections.Dictionary result, string key)
	{
		if (!dict.ContainsKey(key)) return;
		var arr = dict[key].As<float[]>();
		if (arr != null && arr.Length > 0)
		{
			var outArr = new Godot.Collections.Array();
			foreach (var x in arr) outArr.Add(x);
			result[key] = outArr;
		}
		else
			result[key] = dict[key];
	}

	private static void ForwardIntArray(Godot.Collections.Dictionary dict, Godot.Collections.Dictionary result, string key)
	{
		if (!dict.ContainsKey(key)) return;
		var arr = dict[key].As<int[]>();
		if (arr != null && arr.Length > 0)
		{
			var outArr = new Godot.Collections.Array();
			foreach (var x in arr) outArr.Add(x);
			result[key] = outArr;
		}
		else
			result[key] = dict[key];
	}

	/// <summary>Snake_case alias for GDScript.</summary>
	public Godot.Collections.Dictionary apply_world_gen_form(Godot.Collections.Dictionary formDict) => ApplyWorldGenForm(formDict);

	/// <summary>Advance sim by dt seconds; speed is time scale multiplier.</summary>
	public void step(double dt, double speed)
	{
		if (_backend != null)
		{
			_backend.Call("step", dt, speed);
			return;
		}
	}

	/// <summary>Set sim speed mode (e.g. 0=pause, 1=normal, 2=fast).</summary>
	public void set_speed(int mode)
	{
		if (_backend != null)
		{
			_backend.Call("set_speed", mode);
			return;
		}
		GD.Print("[SimAPI] set_speed mode=", mode);
	}

	/// <summary>Request chunk data for chunks in [center_chunk - radius, center_chunk + radius].</summary>
	public void request_chunks(Vector2I centerChunk, int radius)
	{
		if (_backend != null)
		{
			_backend.Call("request_chunks", centerChunk, radius);
			return;
		}
		for (int ox = -radius; ox <= radius; ox++)
		for (int oy = -radius; oy <= radius; oy++)
		{
			var c = new Vector2I(centerChunk.X + ox, centerChunk.Y + oy);
			if (_stubRequested.TryGetValue(c, out var have) && have)
				continue;
			_stubRequested[c] = true;
			_stubDiffQueue.Add(StubMakeChunkLoaded(c));
		}
	}

	private static Godot.Collections.Dictionary StubMakeChunkLoaded(Vector2I coord)
	{
		const int side = 17;
		var heightSamples = new Godot.Collections.Array();
		heightSamples.Resize(side * side);
		for (int i = 0; i < side * side; i++)
			heightSamples[i] = 0.0f;
		return new Godot.Collections.Dictionary
		{
			["type"] = DIFF_CHUNK_LOADED,
			["coord"] = coord,
			["height_samples"] = heightSamples
		};
	}

	/// <summary>Poll up to max diffs from sim. Returns array of dicts.</summary>
	public Godot.Collections.Array poll_diffs(int maxCount)
	{
		if (_backend != null)
		{
			var raw = _backend.Call("poll_diffs", maxCount).As<Godot.Collections.Array>();
			var diffs = new Godot.Collections.Array();
			foreach (Variant v in raw)
			{
				if (v.Obj is Godot.Collections.Dictionary d)
					diffs.Add(NormaliseDiff(d));
			}
			return diffs;
		}
		var result = new Godot.Collections.Array();
		int take = System.Math.Min(maxCount, _stubDiffQueue.Count);
		for (int i = 0; i < take; i++)
		{
			result.Add(_stubDiffQueue[0]);
			_stubDiffQueue.RemoveAt(0);
		}
		return result;
	}

	private static Godot.Collections.Dictionary NormaliseDiff(Godot.Collections.Dictionary d)
	{
		var dup = new Godot.Collections.Dictionary();
		foreach (var kv in d)
			dup[kv.Key] = kv.Value;
		if (dup.ContainsKey("height_samples"))
		{
			var heightVal = dup["height_samples"];
			// C++ may return PackedFloat32Array; C# bindings expose as float[] or similar.
			float[] floats = heightVal.As<float[]>();
			if (floats != null && floats.Length > 0)
			{
				var arr = new Godot.Collections.Array();
				arr.Resize(floats.Length);
				for (int i = 0; i < floats.Length; i++)
					arr[i] = floats[i];
				dup["height_samples"] = arr;
			}
		}
		return dup;
	}

	/// <summary>Apply one intent (e.g. dig at cell). Temporary dict form; typed later.</summary>
	public void apply_intent(Godot.Collections.Dictionary intentDict)
	{
		if (_backend != null)
		{
			_backend.Call("apply_intent", intentDict);
			return;
		}
		GD.Print("[SimAPI] apply_intent ", intentDict);
	}

	/// <summary>Return res:// path for a UI asset id from data/core/defs/ui_assets.xml. Empty if unknown.</summary>
	public string GetUiAssetPath(string assetId) => ResolveUiAssetPath(assetId);

	/// <summary>Snake_case alias for GDScript (GetUiAssetPath).</summary>
	public string get_ui_asset_path(string assetId) => ResolveUiAssetPath(assetId);

	/// <summary>Load and cache a UI texture by asset id. Returns the same instance for repeated calls to avoid duplicate copies.</summary>
	public Texture2D GetUiAssetTexture(string assetId) => ResolveUiAssetTexture(assetId);

	/// <summary>Snake_case alias for GDScript.</summary>
	public Texture2D get_ui_asset_texture(string assetId) => ResolveUiAssetTexture(assetId);

	private Texture2D ResolveUiAssetTexture(string assetId)
	{
		if (_cachedUiTextures.TryGetValue(assetId, out var cached))
			return cached;
		string path = ResolveUiAssetPath(assetId);
		if (string.IsNullOrEmpty(path))
			return null;
		var tex = GD.Load<Texture2D>(path);
		if (tex != null)
			_cachedUiTextures[assetId] = tex;
		return tex;
	}

	private string ResolveUiAssetPath(string assetId)
	{
		if (_backend != null)
		{
			var v = _backend.Call("get_ui_asset_path", assetId);
			return v.As<string>() ?? "";
		}
		return _uiAssetPaths.TryGetValue(assetId, out var p) ? p : "";
	}
}
