#include "world/PlanetGlobeGenerator.hpp"
#include "world/PlanetGlobePipeline.hpp"
#include "world/PlanetTerrainMesh.hpp"
#include "util/Parallel.hpp"

namespace growth {

void PlanetGlobeGenerator::run(const WorldSeed &world_seed,
                               const PlanetGenome &planet_genome,
                               const size_t num_sites,
                               const double jitter_percent,
                               const size_t num_plate_regions,
                               const float temperature_01,
                               const float precipitation_01,
                               PlanetGlobe &out,
                               const bool use_planet_terrain_mesh,
                               PlanetTerrainMesh *out_planet_terrain_mesh,
                               const DefDatabase *defs,
                               const String &pipeline_id,
                               perf::PerformanceLogger *perf,
                               const size_t sim_num_sites) const {
	(void)planet_genome;

	static DefDatabase k_empty_defs;
	const DefDatabase &db = defs != nullptr ? *defs : k_empty_defs;

	PlanetGlobePipelineContext ctx;
	ctx.world_seed              = &world_seed;
	ctx.planet_genome           = &planet_genome;
	ctx.num_sites               = num_sites;
	ctx.sim_num_sites           = sim_num_sites;
	ctx.jitter_percent          = jitter_percent;
	ctx.num_plate_regions       = num_plate_regions;
	ctx.temperature_01          = static_cast<double>(temperature_01);
	ctx.precipitation_01        = static_cast<double>(precipitation_01);
	ctx.globe                   = &out;
	ctx.use_planet_terrain_mesh = use_planet_terrain_mesh;
	ctx.planet_terrain_mesh     = out_planet_terrain_mesh;
	ctx.perf                    = perf;

	PlanetGlobePipeline pipeline;
	pipeline.run(db, pipeline_id.empty() ? String("default_globe") : pipeline_id, ctx);
}

} // namespace growth
