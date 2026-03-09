#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace growth {

/// Result of tectonic plate assignment: each Voronoi region (site) belongs to one plate.
struct TectonicPlates {
	/// region_to_plate[site_idx] = plate id (0 .. num_plates - 1).
	std::vector<int32_t> region_to_plate;
	size_t num_plates = 0;
};

} // namespace growth
