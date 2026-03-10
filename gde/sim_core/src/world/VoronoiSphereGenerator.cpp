#include "world/Circumcenter.hpp"
#include "world/Delaunay2D.hpp"
#include "world/FibonacciSphere.hpp"
#include "world/VoronoiSphereGenerator.hpp"
#include "math/Vec3.hpp"
#include "util/Random.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

namespace growth {

namespace {

	// Stereographic projection from north pole: (x,y,z) on unit sphere -> (u,v) in plane.
	// (u,v) = (x/(1-z), y/(1-z)). Near pole we use tangent-plane coords so points stay distinct (avoids degenerate Delaunay).
	void stereographic(const Vec3 &p, float &u, float &v) {
		const float denom = 1.0f - p.z;
		if (denom <= 1e-6f) {
			// Near north pole: project onto tangent plane with scale so points don't collapse to (0,0).
			const float scale = 1e3f;
			u = p.x * scale;
			v = p.y * scale;
			return;
		}
		u = p.x / denom;
		v = p.y / denom;
	}

	using Edge = std::pair<size_t, size_t>;
	Edge ordered_edge(size_t a, size_t b) {
		return a < b ? Edge{a, b} : Edge{b, a};
	}

	/// Find the boundary cycle (convex hull). Boundary edges appear in exactly one triangle.
	/// Build adjacency so every boundary edge is kept; traverse by following "neighbour we didn't come from".
	bool boundary_cycle(const std::vector<std::array<size_t, 3>> &triangles,
	                   std::vector<size_t> &cycle_out) {
		cycle_out.clear();
		std::map<Edge, int> edge_count;
		for (const auto &t : triangles) {
			edge_count[ordered_edge(t[0], t[1])]++;
			edge_count[ordered_edge(t[1], t[2])]++;
			edge_count[ordered_edge(t[2], t[0])]++;
		}
		std::map<Edge, bool> is_boundary;
		for (const auto &kv : edge_count)
			if (kv.second == 1) is_boundary[kv.first] = true;
		if (is_boundary.empty()) return false;
		std::unordered_map<size_t, std::vector<size_t>> adj;
		for (const auto &t : triangles) {
			for (int i = 0; i < 3; ++i) {
				size_t a = t[i], b = t[(i + 1) % 3];
				if (is_boundary[ordered_edge(a, b)]) {
					adj[a].push_back(b);
					adj[b].push_back(a);
				}
			}
		}
		size_t start = adj.begin()->first;
		if (adj[start].size() < 2) return false;
		size_t prev = start;
		size_t cur = adj[start][0];
		cycle_out.push_back(start);
		while (cur != start && cycle_out.size() <= adj.size()) {
			cycle_out.push_back(cur);
			const auto &neigh = adj[cur];
			size_t next_cur = prev;
			for (size_t v : neigh)
				if (v != prev) { next_cur = v; break; }
			if (next_cur == prev) return false;
			prev = cur;
			cur = next_cur;
		}
		return (cur == start && cycle_out.size() >= 3);
	}

	/// Normalise 2D points into a bounded box for stable Delaunay.
	void normalise_projected(std::vector<Vec2> &projected) {
		const size_t nn = projected.size();
		if (nn == 0) return;
		float min_u = projected[0].x, max_u = projected[0].x, min_v = projected[0].y, max_v = projected[0].y;
		for (size_t i = 1; i < nn; ++i) {
			min_u = std::min(min_u, projected[i].x);
			max_u = std::max(max_u, projected[i].x);
			min_v = std::min(min_v, projected[i].y);
			max_v = std::max(max_v, projected[i].y);
		}
		const float range_u = max_u - min_u, range_v = max_v - min_v;
		const float range = (range_u > range_v ? range_u : range_v);
		const float scale_2d = (range > 1e-9f) ? (10.0f / range) : 1.0f;
		const float mid_u = (min_u + max_u) * 0.5f, mid_v = (min_v + max_v) * 0.5f;
		for (size_t i = 0; i < nn; ++i) {
			projected[i].x = (projected[i].x - mid_u) * scale_2d;
			projected[i].y = (projected[i].y - mid_v) * scale_2d;
		}
	}

	/// Max tangent-plane displacement at 100% jitter (fraction of radius); keeps Delaunay stable.
	const float k_max_jitter_radius = 0.25f;

