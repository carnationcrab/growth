#pragma once

#include "../gen/WorldSeed.hpp"
#include "../util/CsrGraph.hpp"
#include "RegionElevation.hpp"
#include "RegionMoisture.hpp"
#include "SphereTopology.hpp"
#include "TectonicPlates.hpp"

namespace growth {

/// Assigns per-region moisture using ocean-distance BFS and climate sliders. Deterministic.
class MoistureAssigner {
public:
	void assign(const SphereTopology &topology,
	            const CsrGraph &region_neighbours,
	            const TectonicPlates &tectonic_plates,
	            const RegionElevation &region_elevation,
	            const WorldSeed &world_seed,
	            double temperature_01,
	            double precipitation_01,
	            RegionMoisture &out) const;
};

} // namespace growth
