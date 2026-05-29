#pragma once

// Sole include of <map> in sim_core.
#include <map>

namespace growth {

struct Cmap final {
	Cmap() = delete;

	template <typename K, typename V>
	using Map = std::map<K, V>;
};

template <typename K, typename V>
using Map = Cmap::Map<K, V>;

} // namespace growth
