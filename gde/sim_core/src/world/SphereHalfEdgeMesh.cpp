#include "world/SphereHalfEdgeMesh.hpp"
#include <map>
#include <utility>

namespace growth {

namespace {

using Edge = std::pair<size_t, size_t>;

Edge ordered_edge(size_t a, size_t b) {
	return a < b ? Edge{a, b} : Edge{b, a};
}

} // namespace

void build_sphere_half_edge_mesh(const VoronoiSphere &voronoi, SphereHalfEdgeMesh &mesh) {
	mesh.r_xyz = voronoi.sites;
	mesh.t_xyz = voronoi.circumcenters;

	const size_t num_tri = voronoi.triangles.size();
	const size_t num_sides = num_tri * 3u;
	mesh.s_begin_r.resize(num_sides);
	mesh.s_end_r.resize(num_sides);
	mesh.s_inner_t.resize(num_sides);
	mesh.s_outer_t.resize(num_sides);
	mesh.s_next_s.resize(num_sides);
	mesh.s_twin_s.resize(num_sides);

	// Edge (min, max) -> list of (triangle, edge_index) so we can find outer_t and twin.
	std::map<Edge, std::vector<std::pair<size_t, int>>> edge_to_tri_and_e;
	for (size_t t = 0; t < num_tri; ++t) {
		const auto &tr = voronoi.triangles[t];
		for (int e = 0; e < 3; ++e) {
			size_t u = tr[e];
			size_t v = tr[(e + 1) % 3];
			edge_to_tri_and_e[ordered_edge(u, v)].emplace_back(t, e);
		}
	}

	for (size_t t = 0; t < num_tri; ++t) {
		const auto &tr = voronoi.triangles[t];
		for (int e = 0; e < 3; ++e) {
			size_t s = t * 3u + static_cast<size_t>(e);
			size_t u = tr[e];
			size_t v = tr[(e + 1) % 3];
			mesh.s_begin_r[s] = u;
			mesh.s_end_r[s] = v;
			mesh.s_inner_t[s] = t;
			mesh.s_next_s[s] = t * 3u + static_cast<size_t>((e + 1) % 3);

			const auto &candidates = edge_to_tri_and_e[ordered_edge(u, v)];
			size_t outer = SphereHalfEdgeMesh::k_invalid;
			size_t twin = SphereHalfEdgeMesh::k_invalid;
			for (const auto &c : candidates) {
				if (c.first == t) continue;
				const auto &tr_other = voronoi.triangles[c.first];
				int e2 = c.second;
				size_t other_begin = tr_other[static_cast<size_t>(e2)];
				size_t other_end = tr_other[static_cast<size_t>((e2 + 1) % 3)];
				// Twin is the half-edge in the opposite direction (v -> u), not (u -> v).
				if (other_begin == v && other_end == u) {
					outer = c.first;
					twin = c.first * 3u + static_cast<size_t>(e2);
					break;
				}
			}
			mesh.s_outer_t[s] = outer;
			mesh.s_twin_s[s] = twin;
		}
	}

	// Twin was set for one direction; set twin for the other direction when we have a pair.
	for (size_t s = 0; s < num_sides; ++s) {
		size_t t = mesh.s_twin_s[s];
		if (t != SphereHalfEdgeMesh::k_invalid && mesh.s_twin_s[t] == SphereHalfEdgeMesh::k_invalid)
			mesh.s_twin_s[t] = s;
	}
}

} // namespace growth
