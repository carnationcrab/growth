using Godot;
using System.Collections.Generic;

/// <summary>
/// Computes visible chunks around focus, requests them from sim, consumes diffs,
/// instantiates/unloads ChunkNodes and feeds them data.
/// </summary>
public partial class WorldViewManager : Node
{
	private const int ChunkSizeCells = 16;
	private const float CellSize = 1.0f;
	private const int StreamRadiusChunks = 2;

	private Node3D _worldRoot;
	private Node3D _focus;
	private PackedScene _chunkScene;
	private readonly Dictionary<Vector2I, Node> _chunks = new();

	public override void _Ready()
	{
	}

	/// <summary>Initialise with world root and focus node.</summary>
	public void setup(Node3D worldRoot, Node3D focus)
	{
		_worldRoot = worldRoot;
		_focus = focus;
		_chunkScene = GD.Load<PackedScene>("res://godot/world/ChunkNode.tscn");
		if (_chunkScene == null)
			GD.PushError("WorldViewManager: ChunkNode.tscn not found.");
	}

	/// <summary>Clear cached chunk nodes (e.g. when leaving overworld preview).</summary>
	public void reset_streaming()
	{
		int count = _chunks.Count;
		int freed = 0;
		foreach (var kv in _chunks)
		{
			if (SafeReleaseNode(kv.Value))
				freed++;
		}
		_chunks.Clear();
		GD.Print("[WorldViewManager] reset_streaming: dropped ", count, " cache entries, freed ", freed, " live node(s).");
	}

	/// <summary>Sync chunk cache after preview cleared WorldRoot without notifying this controller.</summary>
	public static void ResetStreamingOnMain(Node main)
	{
		if (FindOnMain(main) is WorldViewManager wvm)
			wvm.reset_streaming();
	}

	private static WorldViewManager FindOnMain(Node main)
	{
		if (main == null)
			return null;
		foreach (Node child in main.GetChildren())
		{
			if (child is WorldViewManager wvm)
				return wvm;
			if (child.GetScript().As<Script>() is CSharpScript cs
				&& cs.ResourcePath.EndsWith("WorldViewManager.cs"))
				return child as WorldViewManager;
		}
		return null;
	}

	private static bool SafeReleaseNode(Node node)
	{
		if (node == null || !GodotObject.IsInstanceValid(node))
			return false;
		node.QueueFree();
		return true;
	}

	/// <summary>Call each frame with current focus position to stream chunks.</summary>
	public void update_streaming(Vector3 focusPos)
	{
		if (_worldRoot == null || _chunkScene == null)
			return;
		Vector2I centerChunk = WorldToChunk(focusPos);
		SimAPI sim = GetNode<SimAPI>("/root/SimAPI");
		if (sim == null) return;
		sim.request_chunks(centerChunk, StreamRadiusChunks);
		ConsumeDiffs(sim);
		UnloadFarChunks(centerChunk);
	}

	private static Vector2I WorldToChunk(Vector3 worldPos)
	{
		float cs = ChunkSizeCells * CellSize;
		return new Vector2I(
			Mathf.FloorToInt(worldPos.X / cs),
			Mathf.FloorToInt(worldPos.Z / cs)
		);
	}

	private void ConsumeDiffs(SimAPI sim)
	{
		Godot.Collections.Array diffs = sim.poll_diffs(32);
		foreach (Variant v in diffs)
		{
			if (v.Obj is not Godot.Collections.Dictionary d)
				continue;
			string typ = d.ContainsKey("type") ? d["type"].AsString() : "";
			if (typ == "ChunkLoaded")
				ApplyChunkLoaded(d);
			else if (typ == "ChunkUnloaded")
				ApplyChunkUnloaded(d);
			else if (typ == "CellChanged")
				ApplyCellChanged(d);
		}
	}

	private void ApplyChunkLoaded(Godot.Collections.Dictionary d)
	{
		Vector2I coord = d.ContainsKey("coord") ? d["coord"].AsVector2I() : new Vector2I(int.MaxValue, int.MaxValue);
		if (coord.X == int.MaxValue && coord.Y == int.MaxValue)
			return;
		if (_chunks.ContainsKey(coord))
			return;
		Node node = _chunkScene.Instantiate<Node>();
		if (!node.HasMethod("apply_height_grid"))
		{
			node.QueueFree();
			return;
		}
		_worldRoot.AddChild(node);
		Godot.Collections.Array heightSamples = d.ContainsKey("height_samples") ? d["height_samples"].As<Godot.Collections.Array>() : new Godot.Collections.Array();
		node.Call("apply_height_grid", heightSamples, coord);
		_chunks[coord] = node;
	}

	private void ApplyChunkUnloaded(Godot.Collections.Dictionary d)
	{
		Vector2I coord = d.ContainsKey("coord") ? d["coord"].AsVector2I() : new Vector2I(int.MaxValue, int.MaxValue);
		if (coord.X == int.MaxValue && coord.Y == int.MaxValue)
			return;
		if (!_chunks.TryGetValue(coord, out Node node))
			return;
		_chunks.Remove(coord);
		SafeReleaseNode(node);
	}

	private void ApplyCellChanged(Godot.Collections.Dictionary d)
	{
		// Optional: incremental cell update; for now mesh is rebuilt on chunk load.
	}

	private void UnloadFarChunks(Vector2I centerChunk)
	{
		var toRemove = new List<Vector2I>();
		foreach (Vector2I coord in _chunks.Keys)
		{
			int dx = Mathf.Abs(coord.X - centerChunk.X);
			int dy = Mathf.Abs(coord.Y - centerChunk.Y);
			if (dx > StreamRadiusChunks || dy > StreamRadiusChunks)
				toRemove.Add(coord);
		}
		foreach (Vector2I coord in toRemove)
		{
			if (_chunks.TryGetValue(coord, out Node node))
			{
				_chunks.Remove(coord);
				SafeReleaseNode(node);
			}
		}
	}
}
