#pragma once

// Sole include of <iomanip> in sim_core.
#include <iomanip>

namespace growth {

struct Ciomanip final {
	Ciomanip() = delete;

	static auto setprecision(int n) { return std::setprecision(n); }
	static auto fixed() { return std::fixed; }
};

} // namespace growth
