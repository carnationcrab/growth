#include "util/JobSystem.hpp"
#include "base/gateway/Cchrono.hpp"

namespace {
	thread_local bool g_is_worker_thread = false;
}

namespace growth {
namespace jobs {

JobSystem::~JobSystem() {
	shutdown();
}

void JobSystem::startup(U32 worker_count) {
	if (running_.load())
		return;

	if (worker_count == 0)
		worker_count = 1;

	running_.store(true);
	workers_.reserve(worker_count);
	for (U32 i = 0; i < worker_count; ++i)
		workers_.emplace_back([this]() { worker_loop(); });
}

void JobSystem::shutdown() {
	if (!running_.load())
		return;

	running_.store(false);
	cv_.notify_all();
	for (Cthread::Thread &worker : workers_) {
		if (worker.joinable())
			worker.join();
	}
	workers_.clear();
	queue_.clear();
}

bool JobSystem::is_running() const noexcept {
	return running_.load();
}

JobHandle JobSystem::kick_job(const JobDecl &decl) {
	JobHandle out;
	if (decl.fn == nullptr)
		return out;

	Job job;
	job.fn      = decl.fn;
	job.param   = decl.param;
	job.counter = decl.counter;
	job.id      = next_id_.fetch_add(1);
	out.id      = job.id;

	if (job.counter != nullptr)
		job.counter->value.fetch_add(1);

	{
		Cmutex::LockGuard lock(mutex_);
		queue_.push_back(job);
	}
	cv_.notify_one();
	return out;
}

bool JobSystem::try_pop(Job &out) {
	Cmutex::LockGuard lock(mutex_);
	if (queue_.empty())
		return false;
	out = queue_.back();
	queue_.pop_back();
	return true;
}

void JobSystem::wait(JobCounter &counter) {
	while (counter.value.load() > 0) {
		Job job;
		if (try_pop(job)) {
			job.fn(job.param);
			if (job.counter != nullptr)
				job.counter->value.fetch_sub(1);
			continue;
		}

		if (g_is_worker_thread) {
			Cthread::yield();
		} else {
			Cmutex::UniqueLock lock(mutex_);
			CconditionVariable::wait_for(cv_, lock, Milliseconds(1));
		}
	}
}

void JobSystem::worker_loop() {
	g_is_worker_thread = true;
	while (running_.load()) {
		Job job;
		{
			Cmutex::UniqueLock lock(mutex_);
			CconditionVariable::wait(cv_, lock, [&]() { return !running_.load() || !queue_.empty(); });
			if (!running_.load())
				break;
			if (queue_.empty())
				continue;
			job = queue_.back();
			queue_.pop_back();
		}

		if (job.fn == nullptr)
			continue;
		job.fn(job.param);
		if (job.counter != nullptr)
			job.counter->value.fetch_sub(1);
	}
	g_is_worker_thread = false;
}

JobSystem &global_job_system() {
	static JobSystem s_job_system;
	return s_job_system;
}

} // namespace jobs
} // namespace growth
