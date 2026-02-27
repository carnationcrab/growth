// Spherical phyllotaxis: golden-angle spiral (dlong = π(3−√5), equal dz) so points are evenly distributed.
#include "world/FibonacciSphere.hpp"
#include <cmath>

namespace growth {

namespace {
	// Golden angle in radians: π(3−√5) ≈ 2.39996323. Longitude step so that bands don't align.
	const double k_golden_angle = 3.141592653589793 * (3.0 - std::sqrt(5.0));
}

void FibonacciSphere::fill(size_t n, std::vector<Vec3> &out) {
	out.clear();
	if (n == 0) return;

	const double dz = 2.0 / static_cast<double>(n);
	double z = 1.0 - dz * 0.5;
	double longitude = 0.0;

	out.reserve(n);
	for (size_t i = 0; i < n; ++i) {
		double r = std::sqrt(1.0 - z * z);
		double x = std::cos(longitude) * r;
		double y = std::sin(longitude) * r;
		out.emplace_back(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
		z -= dz;
		longitude += k_golden_angle;
	}
}

} // namespace growth
