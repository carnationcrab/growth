#pragma once
#include "Types.hpp"

#include "math/Vec3.hpp"
#include "base/gateway/Carray.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

/// Unit-sphere mesh topology: region vertices and triangle faces (icosphere).
/// Canonical shape for the post-refactor pipeline. The legacy `VoronoiSphere` struct in
/// `VoronoiSphere.hpp` carries extra (now redundant) `circumcenters` and `cells` fields and
/// is being phased out — do not alias the two together, or both headers redefine the name.
struct SphereTopology {
	Vector<Vec3> sites;
	Vector<Array<Size, 3>> triangles;
};

} // namespace growth
