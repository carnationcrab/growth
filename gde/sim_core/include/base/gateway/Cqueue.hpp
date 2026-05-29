#pragma once

// Sole include of <queue> in sim_core.
#include <queue>

namespace growth {

struct Cqueue final {
	Cqueue() = delete;

	template <typename T>
	using Queue = std::queue<T>;
};

template <typename T>
using Queue = Cqueue::Queue<T>;

} // namespace growth
