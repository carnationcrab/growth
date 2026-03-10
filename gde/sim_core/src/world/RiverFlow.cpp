#include "world/RiverFlow.hpp"
#include <algorithm>
#include <cstddef>
#include <vector>

namespace growth {

void assign_downflow(
	const SphereHalfEdgeMesh &mesh,
	const std::vector<float> &triangle_elevation,
	RiverFlow &out
) {
	const size_t num_tri = mesh.num_triangles();
	const size_t num_sides = mesh.num_sides();
	out.t_downflow_s.resize(num_tri);
	out.s_flow.resize(num_sides, 0.0f);

	for (size_t t = 0; t < num_tri; ++t) {
		float e_t = (t < triangle_elevation.size()) ? triangle_elevation[t] : 0.0f;
		size_t best_s = RiverFlow::k_invalid;
		float best_elev = e_t;

		for (int e = 0; e < 3; ++e) {
			size_t s = t * 3u + static_cast<size_t>(e);
			size_t outer_t = mesh.s_outer_t[s];
			if (outer_t == SphereHalfEdgeMesh::k_invalid)
				continue;
			float e_outer = (outer_t < triangle_elevation.size()) ? triangle_elevation[outer_t] : 0.0f;
			if (e_outer < best_elev) {
				best_elev = e_outer;
				best_s = s;
			}
		}
		out.t_downflow_s[t] = best_s;
	}
}

void assign_flow(
	const SphereHalfEdgeMesh &mesh,
	const RiverFlow &downflow,
	const std::vector<float> &triangle_elevation,
	RiverFlow &out
) {
	const size_t num_tri = mesh.num_triangles();
	const size_t num_sides = mesh.num_sides();
	out.t_downflow_s = downflow.t_downflow_s;
	out.s_flow.resize(num_sides, 0.0f);

	// Sort triangle indices by elevation descending (high to low).
	std::vector<size_t> order(num_tri);
	for (size_t t = 0; t < num_tri; ++t)
		order[t] = t;
	std::sort(order.begin(), order.end(), [&triangle_elevation, num_tri](size_t a, size_t b) {
		float ea = (a < triangle_elevation.size()) ? triangle_elevation[a] : 0.0f;
		float eb = (b < triangle_elevation.size()) ? triangle_elevation[b] : 0.0f;
		return ea > eb;
	});

	// Per-triangle accumulated flow (starts at 1, accumulates downstream).
	std::vector<float> t_flow(num_tri, 1.0f);

	for (size_t idx = 0; idx < num_tri; ++idx) {
		size_t t = order[idx];
		size_t s = downflow.t_downflow_s[t];
		if (s == RiverFlow::k_invalid)
			continue;
		size_t outer_t = mesh.s_outer_t[s];
		if (outer_t == SphereHalfEdgeMesh::k_invalid)
			continue;
		t_flow[outer_t] += t_flow[t];
	}

	// Per-edge river strength: for each half-edge s, flow through this edge is the flow from inner_t when it drains through s, plus from outer_t when it drains through twin.
	for (size_t s = 0; s < num_sides; ++s) {
		size_t t_inner = mesh.s_inner_t[s];
		size_t twin = mesh.s_twin_s[s];
		size_t t_outer = (twin != SphereHalfEdgeMesh::k_invalid) ? mesh.s_inner_t[twin] : num_tri;
		float flow = 0.0f;
		if (t_inner < num_tri && downflow.t_downflow_s[t_inner] == s)
			flow += t_flow[t_inner];
		if (twin != SphereHalfEdgeMesh::k_invalid && t_outer < num_tri && downflow.t_downflow_s[t_outer] == twin)
			flow += t_flow[t_outer];
		out.s_flow[s] = flow;
	}
}

} // namespace growth
