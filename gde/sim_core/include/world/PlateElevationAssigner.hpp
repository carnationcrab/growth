#pragma once

#include "../gen/WorldSeed.hpp"
#include "PlateClimate.hpp"
#include "PlateProperties.hpp"
#include "TectonicPlates.hpp"

namespace growth {

/// Assigns one elevation per plate (ocean = negative, continental = positive + small noise). No per-region work.
class PlateElevationAssigner {
public:
	void assign(const TectonicPlates &tectonic_plates, const PlateProperties &plate_properties, const WorldSeed &world_seed, PlateClimate &out) const;
};

} // namespace growth
