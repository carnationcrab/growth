#pragma once
#include "Types.hpp"

#include "../data/DefDatabase.hpp"
#include "../gen/PlanetGenome.hpp"
#include "../gen/WorldSeed.hpp"
#include "PlanetGlobe.hpp"
#include "base/gateway/Catomic.hpp"

namespace growth {

struct PlanetTerrainMesh;

namespace perf {
class PerformanceLogger;
}

/// Optional progress and cancellation for pipeline runs (async world gen).
struct PlanetGlobePipelineProgress {
	Catomic::Atomic<bool> *cancel_flag = nullptr;

	void (*on_stage_begin)(void *userdata, const char *stage_id, int stage_index, int stage_count) = nullptr;
	void *userdata = nullptr;
};

/// Context passed through each world-gen pipeline stage.
struct PlanetGlobePipelineContext {
	const WorldSeed *world_seed = nullptr;
	const PlanetGenome *planet_genome = nullptr;
	Size num_sites = 0;
	/// When > 0 and less than num_sites, run tectonics/climate on coarse grid then upsample fields.
	Size sim_num_sites = 0;
	bool coarse_sim_applied = false;
	double jitter_percent = 0.0;
	Size num_plate_regions = 0;
	double temperature_01   = 0.5;
	double precipitation_01 = 0.5;
	PlanetGlobe *globe = nullptr;
	bool use_planet_terrain_mesh = false;
	PlanetTerrainMesh *planet_terrain_mesh = nullptr;
	perf::PerformanceLogger *perf = nullptr;
	PlanetGlobePipelineProgress *progress = nullptr;

	bool use_coarse_sim_upsample() const {
		return sim_num_sites > 0 && num_sites > sim_num_sites;
	}
};

/// Runs ordered stages from DefDatabase pipeline definition.
class PlanetGlobePipeline {
public:
	/// Returns false if cancelled between stages.
	bool run(const DefDatabase &defs, const String &pipeline_id, PlanetGlobePipelineContext &ctx) const;
};

} // namespace growth
