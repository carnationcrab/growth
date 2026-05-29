using Godot;

/// <summary>
/// Overworld session state in the sim bridge (atlas built at world gen; committed on Start).
/// </summary>
public sealed class OverworldSessionEngine
{
	public bool HasOverworld(GrowthSimBackend backend)
	{
		if (!backend.IsCpp)
			return false;
		var v = backend.Call("has_overworld");
		return v.AsBool();
	}

	public void CommitForPlay(GrowthSimBackend backend)
	{
		if (!backend.IsCpp)
		{
			GD.PrintErr("[SimAPI] commit_overworld_for_play: stub backend — build GDExtension.");
			return;
		}
		backend.Call("commit_overworld_for_play");
		GD.Print("[SimAPI] commit_overworld_for_play: session marked play-committed.");
	}

	/// <summary>Sample overworld at a unit direction in sim space (Z-up).</summary>
	public Godot.Collections.Dictionary SampleSurface(GrowthSimBackend backend, Vector3 unitDirSim)
	{
		if (!backend.IsCpp)
			return new Godot.Collections.Dictionary();
		var v = backend.Call("sample_surface", unitDirSim);
		return v.As<Godot.Collections.Dictionary>() ?? new Godot.Collections.Dictionary();
	}
}
