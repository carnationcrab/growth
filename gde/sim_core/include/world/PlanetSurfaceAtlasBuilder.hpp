#pragma once

#include "world/PlanetGlobe.hpp"
#include "world/PlanetSurfaceAtlas.hpp"

namespace growth {

/// Builds a PlanetSurfaceAtlas from a completed PlanetGlobe (single responsibility).
struct PlanetSurfaceAtlasBuilder {
	static void build_from_globe(const PlanetGlobe &globe, PlanetSurfaceAtlas &out);
};

} // namespace growth
