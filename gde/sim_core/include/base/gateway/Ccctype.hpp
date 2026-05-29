#pragma once

// Sole include of <cctype> in sim_core.
#include <cctype>

namespace growth {

struct Ccctype final {
	Ccctype() = delete;

	static int tolower(int c) { return std::tolower(c); }
	static int toupper(int c) { return std::toupper(c); }
};

} // namespace growth
