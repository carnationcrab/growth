#pragma once

#include "Types.hpp"
#include "base/gateway/Catomic.hpp"
#include "base/gateway/Cthread.hpp"
#include "util/JobSystem.hpp"
#include "base/gateway/Cutility.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {
namespace parallel {

enum class ExecutionProfile {
	MaxPerformance,
	MaxDeterminism,
};

struct RuntimeConfig {
	ExecutionProfile profile = ExecutionProfile::MaxPerformance;
	U32 worker_count_override = 0;
	bool enable_simd = true;
};

void configure(const RuntimeConfig &config);
RuntimeConfig current_config();

namespace detail {

template<typename Fn>
struct ParallelTask {
	Fn *fn = nullptr;
	Catomic::Atomic<Size> *next_index = nullptr;
	Size begin = 0;
	Size end = 0;
	Size grain = 1;
	U32 worker_index = 0;
	U32 worker_count = 1;
	bool deterministic = false;
};

template<typename Fn>
void run_parallel_task(Size param) {
	ParallelTask<Fn> *task = reinterpret_cast<ParallelTask<Fn> *>(param);
	if (task == nullptr || task->fn == nullptr)
		return;

	if (task->deterministic) {
		for (Size i = task->begin + static_cast<Size>(task->worker_index); i < task->end; i += static_cast<Size>(task->worker_count))
			(*task->fn)(i);
		return;
	}

	while (true) {
		const Size chunk_begin = task->next_index->fetch_add(task->grain);
		if (chunk_begin >= task->end)
			break;
		const Size chunk_end = ((chunk_begin + task->grain) < task->end) ? (chunk_begin + task->grain) : task->end;
		for (Size i = chunk_begin; i < chunk_end; ++i)
			(*task->fn)(i);
	}
}

} // namespace detail

/// Effective worker count under the current RuntimeConfig.
/// MaxDeterminism collapses to 1 worker so callers can size per-worker buckets correctly.
inline U32 effective_worker_count() {
	const RuntimeConfig cfg = current_config();
	if (cfg.profile == ExecutionProfile::MaxDeterminism)
		return 1u;
	U32 workers = cfg.worker_count_override > 0 ? cfg.worker_count_override : static_cast<U32>(Cthread::hardware_concurrency());
	return workers == 0 ? 1u : workers;
}

namespace detail {

template<typename Fn>
struct WorkerTask {
	Fn *fn = nullptr;
	Size begin = 0;
	Size end = 0;
	U32 worker_index = 0;
	U32 worker_count = 1;
};

template<typename Fn>
void run_worker_task(Size param) {
	WorkerTask<Fn> *task = reinterpret_cast<WorkerTask<Fn> *>(param);
	if (task == nullptr || task->fn == nullptr)
		return;
	for (Size i = task->begin + static_cast<Size>(task->worker_index); i < task->end; i += static_cast<Size>(task->worker_count))
		(*task->fn)(i, task->worker_index);
}

} // namespace detail

/// Worker-index-aware parallel loop with static stride partitioning. The callback receives
/// (item_index, worker_index). Partitioning is deterministic (worker w handles indices
/// {begin + w, begin + w + W, ...}), so result indices and per-worker buckets are reproducible
/// across runs and core counts (so long as the body is itself deterministic in writes).
template<typename Fn>
void for_workers(Size begin, Size end, Fn &&fn) {
	if (end <= begin)
		return;
	const U32 workers = effective_worker_count();
	if (workers <= 1) {
		for (Size i = begin; i < end; ++i)
			fn(i, 0u);
		return;
	}

	jobs::JobSystem &job_system = jobs::global_job_system();
	if (!job_system.is_running())
		job_system.startup(workers);

	Vector<detail::WorkerTask<Fn>> tasks(workers);
	jobs::JobCounter counter;
	for (U32 worker = 0; worker < workers; ++worker) {
		detail::WorkerTask<Fn> &task = tasks[worker];
		task.fn = &fn;
		task.begin = begin;
		task.end = end;
		task.worker_index = worker;
		task.worker_count = workers;

		jobs::JobDecl job;
		job.fn = &detail::run_worker_task<Fn>;
		job.param = reinterpret_cast<Size>(&task);
		job.counter = &counter;
		job_system.kick_job(job);
	}
	job_system.wait(counter);
}

/// Deterministic 8-bit-pass radix sort on U64 keys with parallel U64 payload.
/// Keys must already encode the desired ordering (caller appends tie-break bits if needed).
/// Implementation: 8 passes, 256 buckets each; serial counting + serial scatter using parallel
/// memcpy-style writes inside each pass. O(N * key_bytes), bit-identical output regardless of
/// worker count.
inline void sort_radix_u64_pairs(Vector<U64> &keys, Vector<U64> &values) {
	const Size n = keys.size();
	if (n != values.size() || n < 2)
		return;

	Vector<U64> keys_tmp(n);
	Vector<U64> vals_tmp(n);

	for (int pass = 0; pass < 8; ++pass) {
		const int shift = pass * 8;
		Size counts[257] = {};
		for (Size i = 0; i < n; ++i) {
			const U32 b = static_cast<U32>((keys[i] >> shift) & 0xffu);
			++counts[b + 1];
		}
		for (int b = 1; b < 257; ++b)
			counts[b] += counts[b - 1];

		for (Size i = 0; i < n; ++i) {
			const U32 b = static_cast<U32>((keys[i] >> shift) & 0xffu);
			const Size dst = counts[b]++;
			keys_tmp[dst] = keys[i];
			vals_tmp[dst] = values[i];
		}
		keys.swap(keys_tmp);
		values.swap(vals_tmp);
	}
}

/// Parallel index loop [begin, end). Single-threaded when count <= grain or hardware concurrency is 1.
template<typename Fn>
void for_range(Size begin, Size end, Size grain, Fn &&fn) {
	if (end <= begin)
		return;
	const Size count = end - begin;
	if (grain == 0)
		grain = 1;
	const RuntimeConfig cfg = current_config();
	U32 workers = cfg.worker_count_override > 0 ? cfg.worker_count_override : static_cast<U32>(Cthread::hardware_concurrency());
	if (workers == 0)
		workers = 1;
	if (workers <= 1 || count <= grain) {
		for (Size i = begin; i < end; ++i)
			fn(i);
		return;
	}

	jobs::JobSystem &job_system = jobs::global_job_system();
	if (!job_system.is_running())
		job_system.startup(workers);

	Catomic::Atomic<Size> next_index{ begin };
	Vector<detail::ParallelTask<Fn>> tasks;
	tasks.resize(workers);
	jobs::JobCounter counter;
	const bool deterministic = cfg.profile == ExecutionProfile::MaxDeterminism;

	for (U32 worker = 0; worker < workers; ++worker) {
		detail::ParallelTask<Fn> &task = tasks[worker];
		task.fn = &fn;
		task.next_index = &next_index;
		task.begin = begin;
		task.end = end;
		task.grain = grain;
		task.worker_index = worker;
		task.worker_count = workers;
		task.deterministic = deterministic;

		jobs::JobDecl job;
		job.fn = &detail::run_parallel_task<Fn>;
		job.param = reinterpret_cast<Size>(&task);
		job.counter = &counter;
		job_system.kick_job(job);
	}

	job_system.wait(counter);
}

} // namespace parallel
} // namespace growth
