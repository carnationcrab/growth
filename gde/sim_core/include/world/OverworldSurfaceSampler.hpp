#pragma once

#include "Types.hpp"
#include "math/Vec3.hpp"
#include "world/PlanetSurfaceAtlas.hpp"

namespace growth {

/// Result of sampling one overworld region (temperature derived, not stored).
struct SurfaceSample {
	U32 region_id = 0;
	I32 plate_id = 0;
	float elevation = 0.0f;
	float moisture = 0.0f;
	float temperature = 0.0f;
	U32 biome_id = 0;
};

/// Locate and sample overworld surface fields (single responsibility).
struct OverworldSurfaceSampler {
	static U32 locate_region(const PlanetSurfaceAtlas &atlas, Vec3 unit_dir);
	static SurfaceSample sample_region(const PlanetSurfaceAtlas &atlas, U32 region_id);
	static SurfaceSample sample_unit_dir(const PlanetSurfaceAtlas &atlas, Vec3 unit_dir);
};

} // namespace growth
