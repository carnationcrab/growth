#pragma once

#include "../gen/WorldSeed.hpp"
#include "../gen/PlanetGenome.hpp"
#include "PlanetGlobe.hpp"
#include <cstddef>

namespace growth {

/// Single entry point for building a full planet: Voronoi sphere, tectonics, elevation, moisture; later biomes, rivers.
/// Uses VoronoiSphereGenerator for the reusable sphere mesh; all planet-specific layers live here.
class PlanetGlobeGenerator {
public:
	/// Run the full pipeline: (1) Voronoi sphere, (2) tectonic plates, (3) plate properties, (4) elevation, (5) moisture.
	/// Fills out.voronoi, out.plates, out.elevation, out.moisture. num_sites = Voronoi site count; jitter_percent 0–100; num_plate_regions = plate count (e.g. 10–50).
	/// temperature_01 and precipitation_01 are 0–1 climate multipliers (e.g. form sliders 0–200 / 200); higher temperature slightly lowers moisture.
	void run(const WorldSeed &world_seed, const PlanetGenome &planet_genome, size_t num_sites, double jitter_percent, size_t num_plate_regions, float temperature_01, float precipitation_01, PlanetGlobe &out) const;
};

} // namespace growth
