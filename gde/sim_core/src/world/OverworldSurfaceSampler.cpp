#include "world/OverworldSurfaceSampler.hpp"
#include "base/gateway/Cmath.hpp"
#include "math/Scalar.hpp"

namespace growth {

namespace {

float region_temperature(Vec3 unit_site, float elevation) {
	const float lat = clamp(unit_site.z, -1.0f, 1.0f);
	return clamp(Cmath::cos(lat * constants::half_pi) - elevation * 0.6f, -1.0f, 1.0f);
}

} // namespace

U32 OverworldSurfaceSampler::locate_region(const PlanetSurfaceAtlas &atlas, Vec3 unit_dir) {
	if (atlas.sites.empty())
		return 0;

	unit_dir = unit_dir.normalised();
	float best_dot = -2.0f;
	U32 best = 0;
	for (U32 i = 0; i < atlas.region_count; ++i) {
		const float d = unit_dir.dot(atlas.sites[i]);
		if (d > best_dot) {
			best_dot = d;
			best = i;
		}
	}
	return best;
}

SurfaceSample OverworldSurfaceSampler::sample_region(const PlanetSurfaceAtlas &atlas, U32 region_id) {
	SurfaceSample out;
	if (atlas.region_count == 0)
		return out;

	if (region_id >= atlas.region_count)
		region_id = 0;

	out.region_id = region_id;
	if (region_id < atlas.region_to_plate.size())
		out.plate_id = atlas.region_to_plate[region_id];
	if (region_id < atlas.region_elevation.size())
		out.elevation = atlas.region_elevation[region_id];
	if (region_id < atlas.region_moisture.size())
		out.moisture = atlas.region_moisture[region_id];

	const Vec3 site = atlas.sites[region_id];
	out.temperature = region_temperature(site, out.elevation);
	return out;
}

SurfaceSample OverworldSurfaceSampler::sample_unit_dir(const PlanetSurfaceAtlas &atlas, Vec3 unit_dir) {
	const U32 region_id = locate_region(atlas, unit_dir);
	return sample_region(atlas, region_id);
}

} // namespace growth
