#pragma once

#include "Cstddef.hpp"

// Sole include of <cstring> in sim_core.
#include <cstring>

namespace growth {

struct Ccstring final {
	Ccstring() = delete;

	static void *memcpy(void *dest, const void *src, Size count) {
		return std::memcpy(dest, src, count);
	}

	static Size strlen(const char *s) { return std::strlen(s); }
};

} // namespace growth
