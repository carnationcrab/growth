#pragma once

#include "math/Vec3.hpp"
#include "base/gateway/Carray.hpp"
#include "base/gateway/Cstddef.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

/// Reusable sphere topology: Fibonacci sites on the unit sphere and Delaunay triangulation.
/// Used by PlanetGlobeGenerator as the first stage; can be reused for other globe/cell meshes.
/// Sites are Fibonacci points plus optionally one cap vertex (centroid of boundary) used to fill the hole.
struct VoronoiSphere {
	/// Seed points on the unit sphere (Fibonacci), plus optional cap vertex at end when hole is filled.
	Vector<Vec3> sites;
	/// Delaunay triangles: each is three indices into sites.
	Vector<Array<size_t, 3>> triangles;
	/// Voronoi vertices: one circumcenter (on unit sphere) per triangle; index i = circumcenter of triangles[i].
	Vector<Vec3> circumcenters;
	/// Per-site Voronoi cells: cells[site_idx] = ordered list of circumcenter indices (into circumcenters) forming the cell polygon.
	Vector<Vector<size_t>> cells;
};

} // namespace growth
