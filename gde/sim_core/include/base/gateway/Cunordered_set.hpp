#pragma once

// Sole include of <unordered_set> in sim_core.
#include <unordered_set>

namespace growth {

struct Cunordered_set final {
	Cunordered_set() = delete;

	template <typename T>
	using UnorderedSet = std::unordered_set<T>;
};

template <typename T>
using UnorderedSet = Cunordered_set::UnorderedSet<T>;

} // namespace growth
