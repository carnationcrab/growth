#pragma once

#include "ParsedWorldGenForm.hpp"
#include "../gen/WorldSeed.hpp"
#include "../gen/PlanetGenome.hpp"

namespace growth {

/// Runs seed generation and planet generation from parsed form. No UI/platform types.
class WorldGenRunner {
public:
	/// Produce WorldSeed and PlanetGenome from parsed form. Deterministic; applies temperature/precipitation to genome.
	void run(const ParsedWorldGenForm &form, WorldSeed &world_seed_out, PlanetGenome &planet_genome_out) const;
};

} // namespace growth
