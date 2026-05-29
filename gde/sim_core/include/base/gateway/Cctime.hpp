#pragma once

// Sole include of <ctime> in sim_core.
#include <ctime>

namespace growth {

struct Cctime final {
	Cctime() = delete;

	static std::time_t time(std::time_t *t) { return std::time(t); }
};

} // namespace growth
