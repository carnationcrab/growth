#pragma once

#include "Types.hpp"
#include "base/gateway/Catomic.hpp"
#include "base/gateway/CconditionVariable.hpp"
#include "base/gateway/Cmutex.hpp"
#include "base/gateway/Cthread.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {
namespace jobs {

using JobEntryPoint = void (*)(Size);

struct JobCounter {
	Catomic::Atomic<int> value{ 0 };
};

struct JobDecl {
	JobEntryPoint fn = nullptr;
	Size param  = 0;
	JobCounter *counter = nullptr;
};

struct JobHandle {
	U64 id = 0;
};

class JobSystem {
public:
	JobSystem() = default;
	~JobSystem();

	JobSystem(const JobSystem &)            = delete;
	JobSystem &operator=(const JobSystem &) = delete;

	void startup(U32 worker_count);
	void shutdown();
	bool is_running() const noexcept;

	JobHandle kick_job(const JobDecl &decl);
	void wait(JobCounter &counter);

private:
	struct Job {
		JobEntryPoint fn = nullptr;
		Size param  = 0;
		JobCounter *counter = nullptr;
		U64 id = 0;
	};

	bool try_pop(Job &out);
	void worker_loop();

	Catomic::Atomic<bool> running_{ false };
	Catomic::Atomic<U64> next_id_{ 1 };
	Cmutex::Mutex mutex_;
	CconditionVariable::ConditionVariable cv_;
	Vector<Job> queue_;
	Vector<Cthread::Thread> workers_;
};

/// Process-wide job system used by worldgen stage internals.
JobSystem &global_job_system();

} // namespace jobs
} // namespace growth
