#include "base/gateway/Cstddef.hpp"
#include "base/gateway/Cvector.hpp"
#pragma once


namespace growth {

/// Per-plate elevation and moisture (one value per tectonic plate). Lightweight; no per-region data.
struct PlateClimate {
	Vector<float> plate_elevation;
	Vector<float> plate_moisture;
	size_t num_plates = 0;
};

} // namespace growth
