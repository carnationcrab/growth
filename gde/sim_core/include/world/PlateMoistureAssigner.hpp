#pragma once

#include "../gen/WorldSeed.hpp"
#include "PlateClimate.hpp"
#include "TectonicPlates.hpp"

namespace growth {

/// Assigns one moisture per plate (RNG + temperature/precipitation multipliers). No per-region work.
class PlateMoistureAssigner {
public:
	void assign(const TectonicPlates &tectonic_plates, const WorldSeed &world_seed, float temperature_01, float precipitation_01, PlateClimate &out) const;
};

} // namespace growth
