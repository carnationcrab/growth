using Godot;

/// <summary>
/// Normalises world-gen results from GrowthSim for GDScript/C# consumers (array copies, packed types).
/// </summary>
public sealed class WorldGenResultEngine
{
	/// <summary>Run world gen from form; returns Dictionary with marshalled arrays for GDScript.</summary>
	public Godot.Collections.Dictionary ApplyForm(GrowthSimBackend backend, Godot.Collections.Dictionary formDict)
	{
		if (backend.IsCpp)
		{
			var v = backend.Call("apply_world_gen_form", formDict);
			var dict = v.As<Godot.Collections.Dictionary>();
			if (dict != null)
				return NormaliseResult(backend, dict);
		}

		GD.PrintErr("[SimAPI] apply_world_gen_form: stub backend — build the GDExtension (cd gde; .\\build.ps1) and restart Godot.");
		var empty = new Godot.Collections.Dictionary();
		empty[WorldGenPreviewKeys.Sites] = System.Array.Empty<Vector3>();
		empty[WorldGenPreviewKeys.Triangles] = System.Array.Empty<int>();
		empty[WorldGenPreviewKeys.Ok] = false;
		empty[WorldGenPreviewKeys.Error] = "GrowthSim GDExtension not loaded";
		return empty;
	}

