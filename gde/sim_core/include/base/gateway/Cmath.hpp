#pragma once

// Sole include of <cmath> in sim_core. All other code uses growth::Cmath.
#include <cmath>

namespace growth {

struct Cmath final {
	Cmath() = delete;

	static double sqrt(double x) { return std::sqrt(x); }
	static double abs(double x) { return std::abs(x); }
	static double sin(double x) { return std::sin(x); }
	static double cos(double x) { return std::cos(x); }
	static double exp(double x) { return std::exp(x); }
	static double log(double x) { return std::log(x); }
	static double pow(double x, double y) { return std::pow(x, y); }
	static double floor(double x) { return std::floor(x); }
	static float sqrt(float x) { return std::sqrt(x); }
	static float abs(float x) { return std::fabs(x); }
};

} // namespace growth
