#pragma once

#include "VoronoiSphere.hpp"
#include "../gen/WorldSeed.hpp"

namespace growth {

/// Produces reusable sphere topology: Fibonacci sites on the unit sphere (and later Delaunay).
/// No planet-specific logic; used by PlanetGlobeGenerator and reusable for other globes.
class VoronoiSphereGenerator {
public:
	/// Generate sphere with num_sites Fibonacci sites (and Delaunay). Deterministic.
	/// jitter_percent: 0–100; applies tangent-plane displacement to sites for variety (0 = none).
	VoronoiSphere generate(const WorldSeed &world_seed, size_t num_sites, float jitter_percent = 0.0f) const;
};

} // namespace growth
