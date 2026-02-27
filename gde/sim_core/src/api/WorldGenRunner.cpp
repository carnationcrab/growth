// World-gen form submit pipeline (2/4): from ParsedWorldGenForm produce WorldSeed + PlanetGenome (seed gen -> blueprint -> planet gen -> apply form sliders).
#include "api/WorldGenRunner.hpp"
#include "gen/SeedGenerator.hpp"
#include "gen/PlanetBlueprint.hpp"
#include "gen/PlanetGenerator.hpp"
#include <algorithm>
#include <cmath>

namespace growth {

namespace {
	void apply_form_to_genome(PlanetGenome &g, double form_temperature, double form_precipitation) {
		double p = form_precipitation / 200.0;
		p = std::max(0.0, std::min(1.0, p));
		g.precipitation = 0.1 + 0.9 * p;
		double t = form_temperature / 200.0;
		t = std::max(0.0, std::min(1.0, t));
		g.S_eff *= (0.7 + 0.6 * t);
	}
}

void WorldGenRunner::run(const ParsedWorldGenForm &form, WorldSeed &world_seed_out, PlanetGenome &planet_genome_out) const {
	// Seed: terrain/world deterministic seed from form keys (or 0 if empty).
	SeedGenerator gen;
	world_seed_out = form.seed_form.empty() ? gen.from_seed(0) : gen.from_form(form.seed_form);
	// Blueprint: preset (Earthlike/OceanWorld/Dry) + seed for sampling.
	PlanetGenBlueprint bp;
	bp.seed = world_seed_out.value;
	set_blueprint_to_preset(bp, form.planet_preset);
	// Planet genome: sample blueprint, apply habitability constraints; then apply form temperature/precipitation sliders.
	PlanetGenerator planet_gen;
	planet_genome_out = planet_gen.generate_from_blueprint(bp);
	apply_form_to_genome(planet_genome_out, form.temperature, form.precipitation);
}

} // namespace growth
