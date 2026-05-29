#pragma once

// Sole include of <cstddef> in sim_core. Prefer growth::Size in new code.
#include <cstddef>

namespace growth {

struct Cstddef final {
	Cstddef() = delete;

	using Size      = std::size_t;
	using Ptrdiff   = std::ptrdiff_t;
	using Nullptr   = std::nullptr_t;
};

} // namespace growth
