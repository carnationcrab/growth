#pragma once

#include <cstddef>
#include <vector>

namespace growth {

/// Per-plate elevation and moisture (one value per tectonic plate). Lightweight; no per-region data.
struct PlateClimate {
	std::vector<float> plate_elevation;
	std::vector<float> plate_moisture;
	size_t num_plates = 0;
};

} // namespace growth
