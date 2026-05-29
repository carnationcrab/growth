#pragma once

#include "math/Vec3.hpp"
#include "SphereTopology.hpp"
#include "VoronoiSphere.hpp"
#include "base/gateway/Cstddef.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

/// Half-edge mesh over the sphere: regions (Voronoi cells), triangles (Delaunay), sides (half-edges).
/// Built from VoronoiSphere; supports streaming and detailed map generation.
/// Side index s: use s_begin_r(s), s_end_r(s), s_inner_t(s), s_outer_t(s); s_next_s(s) and s_twin_s(s) for traversal.
struct SphereHalfEdgeMesh {
	/// Region centre positions (Voronoi sites). r_xyz[r].
	Vector<Vec3> r_xyz;
	/// Triangle centre positions (circumcenters). t_xyz[t].
	Vector<Vec3> t_xyz;

	/// Half-edge connectivity. num_sides = 3 * num_triangles (one half-edge per triangle edge).
	/// Invalid index is k_invalid (e.g. boundary has no outer_t or twin).
	static constexpr size_t k_invalid = static_cast<size_t>(-1);

	/// Region at start of half-edge s. s_begin_r(s).
	Vector<size_t> s_begin_r;
	/// Region at end of half-edge s. s_end_r(s).
	Vector<size_t> s_end_r;
	/// Triangle (Delaunay) to the left of s. s_inner_t(s).
	Vector<size_t> s_inner_t;
	/// Triangle to the right of s, or k_invalid if boundary. s_outer_t(s).
	Vector<size_t> s_outer_t;
	/// Next half-edge around the same triangle (CCW). s_next_s(s).
	Vector<size_t> s_next_s;
	/// Twin half-edge (same edge, opposite direction). s_twin_s(s).
	Vector<size_t> s_twin_s;

	size_t num_regions()   const { return r_xyz.size(); }
	size_t num_triangles() const { return t_xyz.size(); }
	size_t num_sides()     const { return s_begin_r.size(); }

	/// Walk one step around the region at s_begin_r[s]. Returns the next outgoing half-edge
	/// from the same start region, or k_invalid if the twin or next link is unavailable
	/// (open boundary or malformed mesh). Standard half-edge identity:
	///   next_around_region(s) = s_next_s[ s_twin_s[s] ]
	/// crossing through the twin lands on a half-edge whose end_r is our origin region; one
	/// step further along that triangle returns to a half-edge starting at the same region.
	static size_t next_side_around_region(const SphereHalfEdgeMesh &mesh, size_t s) {
		if (s == k_invalid || s >= mesh.s_twin_s.size())
			return k_invalid;
		const size_t twin = mesh.s_twin_s[s];
		if (twin == k_invalid || twin >= mesh.s_next_s.size())
			return k_invalid;
		return mesh.s_next_s[twin];
	}
};

/// Build a half-edge mesh from a sphere topology. Fills mesh.r_xyz, mesh.t_xyz (unit-sphere
/// triangle centroids derived from sites + triangles) and all s_* arrays.
void build_sphere_half_edge_mesh(const SphereTopology &topology, SphereHalfEdgeMesh &mesh);

/// Legacy overload that accepts the VoronoiSphere bundle (uses voronoi.circumcenters for t_xyz
/// instead of derived centroids). Kept until all callers are migrated to SphereTopology.
void build_sphere_half_edge_mesh(const VoronoiSphere &voronoi, SphereHalfEdgeMesh &mesh);

} // namespace growth
