#pragma once

#include <cstddef>
#include <vector>

namespace growth {

/// Per-region elevation (one value per Voronoi site). Negative = ocean, positive = land.
struct RegionElevation {
	std::vector<float> region_elevation;
};

} // namespace growth
