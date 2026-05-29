#pragma once

// Sole include of <variant> in sim_core.
#include <variant>

namespace growth {

struct Cvariant final {
	Cvariant() = delete;

	template <typename... Ts>
	using Variant = std::variant<Ts...>;
};

template <typename... Ts>
using Variant = Cvariant::Variant<Ts...>;

} // namespace growth
