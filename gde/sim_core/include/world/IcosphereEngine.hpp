#pragma once
#include "Types.hpp"

#include "SphereTopology.hpp"

namespace growth {

/// Builds a geodesic icosphere: uniform spherical triangles, O(triangle count).
class IcosphereEngine {
public:
	/// Subdivision level 0 = icosahedron (12 vertices, 20 triangles).
	static Size vertex_count_for_subdivision(int subdivision);
	static Size triangle_count_for_subdivision(int subdivision);

	/// Smallest subdivision whose vertex count is >= target_vertices (capped at 9).
	static int subdivision_for_target(Size target_vertices);

	/// True when a fine icosphere refines a coarse one (first coarse verts are shared indices).
	static bool fine_embeds_coarse(Size fine_vertices, Size coarse_vertices);

	/// Fill out.sites (unit sphere) and out.triangles for the given subdivision level.
	static void build(int subdivision, SphereTopology &out);
};

} // namespace growth
