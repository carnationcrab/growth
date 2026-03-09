#pragma once

#include "../gen/WorldSeed.hpp"
#include "../gen/PlanetGenome.hpp"
#include "VoronoiSphere.hpp"

namespace growth {

/// Single entry point for building a full planet: Voronoi sphere topology then noise, tectonics, biomes, rivers, etc.
/// Uses VoronoiSphereGenerator for the reusable sphere mesh; all planet-specific layers live here.
class PlanetGlobeGenerator {
public:
	/// Run the full pipeline: (1) Voronoi sphere, (2) noise, (3) tectonics/plates, (4) biomes, (5) rivers, etc.
	/// Fills voronoi_sphere_out with the generated Fibonacci sites (and Delaunay). num_sites is the Voronoi site count.
	/// jitter_percent: 0–100, site displacement for variety (0 = none).
	void run(const WorldSeed &world_seed, const PlanetGenome &planet_genome, VoronoiSphere &voronoi_sphere_out, size_t num_sites, double jitter_percent = 0.0) const;
};

} // namespace growth
