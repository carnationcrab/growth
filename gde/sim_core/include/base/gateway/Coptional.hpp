#pragma once

// Sole include of <optional> in sim_core.
#include <optional>

namespace growth {

struct Coptional final {
	Coptional() = delete;

	template <typename T>
	using Optional = std::optional<T>;
};

template <typename T>
using Optional = Coptional::Optional<T>;

inline constexpr std::nullopt_t nullopt = std::nullopt;

} // namespace growth
