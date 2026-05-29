#pragma once

// Sole include of <unordered_map> in sim_core.
#include <unordered_map>

namespace growth {

struct Cunordered_map final {
	Cunordered_map() = delete;

	template <typename K, typename V>
	using UnorderedMap = std::unordered_map<K, V>;
};

template <typename K, typename V>
using UnorderedMap = Cunordered_map::UnorderedMap<K, V>;

} // namespace growth
