#pragma once

#include "../gen/WorldSeed.hpp"
#include "PlateProperties.hpp"
#include "TectonicPlates.hpp"
#include "VoronoiSphere.hpp"
#include <cstddef>

namespace growth {

/// Assigns per-plate movement vector and ocean/continental flag. Deterministic from WorldSeed.
class PlatePropertiesAssigner {
public:
	/// Fill out.plate_vector and out.plate_is_ocean from voronoi_sphere and tectonic_plates.
	void assign(const VoronoiSphere &voronoi_sphere, const TectonicPlates &tectonic_plates, const WorldSeed &world_seed, PlateProperties &out) const;
};

} // namespace growth
