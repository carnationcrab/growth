#pragma once

#include "../gen/WorldSeed.hpp"
#include "../util/CsrGraph.hpp"
#include "SphereTopology.hpp"
#include "TectonicPlates.hpp"
#include "base/gateway/Cstddef.hpp"

namespace growth {

/// Assigns each region to a tectonic plate. Deterministic given WorldSeed and the input
/// adjacency: picks num_plate_regions seeds via the RNG stream, then runs a multi-source
/// parallel BFS labelling (closest seed by graph distance; ties broken by smaller plate id).
class TectonicPlateAssigner {
public:
	void assign(const SphereTopology &topology,
	            const CsrGraph &region_neighbours,
	            const WorldSeed &world_seed,
	            size_t num_plate_regions,
	            TectonicPlates &out) const;
};

} // namespace growth
