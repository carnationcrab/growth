using Godot;

/// <summary>
/// RBG-style colour ramps for planet map preview layers (1843-planet-generation / mapgen4 palette).
/// </summary>
public static class PlanetMapPalette
{
	private static readonly Vector3 PreviewLightDir = new Vector3(0.35f, 0.85f, 0.4f).Normalized();

	/// <summary>Sim Z-up unit site; elevation in roughly -1..1.</summary>
	public static float RegionTemperature(Vector3 unitSiteSim, float elevation)
	{
		float lat = Mathf.Clamp(unitSiteSim.Z, -1f, 1f);
		return Mathf.Clamp(Mathf.Cos(lat * Mathf.Pi * 0.5f) - elevation * 0.6f, -1f, 1f);
	}

	public static Color ElevationColor(float elevation)
	{
		if (elevation < 0f)
		{
			float depth = Mathf.Clamp(-elevation, 0f, 1f);
			var deep = new Color(0.02f, 0.06f, 0.18f);
			var shelf = new Color(0.22f, 0.52f, 0.78f);
			return deep.Lerp(shelf, Mathf.Pow(1f - depth, 0.55f));
		}

		float t = Mathf.Clamp(elevation, 0f, 1f);
		var low = new Color(0.42f, 0.52f, 0.38f);
		var mid = new Color(0.62f, 0.58f, 0.38f);
		var high = new Color(0.88f, 0.84f, 0.78f);
		var peak = new Color(0.97f, 0.97f, 0.94f);
		if (t < 0.35f)
			return low.Lerp(mid, t / 0.35f);
		if (t < 0.72f)
			return mid.Lerp(high, (t - 0.35f) / 0.37f);
		return high.Lerp(peak, (t - 0.72f) / 0.28f);
	}

	public static Color MoistureColor(float moisture)
	{
		float m = Mathf.Clamp(moisture, 0f, 1f);
		var dry = new Color(0.55f, 0.42f, 0.28f);
		var wet = new Color(0.18f, 0.48f, 0.72f);
		return dry.Lerp(wet, m);
	}

	public static Color TemperatureColor(float temperature)
	{
		float t = (temperature + 1f) * 0.5f;
		var cold = new Color(0.15f, 0.35f, 0.75f);
		var mild = new Color(0.35f, 0.65f, 0.35f);
		var hot = new Color(0.85f, 0.45f, 0.18f);
		if (t < 0.5f)
			return cold.Lerp(mild, t * 2f);
		return mild.Lerp(hot, (t - 0.5f) * 2f);
	}

	public static Color BiomeColor(float elevation, float moisture, float temperature)
	{
		if (elevation < 0f)
			return ElevationColor(elevation);

		if (elevation > 0.72f)
			return new Color(0.96f, 0.96f, 0.93f);

		float m = Mathf.Clamp(moisture, 0f, 1f);
		float temp = (temperature + 1f) * 0.5f;

		// Game biomes from data/core/defs/biomes.xml (preview approximation).
		if (temp > 0.58f && m < 0.38f)
			return new Color(0.72f, 0.62f, 0.36f); // arid_shrubland
		if (temp >= 0.32f && temp <= 0.68f && m >= 0.42f)
			return new Color(0.24f, 0.52f, 0.26f); // temperate_forest
		if (temp < 0.32f)
			return m > 0.45f ? new Color(0.55f, 0.68f, 0.58f) : new Color(0.72f, 0.74f, 0.7f);
		if (m > 0.55f)
			return new Color(0.28f, 0.58f, 0.28f);
		if (m < 0.3f)
			return new Color(0.68f, 0.6f, 0.38f);
		return new Color(0.48f, 0.56f, 0.36f);
	}

	public static Color ApplySoftShading(Color baseColor, Vector3 normal)
	{
		float ndl = Mathf.Max(0f, normal.Dot(PreviewLightDir));
		float shade = 0.58f + 0.42f * ndl;
		return new Color(baseColor.R * shade, baseColor.G * shade, baseColor.B * shade);
	}

	public static Color RiverLineColor => new Color(0.04f, 0.12f, 0.42f, 0.92f);
}
