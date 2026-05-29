using Godot;

/// <summary>
/// Sim (Z-up unit sphere) ↔ Godot (Y-up) presentation transforms. Owned by the view layer only;
/// sim_core stays Z-up and does not target a specific renderer.
/// </summary>
public static class PresentationCoords
{
	/// <summary>Rotation −90° about X: sim +Z (north) → Godot +Y. Orientation-preserving (det +1).</summary>
	public static Vector3 SimToGodot(Vector3 sim) => new(sim.X, sim.Z, -sim.Y);

	/// <summary>Unit direction in sim space → unit direction in Godot space.</summary>
	public static Vector3 SimDirectionToGodot(Vector3 simUnitDir)
	{
		Vector3 g = SimToGodot(simUnitDir);
		return g.LengthSquared() > 1e-12f ? g.Normalized() : Vector3.Up;
	}

	/// <summary>
	/// Sim-space outward test (matches worldgen_bench), then flip all triangles once for Godot CCW front faces.
	/// </summary>
	public static int[] PrepareTerrainMeshIndicesForDisplay(int[] indices, Vector3[] godotVertices)
	{
		int[] simOutward = EnsureOutwardWindingSimSpace(indices, godotVertices);
		return FlipAllTriangleWindings(simOutward);
	}

	/// <summary>
	/// Per triangle: geometric normal should point away from planet centre (origin).</summary>
	public static int[] EnsureOutwardWindingSimSpace(int[] indices, Vector3[] godotVertices)
	{
		if (indices == null || indices.Length < 3 || godotVertices == null || godotVertices.Length == 0)
			return indices;

		var outIdx = new int[indices.Length];
		System.Array.Copy(indices, outIdx, indices.Length);
		int numTris = outIdx.Length / 3;
		int numVerts = godotVertices.Length;
		for (int t = 0; t < numTris; t++)
		{
			int i0 = outIdx[t * 3];
			int i1 = outIdx[t * 3 + 1];
			int i2 = outIdx[t * 3 + 2];
			if ((uint)i0 >= (uint)numVerts || (uint)i1 >= (uint)numVerts || (uint)i2 >= (uint)numVerts)
				continue;
			Vector3 a = godotVertices[i0];
			Vector3 b = godotVertices[i1];
			Vector3 c = godotVertices[i2];
			Vector3 centre = (a + b + c) * (1f / 3f);
			Vector3 n = (b - a).Cross(c - a);
			if (n.LengthSquared() < 1e-12f)
				continue;
			if (n.Dot(centre) < 0f)
				(outIdx[t * 3 + 1], outIdx[t * 3 + 2]) = (outIdx[t * 3 + 2], outIdx[t * 3 + 1]);
		}
		return outIdx;
	}

	/// <summary>
	/// Godot treats the opposite triangle chirality as front-facing vs sim geometric outward (det +1 SimToGodot).
	/// </summary>
	public static int[] FlipAllTriangleWindings(int[] indices)
	{
		if (indices == null || indices.Length < 3)
			return indices;
		var flipped = new int[indices.Length];
		for (int t = 0; t < indices.Length; t += 3)
		{
			flipped[t] = indices[t];
			flipped[t + 1] = indices[t + 2];
			flipped[t + 2] = indices[t + 1];
		}
		return flipped;
	}

	/// <summary>Alias for callers that only need the sim-space pass.</summary>
	public static int[] EnsureOutwardWinding(int[] indices, Vector3[] godotVertices)
		=> EnsureOutwardWindingSimSpace(indices, godotVertices);
}
