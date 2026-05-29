#include "base/gateway/Cstddef.hpp"
#include "base/gateway/Cvector.hpp"
#pragma once


namespace growth {

/// Per-region elevation (one value per Voronoi site). Negative = ocean, positive = land.
struct RegionElevation {
	Vector<float> region_elevation;
};

} // namespace growth