	private static Godot.Collections.Dictionary NormaliseResult(
		GrowthSimBackend backend,
		Godot.Collections.Dictionary dict)
	{
		var result = new Godot.Collections.Dictionary();
		result[WorldGenPreviewKeys.Sites] = dict[WorldGenPreviewKeys.Sites];
		// Fetch triangles via direct method return (PackedInt32Array marshals as int[]); copy to Array for GDScript
		var triRet = backend.Call("get_last_world_gen_triangles");
		var triInts = triRet.As<int[]>();
		if (triInts != null && triInts.Length > 0)
		{
			var arr = new Godot.Collections.Array();
			foreach (var x in triInts) arr.Add(x);
			result[WorldGenPreviewKeys.Triangles] = arr;
		}
		else
		{
			var triArr = triRet.As<Godot.Collections.Array>();
			result[WorldGenPreviewKeys.Triangles] = triArr != null ? triArr : new Godot.Collections.Array();
		}
		// Voronoi: copy circumcenters into Array so C#/GDScript receive it reliably (same as triangles)
		if (dict.ContainsKey(WorldGenPreviewKeys.Circumcenters))
		{
			var cc = dict["circumcenters"].As<Vector3[]>();
			if (cc != null && cc.Length > 0)
			{
				var ccArr = new Godot.Collections.Array();
				foreach (var vec in cc) ccArr.Add(vec);
				result[WorldGenPreviewKeys.Circumcenters] = ccArr;
			}
			else
				result[WorldGenPreviewKeys.Circumcenters] = dict[WorldGenPreviewKeys.Circumcenters];
		}
		if (dict.ContainsKey(WorldGenPreviewKeys.Cells))
			result[WorldGenPreviewKeys.Cells] = dict[WorldGenPreviewKeys.Cells];
		if (dict.ContainsKey(WorldGenPreviewKeys.PlateRegions))
		{
			var pr = dict[WorldGenPreviewKeys.PlateRegions].As<int[]>();
			if (pr != null && pr.Length > 0)
			{
				var prArr = new Godot.Collections.Array();
				foreach (var x in pr) prArr.Add(x);
				result[WorldGenPreviewKeys.PlateRegions] = prArr;
				GD.Print("[SimAPI] apply_world_gen_form: plate_regions forwarded size=", pr.Length);
			}
			else
			{
				result[WorldGenPreviewKeys.PlateRegions] = dict[WorldGenPreviewKeys.PlateRegions];
				GD.Print("[SimAPI] apply_world_gen_form: plate_regions forwarded (raw)");
			}
		}
		else
			GD.Print("[SimAPI] apply_world_gen_form: no plate_regions in result");
		if (dict.ContainsKey(WorldGenPreviewKeys.Ok))
			result[WorldGenPreviewKeys.Ok] = dict[WorldGenPreviewKeys.Ok];
		if (dict.ContainsKey(WorldGenPreviewKeys.Error))
			result[WorldGenPreviewKeys.Error] = dict[WorldGenPreviewKeys.Error];
		if (dict.ContainsKey(WorldGenPreviewKeys.LogLines))
			result[WorldGenPreviewKeys.LogLines] = dict[WorldGenPreviewKeys.LogLines];
		if (dict.ContainsKey(WorldGenPreviewKeys.PerfLines))
			result[WorldGenPreviewKeys.PerfLines] = dict[WorldGenPreviewKeys.PerfLines];
		if (dict.ContainsKey(WorldGenPreviewKeys.PlanetPreset))
			result[WorldGenPreviewKeys.PlanetPreset] = dict[WorldGenPreviewKeys.PlanetPreset];
		if (dict.ContainsKey(WorldGenPreviewKeys.PlateElevation))
		{
			var elev = dict[WorldGenPreviewKeys.PlateElevation].As<float[]>();
			if (elev != null && elev.Length > 0)
			{
				var elevArr = new Godot.Collections.Array();
				foreach (var x in elev) elevArr.Add(x);
				result[WorldGenPreviewKeys.PlateElevation] = elevArr;
			}
			else
				result[WorldGenPreviewKeys.PlateElevation] = dict[WorldGenPreviewKeys.PlateElevation];
		}
		if (dict.ContainsKey(WorldGenPreviewKeys.PlateMoisture))
		{
			var moist = dict[WorldGenPreviewKeys.PlateMoisture].As<float[]>();
			if (moist != null && moist.Length > 0)
			{
				var moistArr = new Godot.Collections.Array();
				foreach (var x in moist) moistArr.Add(x);
				result[WorldGenPreviewKeys.PlateMoisture] = moistArr;
			}
			else
				result[WorldGenPreviewKeys.PlateMoisture] = dict[WorldGenPreviewKeys.PlateMoisture];
		}
		ForwardFloatArray(dict, result, WorldGenPreviewKeys.RegionElevation);
		ForwardFloatArray(dict, result, WorldGenPreviewKeys.RegionMoisture);
		ForwardFloatArray(dict, result, WorldGenPreviewKeys.TriangleElevation);
		ForwardFloatArray(dict, result, WorldGenPreviewKeys.TriangleMoisture);
		if (dict.ContainsKey(WorldGenPreviewKeys.UsePlanetTerrainMesh))
			result[WorldGenPreviewKeys.UsePlanetTerrainMesh] = dict[WorldGenPreviewKeys.UsePlanetTerrainMesh];
		ForwardVector3Array(dict, result, WorldGenPreviewKeys.PlanetTerrainMeshVertices);
		ForwardVector3Array(dict, result, WorldGenPreviewKeys.PlanetTerrainMeshNormals);
		ForwardIntArray(dict, result, WorldGenPreviewKeys.PlanetTerrainMeshIndices);
		ForwardFloatArray(dict, result, WorldGenPreviewKeys.PlanetTerrainMeshRiverStrength);
		ForwardFloatArray(dict, result, WorldGenPreviewKeys.PlanetTerrainMeshVertexElevation);
		ForwardFloatArray(dict, result, WorldGenPreviewKeys.PlanetTerrainMeshVertexMoisture);
		if (dict.ContainsKey(WorldGenPreviewKeys.PlanetTerrainMeshNumRegions))
			result[WorldGenPreviewKeys.PlanetTerrainMeshNumRegions] = dict[WorldGenPreviewKeys.PlanetTerrainMeshNumRegions];
		ForwardIntArray(dict, result, WorldGenPreviewKeys.TopologyTriangles);
		ForwardVector3Array(dict, result, WorldGenPreviewKeys.PlanetTerrainMeshPreErosionVertices);
		ForwardVector3Array(dict, result, WorldGenPreviewKeys.PlanetTerrainMeshPreErosionNormals);
		ForwardVector3Array(dict, result, WorldGenPreviewKeys.PlanetTerrainMeshRiverLines);
		ForwardIntArray(dict, result, WorldGenPreviewKeys.PlanetTerrainMeshTriangleRegion);
		ForwardIntArray(dict, result, WorldGenPreviewKeys.MeshSBeginR);
		ForwardIntArray(dict, result, WorldGenPreviewKeys.MeshSEndR);
		ForwardIntArray(dict, result, WorldGenPreviewKeys.MeshSInnerT);
		ForwardIntArray(dict, result, WorldGenPreviewKeys.MeshSOuterT);
		ForwardIntArray(dict, result, WorldGenPreviewKeys.MeshSNextS);
		ForwardIntArray(dict, result, WorldGenPreviewKeys.MeshSTwinS);
		return result;
	}

	private static void ForwardFloatArray(Godot.Collections.Dictionary dict, Godot.Collections.Dictionary result, string key)
	{
		if (!dict.ContainsKey(key)) return;
		var variant = dict[key];
		if (variant.VariantType == Variant.Type.Array
		    || variant.As<float[]>() is { Length: > 0 }
		    || variant.As<double[]>() is { Length: > 0 })
		{
			result[key] = variant;
			return;
		}
		result[key] = dict[key];
	}

	private static void ForwardIntArray(Godot.Collections.Dictionary dict, Godot.Collections.Dictionary result, string key)
	{
		if (!dict.ContainsKey(key)) return;
		var variant = dict[key];
		if (variant.VariantType == Variant.Type.Array || variant.As<int[]>() is { Length: > 0 })
		{
			result[key] = variant;
			return;
		}
		result[key] = dict[key];
	}

	private static void ForwardVector3Array(Godot.Collections.Dictionary dict, Godot.Collections.Dictionary result, string key)
	{
		if (!dict.ContainsKey(key)) return;
		var variant = dict[key];
		if (variant.VariantType == Variant.Type.Array || variant.As<Vector3[]>() is { Length: > 0 })
		{
			result[key] = variant;
			return;
		}
		result[key] = dict[key];
	}
}
