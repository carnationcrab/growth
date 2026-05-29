#pragma once

// Sole include of <ios> in sim_core.
#include <ios>

namespace growth {

struct Cios final {
	Cios() = delete;

	static constexpr auto binary = std::ios::binary;
};

} // namespace growth
