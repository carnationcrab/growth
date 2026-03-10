#pragma once

#include <cstddef>
#include <vector>

namespace growth {

/// Per-region moisture (one value per Voronoi site). Typically 0–1.
struct RegionMoisture {
	std::vector<float> region_moisture;
};

} // namespace growth
