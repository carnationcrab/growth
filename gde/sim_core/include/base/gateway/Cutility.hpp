#pragma once

// Sole include of <utility> in sim_core.
#include <utility>

namespace growth {

struct Cutility final {
	Cutility() = delete;

	template <typename T>
	static T &&move(T &&t) noexcept {
		return std::move(std::forward<T>(t));
	}

	template <typename T>
	static T &&forward(T &&t) noexcept {
		return std::forward<T>(t);
	}

	template <typename T1, typename T2>
	using Pair = std::pair<T1, T2>;

	template <typename T1, typename T2>
	static Pair<T1, T2> make_pair(T1 &&a, T2 &&b) {
		return std::make_pair(std::forward<T1>(a), std::forward<T2>(b));
	}

	template <typename T1, typename T2>
	static void swap(T1 &a, T2 &b) noexcept {
		std::swap(a, b);
	}
};

template <typename T1, typename T2>
using Pair = Cutility::Pair<T1, T2>;

} // namespace growth
