#include "world/RiverCarve.hpp"
#include "world/RiverFlow.hpp"
#include "world/TriangleValues.hpp"
#include "util/CsrGraph.hpp"
#include "util/Parallel.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

namespace {

constexpr float k_carve_strength = 0.045f;

void smooth_carved_regions(const CsrGraph &neighbours, Vector<float> &elev) {
	const Size num = elev.size();
	if (num == 0 || neighbours.num_nodes() < num)
		return;
	Vector<float> scratch(num);
	for (int pass = 0; pass < 2; ++pass) {
		parallel::for_range(0, num, 1024, [&](Size r) {
			float sum  = elev[r];
			Size count = 1;
			const U32 *nb_b = neighbours.neighbours_begin(r);
			const U32 *nb_e = neighbours.neighbours_end(r);
			for (const U32 *it = nb_b; it != nb_e; ++it) {
				const Size nb = static_cast<Size>(*it);
				if (nb >= num)
					continue;
				sum += elev[nb];
				++count;
			}
			scratch[r] = sum / static_cast<float>(count);
		});
		elev.swap(scratch);
	}
}

// Keep triangle heights consistent with region averages (RBG assignTriangleValues uses mean, not min).
void refresh_triangle_elevation(const SphereTopology &topology,
                                const Vector<float> &region_elev,
                                Vector<float> &triangle_elev_out) {
	const Size num_tri = topology.triangles.size();
	const Size num_reg = region_elev.size();
	triangle_elev_out.resize(num_tri);
	parallel::for_range(0, num_tri, 1024, [&](Size t) {
		const auto &tr = topology.triangles[t];
		const Size r0 = tr[0];
		const Size r1 = tr[1];
		const Size r2 = tr[2];
		const float e0 = (r0 < num_reg) ? region_elev[r0] : 0.0f;
		const float e1 = (r1 < num_reg) ? region_elev[r1] : 0.0f;
		const float e2 = (r2 < num_reg) ? region_elev[r2] : 0.0f;
		triangle_elev_out[t] = (e0 + e1 + e2) * (1.0f / 3.0f);
	});
}

} // namespace

void carve_rivers_from_flow(PlanetGlobe &globe) {
	Vector<float> &elev            = globe.region_elevation.region_elevation;
	const SphereHalfEdgeMesh &mesh = globe.mesh;
	const RiverFlow &river         = globe.river_flow;
	const Size num_regions         = mesh.num_regions();
	const Size num_sides           = mesh.num_sides();
	if (elev.size() < num_regions || river.s_flow.size() != num_sides)
		return;

	const float threshold = river_flow_visual_threshold(river.s_flow);
	if (threshold >= 1e8f)
		return;

	float max_flow = 0.0f;
	for (float f : river.s_flow) {
		if (f > max_flow)
			max_flow = f;
	}
	const float flow_span = max_flow - threshold;
	if (flow_span <= 1e-9f)
		return;

	// Per-region carve total via a single serial sweep over sides above threshold, then apply
	// it in parallel. Deterministic regardless of worker count.
	Vector<float> carve(num_regions, 0.0f);
	for (Size s = 0; s < num_sides; ++s) {
		const float flow = river.s_flow[s];
		if (flow < threshold)
			continue;
		const float tn    = (flow - threshold) / flow_span;
		const float depth = k_carve_strength * tn;
		const Size begin  = mesh.s_begin_r[s];
		const Size end    = mesh.s_end_r[s];
		if (begin < num_regions)
			carve[begin] += depth;
		if (end < num_regions)
			carve[end] += depth;
	}
	parallel::for_range(0, num_regions, 1024, [&](Size r) {
		elev[r] -= carve[r];
	});

	if (!globe.region_neighbours.empty())
		smooth_carved_regions(globe.region_neighbours, elev);

	refresh_triangle_elevation(globe.topology, elev, globe.triangle_values.triangle_elevation);
}

} // namespace growth
