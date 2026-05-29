#pragma once

#include "../gen/WorldSeed.hpp"
#include "../util/CsrGraph.hpp"
#include "PlateProperties.hpp"
#include "SphereTopology.hpp"
#include "TectonicPlates.hpp"
#include "base/gateway/Cstddef.hpp"

namespace growth {

/// Builds per-plate properties (Euler-pole angular velocity + oceanic flag). Deterministic.
class PlatePropertiesAssigner {
public:
	void assign(const SphereTopology &topology,
	            const CsrGraph &region_neighbours,
	            const TectonicPlates &tectonic_plates,
	            const WorldSeed &world_seed,
	            PlateProperties &out) const;
};

} // namespace growth
