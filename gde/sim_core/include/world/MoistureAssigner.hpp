#pragma once

#include "../gen/WorldSeed.hpp"
#include "RegionElevation.hpp"
#include "RegionMoisture.hpp"
#include "TectonicPlates.hpp"
#include "VoronoiSphere.hpp"

namespace growth {

/// Assigns per-region moisture from plate and elevation. Deterministic from WorldSeed.
/// Applies climate multipliers: higher temperature_01 slightly lowers moisture; higher precipitation_01 raises it.
class MoistureAssigner {
public:
	/// Fill out.region_moisture. temperature_01 and precipitation_01 are 0–1 (e.g. from form sliders 0–200 / 200).
	/// Higher temperature_01 reduces moisture slightly; higher precipitation_01 increases it.
	void assign(const VoronoiSphere &voronoi_sphere, const TectonicPlates &tectonic_plates, const RegionElevation &region_elevation, const WorldSeed &world_seed, float temperature_01, float precipitation_01, RegionMoisture &out) const;
};

} // namespace growth
