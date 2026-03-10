#pragma once

#include "../gen/WorldSeed.hpp"
#include "../gen/PlanetGenome.hpp"
#include "PlanetGlobe.hpp"
#include <cstddef>

namespace growth {

struct PlanetTerrainMesh;

/// Single entry point for building a full planet: Voronoi sphere, tectonics, elevation, moisture, rivers, optional planet terrain mesh.
/// Uses VoronoiSphereGenerator for the reusable sphere mesh; all planet-specific layers live here.
class PlanetGlobeGenerator {
public:
	/// Run the full pipeline: (1) Voronoi sphere, (2) half-edge mesh, (3) tectonic plates, (4) plate properties,
	/// (5) elevation, (6) moisture, (7) triangle values, (8) river downflow and flow.
	/// If use_planet_terrain_mesh is true and out_planet_terrain_mesh is not null, generates the planet terrain mesh (quad, river/ridge) into *out_planet_terrain_mesh.
	void run(const WorldSeed &world_seed, const PlanetGenome &planet_genome, size_t num_sites, double jitter_percent, size_t num_plate_regions, float temperature_01, float precipitation_01, PlanetGlobe &out, bool use_planet_terrain_mesh = false, PlanetTerrainMesh *out_planet_terrain_mesh = nullptr) const;
};

} // namespace growth
