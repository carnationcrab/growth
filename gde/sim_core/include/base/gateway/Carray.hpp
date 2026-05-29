#pragma once

#include "Cstddef.hpp"

// Sole include of <array> in sim_core.
#include <array>

namespace growth {

struct Carray final {
	Carray() = delete;

	template <typename T, std::size_t N>
	using Array = std::array<T, N>;
};

template <typename T, std::size_t N>
using Array = Carray::Array<T, N>;

} // namespace growth
