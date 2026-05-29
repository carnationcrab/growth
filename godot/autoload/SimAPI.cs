using Godot;

/// <summary>
/// Single door into C++ sim. No other scripts talk to GDExtension directly.
/// Composes focused engines under godot/autoload/sim/.
/// </summary>
public partial class SimAPI : Node
{
	public const string DIFF_CHUNK_LOADED = ChunkStreamingEngine.ChunkLoaded;
	public const string DIFF_CHUNK_UNLOADED = ChunkStreamingEngine.ChunkUnloaded;
	public const string DIFF_CELL_CHANGED = ChunkStreamingEngine.CellChanged;

	private readonly GrowthSimBackend _backend = new();
	private readonly WorldGenResultEngine _worldGen = new();
	private readonly ChunkStreamingEngine _streaming = new();
	private readonly UiAssetEngine _uiAssets = new();
	private readonly PresentationSceneEngine _presentation = new();
	private readonly OverworldSessionEngine _overworld = new();
	private readonly PlayWorldEntryEngine _playEntry = new();

	public override void _Ready()
	{
		_backend.TryInitialise();
		if (!_backend.IsCpp)
			_uiAssets.LoadFallback();
	}

	/// <summary>Initialise the sim (path only). No generation; call ApplyWorldGenForm when the user submits the world gen form.</summary>
	public void boot(string defdbPath)
	{
		_presentation.Load(defdbPath ?? "");
		_backend.Boot(defdbPath);
		if (!_backend.IsCpp)
		{
			_streaming.ResetStub();
			_uiAssets.LoadFallback();
			GD.Print("[SimAPI] boot defdb_path=", defdbPath);
		}
	}

	/// <summary>Run world gen from form; returns Dictionary with marshalled arrays for GDScript.</summary>
	public Godot.Collections.Dictionary ApplyWorldGenForm(Godot.Collections.Dictionary formDict)
		=> _worldGen.ApplyForm(_backend, formDict);

	/// <summary>Snake_case alias for GDScript.</summary>
	public Godot.Collections.Dictionary apply_world_gen_form(Godot.Collections.Dictionary formDict)
		=> ApplyWorldGenForm(formDict);

	/// <summary>Advance sim by dt seconds; speed is time scale multiplier.</summary>
	public void step(double dt, double speed) => _backend.Step(dt, speed);

	/// <summary>Set sim speed mode (e.g. 0=pause, 1=normal, 2=fast).</summary>
	public void set_speed(int mode) => _backend.SetSpeed(mode);

	/// <summary>Request chunk data for chunks in [center_chunk - radius, center_chunk + radius].</summary>
	public void request_chunks(Vector2I centerChunk, int radius)
		=> _streaming.RequestChunks(_backend, centerChunk, radius);

	/// <summary>Poll up to max diffs from sim. Returns array of dicts.</summary>
	public Godot.Collections.Array poll_diffs(int maxCount)
		=> _streaming.PollDiffs(_backend, maxCount);

	/// <summary>Apply one intent (e.g. dig at cell). Temporary dict form; typed later.</summary>
	public void apply_intent(Godot.Collections.Dictionary intentDict)
		=> _backend.ApplyIntent(intentDict);

	/// <summary>Return res:// path for a UI asset id from data/core/defs/ui_assets.xml. Empty if unknown.</summary>
	public string GetUiAssetPath(string assetId) => _uiAssets.GetPath(_backend, assetId);

	/// <summary>Snake_case alias for GDScript (GetUiAssetPath).</summary>
	public string get_ui_asset_path(string assetId) => GetUiAssetPath(assetId);

	/// <summary>Load and cache a UI texture by asset id. Returns the same instance for repeated calls to avoid duplicate copies.</summary>
	public Texture2D GetUiAssetTexture(string assetId) => _uiAssets.GetTexture(_backend, assetId);

	/// <summary>Snake_case alias for GDScript.</summary>
	public Texture2D get_ui_asset_texture(string assetId) => GetUiAssetTexture(assetId);

	/// <summary>
	/// Resolve a presentation binding from game_profile.xml to a scene res:// path via scene_registry.xml.
	/// Bindings: entry_menu, world_gen_load_screen, planet_preview.
	/// </summary>
	public string GetPresentationScenePath(string binding) => _presentation.GetScenePath(binding);

	/// <summary>Snake_case alias for GDScript.</summary>
	public string get_presentation_scene_path(string binding) => GetPresentationScenePath(binding);

	/// <summary>Apply a world-gen result dict to SpherePreview on main (presentation orchestration).</summary>
	public void ApplyWorldGenPreview(Node main, Godot.Collections.Dictionary result)
		=> WorldGenPreviewApplierEngine.Apply(main, result);

	/// <summary>Snake_case alias for GDScript.</summary>
	public void apply_world_gen_preview(Node main, Godot.Collections.Dictionary result)
		=> ApplyWorldGenPreview(main, result);

	/// <summary>Apply deferred/heavy preview payload (cells, terrain mesh) to an existing preview node.</summary>
	public void ApplyWorldGenPreviewHeavy(Node preview, Godot.Collections.Dictionary result)
		=> WorldGenPreviewApplierEngine.ApplyHeavy(preview, result);

	/// <summary>Snake_case alias for GDScript.</summary>
	public void apply_world_gen_preview_heavy(Node preview, Godot.Collections.Dictionary result)
		=> ApplyWorldGenPreviewHeavy(preview, result);

	/// <summary>True when the sim bridge holds a PlanetSurfaceAtlas from the last world gen.</summary>
	public bool HasOverworld() => _overworld.HasOverworld(_backend);

	/// <summary>Snake_case alias for GDScript.</summary>
	public bool has_overworld() => HasOverworld();

	/// <summary>Mark overworld as committed for play (idempotent).</summary>
	public void CommitOverworldForPlay() => _overworld.CommitForPlay(_backend);

	/// <summary>Snake_case alias for GDScript.</summary>
	public void commit_overworld_for_play() => CommitOverworldForPlay();

	/// <summary>Sample overworld fields at a unit direction in sim space (Z-up).</summary>
	public Godot.Collections.Dictionary SampleSurface(Vector3 unitDirSim)
		=> _overworld.SampleSurface(_backend, unitDirSim);

	/// <summary>Snake_case alias for GDScript.</summary>
	public Godot.Collections.Dictionary sample_surface(Vector3 unitDirSim)
		=> SampleSurface(unitDirSim);

	/// <summary>Leave planet preview and begin game-world streaming on Main.</summary>
	public void EnterGameWorld(Node main) => _playEntry.Enter(main, _backend, _streaming);

	/// <summary>Snake_case alias for GDScript.</summary>
	public void enter_game_world(Node main) => EnterGameWorld(main);
}
