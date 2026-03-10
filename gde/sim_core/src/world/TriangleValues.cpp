#include "world/TriangleValues.hpp"

namespace growth {

void assign_triangle_values_from_regions(
	const VoronoiSphere &voronoi,
	const RegionElevation &region_elevation,
	const RegionMoisture &region_moisture,
	TriangleValues &out
) {
	const size_t num_tri = voronoi.triangles.size();
	const std::vector<float> &r_elev = region_elevation.region_elevation;
	const std::vector<float> &r_moist = region_moisture.region_moisture;
	const size_t num_regions = r_elev.size();

	out.triangle_elevation.resize(num_tri);
	out.triangle_moisture.resize(num_tri);

	for (size_t t = 0; t < num_tri; ++t) {
		const auto &tr = voronoi.triangles[t];
		size_t r0 = tr[0], r1 = tr[1], r2 = tr[2];
		float e0 = (r0 < num_regions) ? r_elev[r0] : 0.0f;
		float e1 = (r1 < num_regions) ? r_elev[r1] : 0.0f;
		float e2 = (r2 < num_regions) ? r_elev[r2] : 0.0f;
		out.triangle_elevation[t] = (e0 + e1 + e2) / 3.0f;
		float m0 = (r0 < num_regions) ? r_moist[r0] : 0.0f;
		float m1 = (r1 < num_regions) ? r_moist[r1] : 0.0f;
		float m2 = (r2 < num_regions) ? r_moist[r2] : 0.0f;
		out.triangle_moisture[t] = (m0 + m1 + m2) / 3.0f;
	}
}

} // namespace growth
