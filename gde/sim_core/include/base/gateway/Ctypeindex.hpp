#pragma once

// Sole include of <typeindex> in sim_core.
#include <typeindex>

namespace growth {

struct Ctypeindex final {
	Ctypeindex() = delete;

	using TypeIndex = std::type_index;
};

using TypeIndex = Ctypeindex::TypeIndex;

} // namespace growth
