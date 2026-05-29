using Godot;

/// <summary>
/// Leaves overworld preview and enables game-world chunk streaming on Main.
/// </summary>
public sealed class PlayWorldEntryEngine
{
	private const string PreviewNodeName = "SpherePreview";
	private const int InitialStreamRadius = 2;

	public void Enter(Node main, GrowthSimBackend backend, ChunkStreamingEngine streaming)
	{
		if (main == null)
		{
			GD.PushWarning("[PlayWorldEntry] Enter: main node is null.");
			return;
		}

		GD.Print("[PlayWorldEntry] leaving overworld preview on ", main.Name, ".");

		GD.Print("[PlayWorldEntry] reset_streaming on WorldViewManager.");
		WorldViewManager.ResetStreamingOnMain(main);

		var preview = main.GetNodeOrNull(PreviewNodeName);
		if (preview != null)
		{
			GD.Print("[PlayWorldEntry] queueing free ", PreviewNodeName, ".");
			preview.QueueFree();
		}
		else
			GD.Print("[PlayWorldEntry] no ", PreviewNodeName, " on main (already removed?).");

		if (!backend.IsCpp)
		{
			GD.Print("[PlayWorldEntry] stub backend — skipping initial chunk request.");
			return;
		}

		var focus = main.GetNodeOrNull<Node3D>("Focus");
		if (focus == null)
		{
			GD.PushWarning("[PlayWorldEntry] Focus node not found; no initial chunk request.");
			return;
		}

		const int chunkSizeCells = 16;
		const float cellSize = 1.0f;
		var pos = focus.GlobalPosition;
		float cs = chunkSizeCells * cellSize;
		var centerChunk = new Vector2I(
			Mathf.FloorToInt(pos.X / cs),
			Mathf.FloorToInt(pos.Z / cs));
		GD.Print("[PlayWorldEntry] request_chunks center=", centerChunk, " radius=", InitialStreamRadius,
			" focus_pos=", pos, ".");
		streaming.RequestChunks(backend, centerChunk, InitialStreamRadius);
		GD.Print("[PlayWorldEntry] game world entry complete.");
	}
}
