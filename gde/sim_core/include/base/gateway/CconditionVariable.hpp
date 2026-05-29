#pragma once

#include "Cchrono.hpp"
#include "Cmutex.hpp"
#include <condition_variable>

namespace growth {

/// Gateway to std::condition_variable. Keep std threading headers behind base wrappers.
struct CconditionVariable final {
	CconditionVariable() = delete;

	using ConditionVariable = std::condition_variable;

	template <typename Lock, typename Pred>
	static void wait(ConditionVariable &cv, Lock &lock, Pred pred) {
		cv.wait(lock, std::move(pred));
	}

	template <typename Lock, typename Rep, typename Period>
	static void wait_for(ConditionVariable &cv, Lock &lock, const Cchrono::Duration<Rep, Period> &dur) {
		cv.wait_for(lock, dur);
	}
};

} // namespace growth

