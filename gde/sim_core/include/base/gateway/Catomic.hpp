#pragma once

#include <atomic>

namespace growth {

/// Gateway to standard atomics. Do not include atomic headers elsewhere under gde/sim_core/.
struct Catomic final {
	Catomic() = delete;

	template <typename T>
	using Atomic = std::atomic<T>;
};

} // namespace growth
