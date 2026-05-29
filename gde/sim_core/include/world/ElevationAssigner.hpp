#pragma once

#include "../gen/WorldSeed.hpp"
#include "../util/CsrGraph.hpp"
#include "PlateProperties.hpp"
#include "RegionElevation.hpp"
#include "SphereTopology.hpp"
#include "TectonicPlates.hpp"

namespace growth {

/// Assigns per-region elevation from plate collisions and distance fields. Deterministic from
/// WorldSeed; uses parallel deterministic BFS over the shared region-neighbour CsrGraph.
class ElevationAssigner {
public:
	void assign(const SphereTopology &topology,
	            const CsrGraph &region_neighbours,
	            const TectonicPlates &tectonic_plates,
	            const PlateProperties &plate_properties,
	            const WorldSeed &world_seed,
	            RegionElevation &out) const;
};

} // namespace growth
