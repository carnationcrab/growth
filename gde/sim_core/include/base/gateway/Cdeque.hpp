#pragma once

// Sole include of <deque> in sim_core.
#include <deque>

namespace growth {

struct Cdeque final {
	Cdeque() = delete;

	template <typename T>
	using Deque = std::deque<T>;
};

template <typename T>
using Deque = Cdeque::Deque<T>;

} // namespace growth
