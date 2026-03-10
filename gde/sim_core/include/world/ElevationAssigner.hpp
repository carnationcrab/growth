#pragma once

#include "../gen/WorldSeed.hpp"
#include "PlateProperties.hpp"
#include "RegionElevation.hpp"
#include "TectonicPlates.hpp"
#include "VoronoiSphere.hpp"

namespace growth {

/// Assigns per-region elevation from plate collisions and distance fields. Deterministic from WorldSeed.
class ElevationAssigner {
public:
	/// Fill out.region_elevation from voronoi_sphere, tectonic_plates, and plate_properties.
	void assign(const VoronoiSphere &voronoi_sphere, const TectonicPlates &tectonic_plates, const PlateProperties &plate_properties, const WorldSeed &world_seed, RegionElevation &out) const;
};

} // namespace growth
