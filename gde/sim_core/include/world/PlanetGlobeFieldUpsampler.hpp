#pragma once

#include "PlanetGlobe.hpp"
#include "gen/PlanetGenome.hpp"
#include "gen/WorldSeed.hpp"

namespace growth {

namespace perf {
class PerformanceLogger;
}

/// Coarse simulation at low site count, nearest-neighbour upsample onto a fine icosphere globe.
class PlanetGlobeFieldUpsampler {
public:
	/// Fine globe must already have topology, mesh, neighbours, and rings at render resolution.
	static void run_coarse_sim_and_upsample(
		const WorldSeed &world_seed,
		const PlanetGenome &planet_genome,
		Size coarse_sites,
		double jitter_percent,
		Size num_plate_regions,
		double temperature_01,
		double precipitation_01,
		PlanetGlobe &fine_globe,
		perf::PerformanceLogger *perf = nullptr);
};

} // namespace growth
