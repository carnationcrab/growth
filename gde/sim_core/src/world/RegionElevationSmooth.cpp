#include "world/RegionElevationSmooth.hpp"
#include "util/Parallel.hpp"
#include "base/gateway/Cmath.hpp"
#include "base/gateway/Cstddef.hpp"

namespace growth {

namespace {

constexpr float k_max_region_elevation_step = 0.14f;

void clamp_region_elevation_slope(const CsrGraph &region_neighbours, Vector<float> &region_elevation) {
	const Size num = region_elevation.size();
	if (num == 0 || region_neighbours.num_nodes() < num)
		return;

	Vector<float> scratch(num);
	for (int iter = 0; iter < 2; ++iter) {
		parallel::for_range(0, num, 1024, [&](Size r) {
			float target = region_elevation[r];
			const U32 *nb_b = region_neighbours.neighbours_begin(r);
			const U32 *nb_e = region_neighbours.neighbours_end(r);
			for (const U32 *it = nb_b; it != nb_e; ++it) {
				const Size nb = static_cast<Size>(*it);
				if (nb >= num)
					continue;
				const float nb_elev = region_elevation[nb];
				const float delta   = target - nb_elev;
				if (Cmath::abs(delta) > k_max_region_elevation_step) {
					const float sign = (delta > 0.0f) ? 1.0f : -1.0f;
					target           = nb_elev + sign * k_max_region_elevation_step;
				}
			}
			scratch[r] = target;
		});
		region_elevation.swap(scratch);
	}
}

} // namespace

void smooth_region_elevation_laplacian(const CsrGraph &region_neighbours, Vector<float> &region_elevation, int passes) {
	const Size num = region_elevation.size();
	if (num == 0 || region_neighbours.num_nodes() < num || passes < 1)
		return;

	Vector<float> scratch(num);
	for (int pass = 0; pass < passes; ++pass) {
		parallel::for_range(0, num, 1024, [&](Size r) {
			float sum  = region_elevation[r];
			Size count = 1;
			const U32 *nb_b = region_neighbours.neighbours_begin(r);
			const U32 *nb_e = region_neighbours.neighbours_end(r);
			for (const U32 *it = nb_b; it != nb_e; ++it) {
				const Size nb = static_cast<Size>(*it);
				if (nb >= num)
					continue;
				sum += region_elevation[nb];
				++count;
			}
			scratch[r] = sum / static_cast<float>(count);
		});
		region_elevation.swap(scratch);
	}
}

void smooth_dual_elevation_laplacian(const SphereTopology &topology,
                                     const CsrGraph &region_triangle_rings,
                                     Vector<float> &region_elevation,
                                     Vector<float> &triangle_elevation,
                                     int passes) {
	const Size num_regions = region_elevation.size();
	const Size num_tri     = triangle_elevation.size();
	if (num_regions == 0 || num_tri == 0 || passes < 1)
		return;
	if (region_triangle_rings.num_nodes() < num_regions)
		return;

	Vector<float> reg_scratch(num_regions);
	Vector<float> tri_scratch(num_tri);

	for (int pass = 0; pass < passes; ++pass) {
		parallel::for_range(0, num_tri, 4096, [&](Size t) {
			const auto &tr = topology.triangles[t];
			float sum      = triangle_elevation[t];
			Size count     = 1;
			for (int corner = 0; corner < 3; ++corner) {
				const Size r = tr[static_cast<Size>(corner)];
				if (r < num_regions) {
					sum += region_elevation[r];
					++count;
				}
			}
			tri_scratch[t] = sum / static_cast<float>(count);
		});

		parallel::for_range(0, num_regions, 1024, [&](Size r) {
			float sum  = region_elevation[r];
			Size count = 1;
			const U32 *b = region_triangle_rings.neighbours_begin(r);
			const U32 *e = region_triangle_rings.neighbours_end(r);
			for (const U32 *it = b; it != e; ++it) {
				const Size t = static_cast<Size>(*it);
				if (t < num_tri) {
					sum += tri_scratch[t];
					++count;
				}
			}
			reg_scratch[r] = sum / static_cast<float>(count);
		});

		region_elevation.swap(reg_scratch);
		triangle_elevation.swap(tri_scratch);
	}
}

void prepare_elevation_for_terrain_mesh(const SphereTopology &topology,
                                        const CsrGraph &region_neighbours,
                                        const CsrGraph &region_triangle_rings,
                                        Vector<float> &region_elevation,
                                        Vector<float> &triangle_elevation) {
	clamp_region_elevation_slope(region_neighbours, region_elevation);
	smooth_dual_elevation_laplacian(topology, region_triangle_rings, region_elevation, triangle_elevation, 4);
}

} // namespace growth
