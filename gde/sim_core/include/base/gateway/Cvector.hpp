#pragma once

// Sole include of <vector> in sim_core.
#include <vector>

namespace growth {

struct Cvector final {
	Cvector() = delete;

	template <typename T>
	using Vector = std::vector<T>;
};

template <typename T>
using Vector = Cvector::Vector<T>;

} // namespace growth
