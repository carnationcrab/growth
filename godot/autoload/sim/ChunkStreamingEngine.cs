using Godot;
using System.Collections.Generic;

/// <summary>
/// Chunk request / diff polling with stub fallback when GrowthSim is unavailable.
/// </summary>
public sealed class ChunkStreamingEngine
{
	public const string ChunkLoaded = "ChunkLoaded";
	public const string ChunkUnloaded = "ChunkUnloaded";
	public const string CellChanged = "CellChanged";

	private readonly List<Godot.Collections.Dictionary> _stubDiffQueue = new();
	private readonly Dictionary<Vector2I, bool> _stubRequested = new();

	public void ResetStub()
	{
		_stubDiffQueue.Clear();
		_stubRequested.Clear();
	}

	public void RequestChunks(GrowthSimBackend backend, Vector2I centerChunk, int radius)
	{
		if (backend.IsCpp)
		{
			backend.Call("request_chunks", centerChunk, radius);
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

	public Godot.Collections.Array PollDiffs(GrowthSimBackend backend, int maxCount)
	{
		if (backend.IsCpp)
		{
			var raw = backend.Call("poll_diffs", maxCount).As<Godot.Collections.Array>();
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

	private static Godot.Collections.Dictionary StubMakeChunkLoaded(Vector2I coord)
	{
		const int side = 17;
		var heightSamples = new Godot.Collections.Array();
		heightSamples.Resize(side * side);
		for (int i = 0; i < side * side; i++)
			heightSamples[i] = 0.0f;
		return new Godot.Collections.Dictionary
		{
			["type"] = ChunkLoaded,
			["coord"] = coord,
			["height_samples"] = heightSamples
		};
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
}
