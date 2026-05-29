#pragma once

#include "Cstddef.hpp"

// Sole include of <cstdlib> in sim_core.
#include <cstdlib>

namespace growth {

struct Ccstdlib final {
	Ccstdlib() = delete;

	static void *malloc(Size size) { return std::malloc(size); }
	static void free(void *ptr) { std::free(ptr); }
	static int atoi(const char *s) { return std::atoi(s); }
};

} // namespace growth
