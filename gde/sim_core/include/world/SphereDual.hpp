#pragma once
#include "Types.hpp"

#include "SphereHalfEdgeMesh.hpp"
#include "SphereTopology.hpp"
#include "util/CsrGraph.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

struct PlanetGlobe;

/// Dual-mesh helpers: triangle centroids and per-region triangle rings.
class SphereDual {
public:
	static Vec3 triangle_centroid(const SphereTopology &topology, const Array<Size, 3> &triangle);

	/// Fills mesh.t_xyz with unit-sphere triangle centroids.
	static void build_triangle_centroids(const SphereTopology &topology, SphereHalfEdgeMesh &mesh);

	/// region_rings_out[r] = triangle indices incident on region r (CCW order when manifold)
	/// stored as a CSR slice. O(sides), two-pass parallel.
	static void build_region_triangle_rings(const SphereHalfEdgeMesh &mesh, CsrGraph &region_rings_out);

	/// Legacy vec-of-vec accessor for preview/export callers. Internally converts CSR -> jagged.
	static void region_triangle_rings_for_globe(const PlanetGlobe &globe, Vector<Vector<Size>> &out);
};

} // namespace growth
