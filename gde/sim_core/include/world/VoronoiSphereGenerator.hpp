#pragma once

#include "VoronoiSphere.hpp"
#include "../gen/WorldSeed.hpp"

namespace growth {

/// Produces reusable sphere topology: Fibonacci sites on the unit sphere (and later Delaunay).
/// No planet-specific logic; used by PlanetGlobeGenerator and reusable for other globes.
class VoronoiSphereGenerator {
public:
	/// Generate sphere with num_sites Fibonacci sites (and Delaunay). Deterministic.
	VoronoiSphere generate(const WorldSeed &world_seed, size_t num_sites) const;
};

} // namespace growth
