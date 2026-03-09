#pragma once

#include "../gen/WorldSeed.hpp"
#include "TectonicPlates.hpp"
#include "VoronoiSphere.hpp"
#include <cstddef>

namespace growth {

/// Assigns each Voronoi region to a tectonic plate via random plate seeds and random fill.
class TectonicPlateAssigner {
public:
	/// Assign plates: picks num_plates random region seeds (10–50 typical), then random-fill from them.
	/// Fills tectonic_out.region_to_plate and tectonic_out.num_plates. Uses world_seed for deterministic RNG.
	void assign(const VoronoiSphere &voronoi_sphere, const WorldSeed &world_seed, size_t num_plates, TectonicPlates &tectonic_out) const;
};

} // namespace growth
