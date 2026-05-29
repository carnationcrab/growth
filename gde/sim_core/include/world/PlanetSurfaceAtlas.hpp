#pragma once

#include "Types.hpp"
#include "math/Vec3.hpp"
#include "util/CsrGraph.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

/// Compact overworld session data: fixed icosphere topology + per-region scalars.
/// Built once from PlanetGlobe; gameplay queries use this instead of half-edge mesh.
struct PlanetSurfaceAtlas {
	Vector<Vec3> sites;
	Vector<U32> topology_triangles;
	CsrGraph region_neighbours;
	Vector<I32> region_to_plate;
	Vector<float> region_elevation;
	Vector<float> region_moisture;
	U32 region_count = 0;
};

} // namespace growth
