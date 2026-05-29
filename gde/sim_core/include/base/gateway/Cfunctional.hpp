#pragma once

// Sole include of <functional> in sim_core.
#include <functional>

namespace growth {

struct Cfunctional final {
	Cfunctional() = delete;

	template <typename Sig>
	using Function = std::function<Sig>;
};

template <typename Sig>
using Function = Cfunctional::Function<Sig>;

} // namespace growth
