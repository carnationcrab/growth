#pragma once

#include "../data/DefDatabase.hpp"
#include "../gen/WorldSeed.hpp"
#include "../gen/PlanetGenome.hpp"
#include "PlanetGlobe.hpp"
#include "base/gateway/Cstddef.hpp"
#include "base/gateway/Cstring.hpp"

namespace growth {

struct PlanetTerrainMesh;

namespace perf {
class PerformanceLogger;
}

/// Thin entry point: runs the data-driven PlanetGlobePipeline (icosphere topology + CSR adjacency).
class PlanetGlobeGenerator {
public:
	void run(const WorldSeed &world_seed,
	         const PlanetGenome &planet_genome,
	         size_t num_sites,
	         double jitter_percent,
	         size_t num_plate_regions,
	         float temperature_01,
	         float precipitation_01,
	         PlanetGlobe &out,
	         bool use_planet_terrain_mesh = false,
	         PlanetTerrainMesh *out_planet_terrain_mesh = nullptr,
	         const DefDatabase *defs = nullptr,
	         const String &pipeline_id = "default_globe",
	         perf::PerformanceLogger *perf = nullptr,
	         size_t sim_num_sites = 0) const;
};

} // namespace growth
