#pragma once

#include "math/Vec3.hpp"
#include "VoronoiSphere.hpp"
#include <cstddef>
#include <vector>

namespace growth {

/// Half-edge mesh over the sphere: regions (Voronoi cells), triangles (Delaunay), sides (half-edges).
/// Built from VoronoiSphere; supports streaming and detailed map generation.
/// Side index s: use s_begin_r(s), s_end_r(s), s_inner_t(s), s_outer_t(s); s_next_s(s) and s_twin_s(s) for traversal.
struct SphereHalfEdgeMesh {
	/// Region centre positions (Voronoi sites). r_xyz[r].
	std::vector<Vec3> r_xyz;
	/// Triangle centre positions (circumcenters). t_xyz[t].
	std::vector<Vec3> t_xyz;

	/// Half-edge connectivity. num_sides = 3 * num_triangles (one half-edge per triangle edge).
	/// Invalid index is k_invalid (e.g. boundary has no outer_t or twin).
	static constexpr size_t k_invalid = static_cast<size_t>(-1);

	/// Region at start of half-edge s. s_begin_r(s).
	std::vector<size_t> s_begin_r;
	/// Region at end of half-edge s. s_end_r(s).
	std::vector<size_t> s_end_r;
	/// Triangle (Delaunay) to the left of s. s_inner_t(s).
	std::vector<size_t> s_inner_t;
	/// Triangle to the right of s, or k_invalid if boundary. s_outer_t(s).
	std::vector<size_t> s_outer_t;
	/// Next half-edge around the same triangle (CCW). s_next_s(s).
	std::vector<size_t> s_next_s;
	/// Twin half-edge (same edge, opposite direction). s_twin_s(s).
	std::vector<size_t> s_twin_s;

	size_t num_regions()   const { return r_xyz.size(); }
	size_t num_triangles() const { return t_xyz.size(); }
	size_t num_sides()     const { return s_begin_r.size(); }
};

/// Build a half-edge mesh from a Voronoi sphere. Fills mesh.r_xyz, mesh.t_xyz and all s_* arrays.
void build_sphere_half_edge_mesh(const VoronoiSphere &voronoi, SphereHalfEdgeMesh &mesh);

} // namespace growth
