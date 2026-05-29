#pragma once

// Sole include of <set> in sim_core.
#include <set>

namespace growth {

struct Cset final {
	Cset() = delete;

	template <typename T>
	using Set = std::set<T>;
};

template <typename T>
using Set = Cset::Set<T>;

} // namespace growth
