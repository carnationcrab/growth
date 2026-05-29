#pragma once

#include "Types.hpp"
#include "base/gateway/Cstddef.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

/// Result of tectonic plate assignment: each Voronoi region (site) belongs to one plate.
struct TectonicPlates {
	/// region_to_plate[site_idx] = plate id (0 .. num_plates - 1).
	Vector<int32_t> region_to_plate;
	/// Voronoi region index used as the BFS seed for each plate (same convention as RBG r_plate[r] === r).
	Vector<U32> plate_seed_region;
	size_t num_plates = 0;
};

} // namespace growth
