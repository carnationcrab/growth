#pragma once

#include "math/Vec3.hpp"
#include <array>
#include <cstddef>
#include <vector>

namespace growth {

/// Reusable sphere topology: Fibonacci sites on the unit sphere and Delaunay triangulation.
/// Used by PlanetGlobeGenerator as the first stage; can be reused for other globe/cell meshes.
/// Sites may include one extra vertex (south pole) used to cap the stereographic hole.
struct VoronoiSphere {
	/// Seed points on the unit sphere (Fibonacci) plus optional south-pole cap vertex at end.
	std::vector<Vec3> sites;
	/// Delaunay triangles: each is three indices into sites.
	std::vector<std::array<size_t, 3>> triangles;
};

} // namespace growth
