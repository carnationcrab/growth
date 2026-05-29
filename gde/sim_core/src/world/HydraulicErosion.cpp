#include "world/HydraulicErosion.hpp"
#include "util/Parallel.hpp"
#include "base/gateway/Cstddef.hpp"

namespace growth {

namespace {

constexpr size_t k_downflow_unset = SphereHalfEdgeMesh::k_invalid;

size_t trunk_triangle_from_downflow_side(const SphereHalfEdgeMesh &mesh, size_t flow_s) {
	if (flow_s == k_downflow_unset)
		return SphereHalfEdgeMesh::k_invalid;
	const size_t outer_t = mesh.s_outer_t[flow_s];
	return outer_t;
}

} // namespace

void apply_hydraulic_erosion(
	const SphereHalfEdgeMesh &mesh,
	const RiverFlow &flow,
	Vector<float> &triangle_elevation) {
	const size_t num_tri = mesh.num_triangles();
	if (flow.order_t.empty() || flow.t_downflow_s.size() != num_tri)
		return;

	for (size_t i = flow.order_t.size(); i > 0; --i) {
		const size_t tributary_t = flow.order_t[i - 1];
		if (tributary_t >= num_tri)
			continue;
		const size_t flow_s = flow.t_downflow_s[tributary_t];
		if (flow_s == k_downflow_unset)
			continue;
		const size_t trunk_t = trunk_triangle_from_downflow_side(mesh, flow_s);
		if (trunk_t == SphereHalfEdgeMesh::k_invalid || trunk_t >= num_tri)
			continue;
		const float tributary_e = triangle_elevation[tributary_t];
		if (triangle_elevation[trunk_t] > tributary_e)
			triangle_elevation[trunk_t] = tributary_e;
	}
}

void sync_region_elevation_from_triangles(
	const SphereTopology &topology,
	const Vector<float> &triangle_elevation,
	Vector<float> &region_elevation) {
	const size_t num_tri     = topology.triangles.size();
	const size_t num_regions = topology.sites.size();
	if (num_regions == 0 || triangle_elevation.size() < num_tri)
		return;

	region_elevation.assign(num_regions, 0.0f);
	Vector<uint32_t> counts(num_regions, 0u);

	for (size_t t = 0; t < num_tri; ++t) {
		const float te = triangle_elevation[t];
		const auto &tr = topology.triangles[t];
		for (int corner = 0; corner < 3; ++corner) {
			const size_t r = tr[static_cast<size_t>(corner)];
			if (r >= num_regions)
				continue;
			region_elevation[r] += te;
			counts[r] += 1u;
		}
	}

	parallel::for_range(0, num_regions, 4096, [&](size_t r) {
		if (counts[r] > 0u)
			region_elevation[r] /= static_cast<float>(counts[r]);
	});
}

} // namespace growth
