#pragma once

#include "math/Vec3.hpp"
#include <cstddef>
#include <vector>

namespace growth {

/// Per-plate properties for elevation: movement direction and ocean/continental flag.
struct PlateProperties {
	/// Unit movement vector per plate (plate_id 0 .. num_plates-1).
	std::vector<Vec3> plate_vector;
	/// True if plate is oceanic (basin), false if continental.
	std::vector<bool> plate_is_ocean;
	size_t num_plates = 0;
};

} // namespace growth