	void apply_site_jitter(std::vector<Vec3> &sites, size_t num_to_jitter, uint64_t seed, float jitter_percent) {
		if (jitter_percent <= 0.0f || num_to_jitter == 0) return;
		const float scale = (jitter_percent / 100.0f) * k_max_jitter_radius;
		random::RNG rng(seed);
		for (size_t i = 0; i < num_to_jitter; ++i) {
			Vec3 &p = sites[i];
			Vec3 axis(0.0f, 0.0f, 1.0f);
			if (std::fabs(p.z) > 0.9f) axis = Vec3(1.0f, 0.0f, 0.0f);
			Vec3 u = p.cross(axis);
			if (u.length() < 1e-6f) continue;
			u = u.normalised();
			Vec3 v = p.cross(u).normalised();
			float r1 = static_cast<float>(rng.next_double() * 2.0 - 1.0);
			float r2 = static_cast<float>(rng.next_double() * 2.0 - 1.0);
			Vec3 offset = u * (r1 * scale) + v * (r2 * scale);
			p = (p + offset).normalised();
		}
	}
} // namespace

VoronoiSphere VoronoiSphereGenerator::generate(const WorldSeed &world_seed, size_t num_sites, float jitter_percent) const {
	std::cerr << "[VoronoiSphereGenerator] generate start (sites=" << num_sites << " jitter=" << jitter_percent << "%)\n";
	VoronoiSphere out;
	FibonacciSphere::fill(num_sites, out.sites);
	const size_t n = out.sites.size();
	std::cerr << "[VoronoiSphereGenerator] Fibonacci sites: " << n << "\n";

	if (jitter_percent > 0.0f)
		apply_site_jitter(out.sites, n, world_seed.value, jitter_percent);

	// 1) Project sites to 2D (stereographic) and normalise
	std::vector<Vec2> projected(n);
	for (size_t i = 0; i < n; ++i) {
		float u, v;
		stereographic(out.sites[i], u, v);
		projected[i].x = u;
		projected[i].y = v;
	}
	normalise_projected(projected);

	// 2) Delaunay on the plane (leaves one boundary = hole)
	std::vector<std::array<size_t, 3>> tri;
	Delaunay2D::triangulate(projected, tri);
	std::cerr << "[VoronoiSphereGenerator] Delaunay triangles (2D): " << tri.size() << "\n";

	if (tri.empty() && n >= 3) {
		for (size_t i = 1; i + 1 < n; ++i)
			tri.push_back({{0, i, i + 1}});
		std::cerr << "[VoronoiSphereGenerator] fallback fan triangles: " << tri.size() << "\n";
	}

	// 3) Delaunay triangles first (all indices 0..n-1)
	for (const auto &t : tri)
		out.triangles.push_back(t);

	// 4) Fill the hole: add cap vertex at centroid of boundary and fan triangles (Bowyer–Watson doesn't connect an extra point)
	std::vector<size_t> boundary;
	if (boundary_cycle(tri, boundary) && boundary.size() >= 3) {
		Vec3 centroid(0.0f, 0.0f, 0.0f);
		for (size_t idx : boundary)
			centroid = centroid + out.sites[idx];
		const float inv = 1.0f / static_cast<float>(boundary.size());
		centroid.x *= inv; centroid.y *= inv; centroid.z *= inv;
		const size_t cap_idx = out.sites.size();
		out.sites.push_back(centroid.normalised());
		for (size_t i = 0; i < boundary.size(); ++i) {
			size_t a = boundary[i];
			size_t b = boundary[(i + 1) % boundary.size()];
			out.triangles.push_back({{a, b, cap_idx}});
		}
		std::cerr << "[VoronoiSphereGenerator] hole filled: cap index " << cap_idx << ", " << boundary.size() << " fan triangles\n";
	}

	// 5) Voronoi vertices: one circumcenter (on sphere) per triangle.
	// Circumcenter formula can leave point slightly inside sphere; project onto surface (x/d, y/d, z/d).
	out.circumcenters.resize(out.triangles.size());
	for (size_t i = 0; i < out.triangles.size(); ++i) {
		const auto &t = out.triangles[i];
		const Vec3 a = out.sites[t[0]], b = out.sites[t[1]], c = out.sites[t[2]];
		if (!circumcenter_on_sphere(a, b, c, out.circumcenters[i]))
			out.circumcenters[i] = (a + b + c).normalised();
		float d = out.circumcenters[i].length();
		if (d > 1e-9f)
			out.circumcenters[i] = out.circumcenters[i] / d;
	}

	// 6) Edge -> triangle indices (for walking neighbour tris)
	using Edge = std::pair<size_t, size_t>;
	std::map<Edge, std::vector<size_t>> edge_to_tris;
	for (size_t i = 0; i < out.triangles.size(); ++i) {
		const auto &t = out.triangles[i];
		for (int e = 0; e < 3; ++e) {
			size_t u = t[e], v = t[(e + 1) % 3];
			if (u > v) std::swap(u, v);
			edge_to_tris[{u, v}].push_back(i);
		}
	}

	// 7) Per-site Voronoi cells: ordered circumcenter indices around each site.
	// Build site -> triangle indices once (O(triangles)) so we don't scan all triangles per site.
	std::vector<std::vector<size_t>> site_to_tris(out.sites.size());
	for (size_t i = 0; i < out.triangles.size(); ++i) {
		const auto &t = out.triangles[i];
		if (t[0] < out.sites.size()) site_to_tris[t[0]].push_back(i);
		if (t[1] < out.sites.size()) site_to_tris[t[1]].push_back(i);
		if (t[2] < out.sites.size()) site_to_tris[t[2]].push_back(i);
	}
	out.cells.resize(out.sites.size());
	for (size_t s = 0; s < out.sites.size(); ++s) {
		std::vector<size_t> &cell = out.cells[s];
		if (site_to_tris[s].empty()) continue;
		const size_t start_tri = site_to_tris[s][0];
		// Walk triangles around s
		size_t cur_tri = start_tri;
		size_t leave_v = static_cast<size_t>(-1); // vertex we go to (other side of edge from s)
		{
			const auto &t = out.triangles[cur_tri];
			if (t[0] == s) { leave_v = t[1]; }
			else if (t[1] == s) { leave_v = t[2]; }
			else { leave_v = t[0]; }
		}
		do {
			cell.push_back(cur_tri);
			Edge e(std::min(s, leave_v), std::max(s, leave_v));
			const auto &neighbours = edge_to_tris[e];
			size_t next_tri = cur_tri;
			for (size_t nb : neighbours) {
				if (nb != cur_tri) { next_tri = nb; break; }
			}
			if (next_tri == cur_tri) break; // boundary edge
			// In next tri, the vertex that is not s and not leave_v is the new leave_v
			const auto &t = out.triangles[next_tri];
			size_t new_leave = static_cast<size_t>(-1);
			for (int i = 0; i < 3; ++i) {
				if (t[i] != s && t[i] != leave_v) { new_leave = t[i]; break; }
			}
			cur_tri = next_tri;
			leave_v = new_leave;
		} while (cur_tri != start_tri);
	}

	// 8) Drop any site not referenced by any triangle (phantom points from degeneracy or duplicates)
	std::set<size_t> used_sites;
	for (const auto &t : out.triangles) {
		used_sites.insert(t[0]);
		used_sites.insert(t[1]);
		used_sites.insert(t[2]);
	}
	if (used_sites.size() < out.sites.size()) {
		const size_t num_removed = out.sites.size() - used_sites.size();
		std::vector<size_t> used_list(used_sites.begin(), used_sites.end());
		std::unordered_map<size_t, size_t> old_to_new;
		for (size_t i = 0; i < used_list.size(); ++i)
			old_to_new[used_list[i]] = i;
		std::vector<Vec3> new_sites(used_list.size());
		for (size_t i = 0; i < used_list.size(); ++i)
			new_sites[i] = out.sites[used_list[i]];
		std::vector<std::array<size_t, 3>> new_triangles(out.triangles.size());
		for (size_t i = 0; i < out.triangles.size(); ++i) {
			const auto &t = out.triangles[i];
			new_triangles[i] = {{old_to_new[t[0]], old_to_new[t[1]], old_to_new[t[2]]}};
		}
		std::vector<std::vector<size_t>> new_cells(used_list.size());
		for (size_t i = 0; i < used_list.size(); ++i)
			new_cells[i] = std::move(out.cells[used_list[i]]);
		out.sites = std::move(new_sites);
		out.triangles = std::move(new_triangles);
		out.cells = std::move(new_cells);
		std::cerr << "[VoronoiSphereGenerator] removed " << num_removed << " phantom site(s)\n";
	}

	std::cerr << "[VoronoiSphereGenerator] done: sites=" << out.sites.size() << " triangles=" << out.triangles.size()
		<< " circumcenters=" << out.circumcenters.size() << " cells=" << out.cells.size() << "\n";
	return out;
}

} // namespace growth
