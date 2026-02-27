#pragma once

#include "../math/Vec3.hpp"
#include <cstddef>
#include <vector>

namespace growth {

/// Evenly distributed points on the unit sphere via spherical phyllotaxis (golden-section spiral).
/// Uses the golden angle π(3−√5) so successive points avoid clustering; good for Voronoi seed points.
/// References:
/// - https://observablehq.com/@mbostock/spherical-fibonacci-lattice
/// - https://observablehq.com/@fil/spherical-phyllotaxis
/// - https://web.archive.org/web/20120421191837/https://www.cgafaq.info/wiki/Evenly_distributed_points_on_sphere
class FibonacciSphere {
public:
	/// Fill `out` with `n` points on the unit sphere (x,y,z), deterministic order. Clears `out` first.
	static void fill(size_t n, std::vector<Vec3> &out);
};

} // namespace growth
