#pragma once

#include "RegionElevation.hpp"
#include "RegionMoisture.hpp"
#include "SphereTopology.hpp"
#include "base/gateway/Cstddef.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

/// Per-triangle elevation and moisture (average of the three region values).
struct TriangleValues {
	Vector<float> triangle_elevation;
	Vector<float> triangle_moisture;
};

void assign_triangle_values_from_regions(
	const SphereTopology &topology,
	const RegionElevation &region_elevation,
	const RegionMoisture &region_moisture,
	TriangleValues &out
);

} // namespace growth
