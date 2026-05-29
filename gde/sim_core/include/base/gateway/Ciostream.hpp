#pragma once

// Sole include of <iostream> in sim_core.
#include <iostream>

namespace growth {

struct Ciostream final {
	Ciostream() = delete;

	static std::ostream &cout() { return std::cout; }
	static std::ostream &cerr() { return std::cerr; }
};

} // namespace growth
