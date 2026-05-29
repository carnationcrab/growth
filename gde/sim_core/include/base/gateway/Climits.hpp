#pragma once

// Sole include of <limits> in sim_core.
#include <limits>

namespace growth {

struct Climits final {
	Climits() = delete;

	template <typename T>
	struct NumericLimits {
		static constexpr T infinity() { return std::numeric_limits<T>::infinity(); }
		static constexpr T lowest() { return std::numeric_limits<T>::lowest(); }
		static constexpr T max() { return std::numeric_limits<T>::max(); }
		static constexpr T min() { return std::numeric_limits<T>::min(); }
	};
};

} // namespace growth
