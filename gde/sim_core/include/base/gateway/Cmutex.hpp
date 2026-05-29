#pragma once

#include <mutex>

namespace growth {

/// Gateway to std::mutex. Do not include mutex headers elsewhere under gde/sim_core/.
struct Cmutex final {
	Cmutex() = delete;

	using Mutex      = std::mutex;
	using LockGuard  = std::lock_guard<Mutex>;
	using UniqueLock = std::unique_lock<Mutex>;
};

} // namespace growth
