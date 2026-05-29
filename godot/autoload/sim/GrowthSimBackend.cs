using Godot;

/// <summary>
/// Sole gateway to the GrowthSim GDExtension. Other engines delegate sim calls through this type.
/// </summary>
public sealed class GrowthSimBackend
{
	private const string GrowthSimDebugDll = "res://gde/bin/growth_sim.windows.template_debug.x86_64.dll";
	private const string GrowthSimReleaseDll = "res://gde/bin/growth_sim.windows.template_release.x86_64.dll";

	private RefCounted _backend;

	public bool IsCpp => _backend != null;

	/// <summary>Initialise C++ backend if available; print diagnostics when stub is used.</summary>
	public void TryInitialise()
	{
		if (ClassDB.ClassExists("GrowthSim"))
			_backend = ClassDB.Instantiate("GrowthSim").As<RefCounted>();

		if (_backend != null)
		{
			GD.Print("[SimAPI] using C++ GrowthSim backend");
			return;
		}

		GD.PrintErr("[SimAPI] GrowthSim not found; using stub backend (world gen will not run).");
		if (!ResourceLoader.Exists("res://gde/growth_sim.gdextension"))
			GD.PrintErr("[SimAPI] Missing res://gde/growth_sim.gdextension — enable the GDExtension config.");
		else if (!FileAccess.FileExists(GrowthSimDebugDll) && !FileAccess.FileExists(GrowthSimReleaseDll))
			GD.PrintErr("[SimAPI] Missing GDExtension DLL. From repo/gde run: .\\build.ps1  (copies to growth_sim.windows.template_debug.x86_64.dll)");
		else
			GD.PrintErr("[SimAPI] GDExtension config/DLL present but GrowthSim class did not register — check Godot Output for load errors, then rebuild with .\\build.ps1");
	}

	public void Boot(string defdbPath)
	{
		if (_backend != null)
			_backend.Call("boot", defdbPath);
	}

	public void Step(double dt, double speed)
	{
		if (_backend != null)
			_backend.Call("step", dt, speed);
	}

	public void SetSpeed(int mode)
	{
		if (_backend != null)
			_backend.Call("set_speed", mode);
		else
			GD.Print("[SimAPI] set_speed mode=", mode);
	}

	public void ApplyIntent(Godot.Collections.Dictionary intentDict)
	{
		if (_backend != null)
			_backend.Call("apply_intent", intentDict);
		else
			GD.Print("[SimAPI] apply_intent ", intentDict);
	}

	public Variant Call(string method, params Variant[] args)
	{
		if (_backend == null)
			return default;
		return _backend.Call(method, args);
	}

	public string GetUiAssetPath(string assetId)
	{
		if (_backend == null)
			return "";
		var v = _backend.Call("get_ui_asset_path", assetId);
		return v.As<string>() ?? "";
	}
}
