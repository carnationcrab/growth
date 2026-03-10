#pragma once

#include "RegionElevation.hpp"
#include "RegionMoisture.hpp"
#include "VoronoiSphere.hpp"
#include <cstddef>
#include <vector>

namespace growth {

/// Per-triangle elevation and moisture (average of the three region values).
/// triangle_elevation[t] = (r_elev[r0] + r_elev[r1] + r_elev[r2]) / 3; same for moisture.
struct TriangleValues {
	std::vector<float> triangle_elevation;
	std::vector<float> triangle_moisture;
};

/// Fill triangle_elevation and triangle_moisture from region data (average of three corners per triangle).
void assign_triangle_values_from_regions(
	const VoronoiSphere &voronoi,
	const RegionElevation &region_elevation,
	const RegionMoisture &region_moisture,
	TriangleValues &out
);

} // namespace growth
