#pragma once

// Sole include of <cassert> in sim_core.
#include <cassert>

namespace growth {

/// Gateway to <cassert>. Domain code may use the global assert macro after including this header.
struct Cassert final {
	Cassert() = delete;
};

} // namespace growth
