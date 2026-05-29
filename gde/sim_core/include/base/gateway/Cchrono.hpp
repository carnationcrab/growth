#pragma once

// Sole include of <chrono> in sim_core.
#include <chrono>

namespace growth {

struct Cchrono final {
	Cchrono() = delete;

	template <typename Rep, typename Period>
	using Duration = std::chrono::duration<Rep, Period>;

	using Milliseconds = std::chrono::milliseconds;
	using SteadyClock  = std::chrono::steady_clock;
	using TimePoint    = SteadyClock::time_point;

	static TimePoint now() { return SteadyClock::now(); }

	template <typename Rep, typename Period>
	static double duration_millis(const Duration<Rep, Period> &d) {
		return std::chrono::duration<double, std::milli>(d).count();
	}

	static double elapsed_millis(TimePoint t0, TimePoint t1) {
		return duration_millis(t1 - t0);
	}
};

using Milliseconds = Cchrono::Milliseconds;
using TimePoint    = Cchrono::TimePoint;

} // namespace growth
