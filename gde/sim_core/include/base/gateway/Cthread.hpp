#pragma once

#include <thread>

namespace growth {

/// Gateway to std::thread. Do not include thread headers elsewhere under gde/sim_core/.
struct Cthread final {
	Cthread() = delete;

	using Thread = std::thread;

	static unsigned hardware_concurrency() {
		const unsigned n = std::thread::hardware_concurrency();
		return n > 0 ? n : 4u;
	}

	static void yield() { std::this_thread::yield(); }
};

} // namespace growth
