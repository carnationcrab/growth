#pragma once
#include "Types.hpp"

#include "SphereHalfEdgeMesh.hpp"
#include "util/CsrGraph.hpp"

namespace growth {

/// Builds region adjacency once from a closed spherical half-edge mesh.
/// Output is CSR (offsets + flat neighbours), parallel two-pass build (count + scatter).
class RegionNeighbourIndex {
public:
	static void build(const SphereHalfEdgeMesh &mesh, CsrGraph &neighbours_out);
};

} // namespace growth
