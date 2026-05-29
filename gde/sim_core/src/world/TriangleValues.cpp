#include "world/TriangleValues.hpp"
#include "util/Parallel.hpp"

namespace growth {

void assign_triangle_values_from_regions(
	const SphereTopology &topology,
	const RegionElevation &region_elevation,
	const RegionMoisture &region_moisture,
	TriangleValues &out
) {
	const size_t num_tri        = topology.triangles.size();
	const Vector<float> &r_elev = region_elevation.region_elevation;
	const Vector<float> &r_moist = region_moisture.region_moisture;
	const size_t num_regions    = r_elev.size();

	out.triangle_elevation.resize(num_tri);
	out.triangle_moisture.resize(num_tri);

	parallel::for_range(0, num_tri, 4096, [&](size_t t) {
		const auto &tr = topology.triangles[t];
		const size_t r0 = tr[0];
		const size_t r1 = tr[1];
		const size_t r2 = tr[2];
		const float e0 = (r0 < num_regions) ? r_elev[r0] : 0.0f;
		const float e1 = (r1 < num_regions) ? r_elev[r1] : 0.0f;
		const float e2 = (r2 < num_regions) ? r_elev[r2] : 0.0f;
		out.triangle_elevation[t] = (e0 + e1 + e2) * (1.0f / 3.0f);
		const float m0 = (r0 < num_regions) ? r_moist[r0] : 0.0f;
		const float m1 = (r1 < num_regions) ? r_moist[r1] : 0.0f;
		const float m2 = (r2 < num_regions) ? r_moist[r2] : 0.0f;
		out.triangle_moisture[t] = (m0 + m1 + m2) * (1.0f / 3.0f);
	});
}

} // namespace growth
