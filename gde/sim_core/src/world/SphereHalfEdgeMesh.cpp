#include "world/SphereHalfEdgeMesh.hpp"
#include "base/gateway/Cmap.hpp"
#include "base/gateway/Cutility.hpp"

namespace growth {

namespace {

using Edge = Pair<size_t, size_t>;

Edge ordered_edge(size_t a, size_t b) {
	return a < b ? Edge{a, b} : Edge{b, a};
}

Vec3 normalise(Vec3 v) {
	const float l = v.length();
	return (l > 0.0f) ? (v / l) : v;
}

// Shared wiring: connects half-edges from a generic triangle list. The caller is responsible
// for populating mesh.r_xyz and mesh.t_xyz before invoking.
template <typename TriList>
void wire_half_edges(const TriList &triangles, SphereHalfEdgeMesh &mesh) {
	const size_t num_tri   = triangles.size();
	const size_t num_sides = num_tri * 3u;
	mesh.s_begin_r.resize(num_sides);
	mesh.s_end_r.resize(num_sides);
	mesh.s_inner_t.resize(num_sides);
	mesh.s_outer_t.resize(num_sides);
	mesh.s_next_s.resize(num_sides);
	mesh.s_twin_s.resize(num_sides);

	Map<Edge, Vector<Pair<size_t, int>>> edge_to_tri_and_e;
	for (size_t t = 0; t < num_tri; ++t) {
		const auto &tr = triangles[t];
		for (int e = 0; e < 3; ++e) {
			const size_t u = tr[e];
			const size_t v = tr[(e + 1) % 3];
			edge_to_tri_and_e[ordered_edge(u, v)].emplace_back(t, e);
		}
	}

	for (size_t t = 0; t < num_tri; ++t) {
		const auto &tr = triangles[t];
		for (int e = 0; e < 3; ++e) {
			const size_t s = t * 3u + static_cast<size_t>(e);
			const size_t u = tr[e];
			const size_t v = tr[(e + 1) % 3];
			mesh.s_begin_r[s] = u;
			mesh.s_end_r[s]   = v;
			mesh.s_inner_t[s] = t;
			mesh.s_next_s[s]  = t * 3u + static_cast<size_t>((e + 1) % 3);

			const auto &candidates = edge_to_tri_and_e[ordered_edge(u, v)];
			size_t outer = SphereHalfEdgeMesh::k_invalid;
			size_t twin  = SphereHalfEdgeMesh::k_invalid;
			for (const auto &c : candidates) {
				if (c.first == t) continue;
				const auto &tr_other = triangles[c.first];
				const int e2              = c.second;
				const size_t other_begin  = tr_other[static_cast<size_t>(e2)];
				const size_t other_end    = tr_other[static_cast<size_t>((e2 + 1) % 3)];
				if (other_begin == v && other_end == u) {
					outer = c.first;
					twin  = c.first * 3u + static_cast<size_t>(e2);
					break;
				}
			}
			mesh.s_outer_t[s] = outer;
			mesh.s_twin_s[s]  = twin;
		}
	}

	for (size_t s = 0; s < num_sides; ++s) {
		const size_t t = mesh.s_twin_s[s];
		if (t != SphereHalfEdgeMesh::k_invalid && mesh.s_twin_s[t] == SphereHalfEdgeMesh::k_invalid)
			mesh.s_twin_s[t] = s;
	}
}

} // namespace

void build_sphere_half_edge_mesh(const SphereTopology &topology, SphereHalfEdgeMesh &mesh) {
	mesh.r_xyz = topology.sites;
	const size_t num_tri = topology.triangles.size();
	mesh.t_xyz.resize(num_tri);
	for (size_t t = 0; t < num_tri; ++t) {
		const auto &tr = topology.triangles[t];
		const Vec3 a = topology.sites[tr[0]];
		const Vec3 b = topology.sites[tr[1]];
		const Vec3 c = topology.sites[tr[2]];
		mesh.t_xyz[t] = normalise(Vec3((a.x + b.x + c.x) / 3.0f,
		                                (a.y + b.y + c.y) / 3.0f,
		                                (a.z + b.z + c.z) / 3.0f));
	}
	wire_half_edges(topology.triangles, mesh);
}

void build_sphere_half_edge_mesh(const VoronoiSphere &voronoi, SphereHalfEdgeMesh &mesh) {
	mesh.r_xyz = voronoi.sites;
	mesh.t_xyz = voronoi.circumcenters;
	wire_half_edges(voronoi.triangles, mesh);
}

} // namespace growth
