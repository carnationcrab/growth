#include "world/Delaunay2D.hpp"
#include "world/FibonacciSphere.hpp"
#include "world/VoronoiSphereGenerator.hpp"
#include "math/Vec3.hpp"
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>

namespace growth {

namespace {

	// Stereographic projection from north pole: (x,y,z) on unit sphere -> (u,v) in plane.
	// (u,v) = (x/(1-z), y/(1-z)). North hemisphere -> inside unit circle; south -> outside.
	void stereographic(const Vec3 &p, float &u, float &v) {
		const float denom = 1.0f - p.z;
		if (denom <= 1e-6f) { u = 0.0f; v = 0.0f; return; } // near north pole: clamp to origin
		u = p.x / denom;
		v = p.y / denom;
	}

	// Find boundary edges (edges that appear in exactly one triangle) and order them into a cycle.
	void boundary_cycle(const std::vector<std::array<size_t, 3>> &triangles,
	                   std::vector<size_t> &cycle_out) {
		std::map<std::pair<size_t, size_t>, int> edge_count;
		auto add_edge = [&edge_count](size_t a, size_t b) {
			if (a > b) std::swap(a, b);
			edge_count[{a, b}]++;
		};
		for (const auto &t : triangles) {
			add_edge(t[0], t[1]);
			add_edge(t[1], t[2]);
			add_edge(t[2], t[0]);
		}
		// Collect boundary edges (count == 1)
		std::vector<std::pair<size_t, size_t>> boundary;
		for (const auto &kv : edge_count) {
			if (kv.second == 1)
				boundary.push_back(kv.first);
		}
		if (boundary.empty()) { cycle_out.clear(); return; }
		// Build adjacency: for each vertex, which vertex follows it on the boundary cycle
		std::unordered_map<size_t, size_t> next;
		for (const auto &e : boundary) {
			next[e.first] = e.second;
		}
		// Walk cycle from first edge
		cycle_out.clear();
		size_t start = boundary[0].first;
		size_t cur = start;
		do {
			cycle_out.push_back(cur);
			auto it = next.find(cur);
			if (it == next.end()) break;
			cur = it->second;
		} while (cur != start && cycle_out.size() <= boundary.size());
	}
} // namespace

VoronoiSphere VoronoiSphereGenerator::generate(const WorldSeed & /* world_seed */, size_t num_sites) const {
	std::cerr << "[VoronoiSphereGenerator] generate start (sites=" << num_sites << ")\n";
	VoronoiSphere out;
	FibonacciSphere::fill(num_sites, out.sites);
	const size_t n = out.sites.size();
	std::cerr << "[VoronoiSphereGenerator] Fibonacci sites: " << n << "\n";

	// 1) Stereographic projection to 2D
	std::vector<Vec2> projected(n);
	for (size_t i = 0; i < n; ++i) {
		float u, v;
		stereographic(out.sites[i], u, v);
		projected[i].x = u;
		projected[i].y = v;
	}

	// Normalise to a bounded box so Delaunay runs in a safe numeric range (avoids huge coords from stereographic).
	float min_u = projected[0].x, max_u = projected[0].x, min_v = projected[0].y, max_v = projected[0].y;
	for (size_t i = 1; i < n; ++i) {
		min_u = std::min(min_u, projected[i].x);
		max_u = std::max(max_u, projected[i].x);
		min_v = std::min(min_v, projected[i].y);
		max_v = std::max(max_v, projected[i].y);
	}
	const float range_u = max_u - min_u;
	const float range_v = max_v - min_v;
	const float range = (range_u > range_v ? range_u : range_v);
	const float scale_2d = (range > 1e-9f) ? (10.0f / range) : 1.0f;
	const float mid_u = (min_u + max_u) * 0.5f;
	const float mid_v = (min_v + max_v) * 0.5f;
	for (size_t i = 0; i < n; ++i) {
		projected[i].x = (projected[i].x - mid_u) * scale_2d;
		projected[i].y = (projected[i].y - mid_v) * scale_2d;
	}

	// 2) Delaunay on the plane
	std::vector<std::array<size_t, 3>> tri;
	Delaunay2D::triangulate(projected, tri);
	std::cerr << "[VoronoiSphereGenerator] Delaunay triangles (2D): " << tri.size() << "\n";

	// Fallback if Delaunay returned nothing: fan from site 0 (ensures we never return 0 triangles)
	if (tri.empty() && n >= 3) {
		for (size_t i = 1; i + 1 < n; ++i)
			tri.push_back({{0, i, i + 1}});
		std::cerr << "[VoronoiSphereGenerator] fallback fan triangles: " << tri.size() << "\n";
	}

	// 3) Boundary of the triangulation (convex hull in 2D). With stereographic from the north pole,
	//    large (u,v) = northern hemisphere; the hole in the 2D triangulation is the projection centre
	//    (north pole). So the boundary in 3D is a northern ring; we fill the hole with the north pole.
	std::vector<size_t> boundary;
	boundary_cycle(tri, boundary);
	std::cerr << "[VoronoiSphereGenerator] boundary cycle length: " << boundary.size() << "\n";

	// 4) North pole vertex and cap triangles (boundary is northern ring; connect to north pole, not south)
	const size_t north_pole_idx = n;
	out.sites.push_back(Vec3(0.0f, 0.0f, 1.0f));
	for (size_t i = 0; i < boundary.size(); ++i) {
		size_t a = boundary[i];
		size_t b = boundary[(i + 1) % boundary.size()];
		out.triangles.push_back({{a, b, north_pole_idx}});
	}
	// 5) Original Delaunay triangles (indices unchanged)
	for (const auto &t : tri)
		out.triangles.push_back(t);

	std::cerr << "[VoronoiSphereGenerator] done: sites=" << out.sites.size() << " triangles=" << out.triangles.size() << "\n";
	return out;
}

} // namespace growth
