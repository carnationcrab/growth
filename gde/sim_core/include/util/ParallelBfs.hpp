#pragma once

#include "Types.hpp"
#include "base/gateway/Catomic.hpp"
#include "util/CsrGraph.hpp"
#include "util/Parallel.hpp"
#include "base/gateway/Calgorithm.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {
namespace parallel {

/// Sentinel marking unvisited nodes in BFS distance arrays. U32::max - readable when printed.
constexpr U32 k_bfs_unvisited = static_cast<U32>(-1);

/// Level-synchronous, deterministic parallel BFS on a CsrGraph.
///
/// Invariant: distance_out[r] is INDEPENDENT of worker count and scheduling for a given
/// (graph, sources, stop_predicate) input. Reaches this by:
/// 1. Atomic CAS on the distance array discovers each node exactly once (writes are idempotent
///    because the only competing value at any level is `level` itself).
/// 2. Per-worker buckets collect newly-discovered children, then are concatenated and sorted
///    by node id so the next-level frontier order is canonical.
/// 3. Inner work uses static-stride partitioning over the frontier.
///
/// StopPredicate signature: `bool stop(U32 node)` — when true, BFS will NOT propagate through
/// the node (treated as a wall; its distance stays unvisited). Use `auto stop = [](U32){ return false; }`
/// for unbounded BFS.
template<typename StopPredicate>
void bfs_distance(
	const CsrGraph &adjacency,
	const Vector<U32> &sources_sorted,
	StopPredicate &&stop,
	Vector<U32> &distance_out
) {
	const Size num_nodes = adjacency.num_nodes();
	distance_out.assign(num_nodes, k_bfs_unvisited);

	if (num_nodes == 0 || sources_sorted.empty())
		return;

	// Atomic mirror of distance — needed for race-free first-touch detection. Final values
	// are copied back to `distance_out` at the end (or we can read atomics directly since they
	// are layout-compatible; copy keeps API simple for callers).
	Vector<Catomic::Atomic<U32>> dist_atomic(num_nodes);
	for (Size i = 0; i < num_nodes; ++i)
		dist_atomic[i].store(k_bfs_unvisited);

	// Sources receive distance 0 regardless of the stop predicate. The predicate only blocks
	// propagation INTO non-source stopped nodes (matching the original behaviour of the
	// sequential queue: a stopped source can still seed BFS, but a stopped non-source node
	// will never be enqueued).
	Vector<U32> frontier;
	frontier.reserve(sources_sorted.size());
	for (U32 s : sources_sorted) {
		if (s >= num_nodes)
			continue;
		U32 expected = k_bfs_unvisited;
		if (dist_atomic[s].compare_exchange_strong(expected, 0u))
			frontier.push_back(s);
	}

	const U32 workers = effective_worker_count();
	Vector<Vector<U32>> buckets(workers);

	U32 level = 1;
	while (!frontier.empty()) {
		for (auto &b : buckets)
			b.clear();

		for_workers(0, frontier.size(), [&](Size idx, U32 worker_idx) {
			const U32 parent = frontier[idx];
			const U32 *nb_begin = adjacency.neighbours_begin(parent);
			const U32 *nb_end = adjacency.neighbours_end(parent);
			for (const U32 *p = nb_begin; p != nb_end; ++p) {
				const U32 nb = *p;
				if (nb >= num_nodes)
					continue;
				if (stop(nb))
					continue;
				U32 expected = k_bfs_unvisited;
				if (dist_atomic[nb].compare_exchange_strong(expected, level))
					buckets[worker_idx].push_back(nb);
			}
		});

		Size total = 0;
		for (const auto &b : buckets)
			total += b.size();
		Vector<U32> next;
		next.reserve(total);
		for (auto &b : buckets)
			next.insert(next.end(), b.begin(), b.end());
		// Canonicalise next-level order so result is independent of worker count.
		Calgorithm::sort(next.begin(), next.end());
		frontier.swap(next);
		++level;
	}

	for (Size i = 0; i < num_nodes; ++i)
		distance_out[i] = dist_atomic[i].load();
}

/// Convenience overload: no stop predicate (full graph traversal).
inline void bfs_distance(
	const CsrGraph &adjacency,
	const Vector<U32> &sources_sorted,
	Vector<U32> &distance_out
) {
	bfs_distance(adjacency, sources_sorted, [](U32) { return false; }, distance_out);
}

/// Sentinel for label_out entries that did not receive a source label.
constexpr U32 k_bfs_unlabeled = static_cast<U32>(-1);

/// Multi-source labelling BFS with deterministic min-tie-break.
///
/// Each visited node receives the label of its nearest source (by graph distance). When two
/// sources are equidistant, the one with the SMALLER label id wins. This is enforced by
/// packing (depth_in_high_32, label_in_low_32) into a U64 and atomically taking the minimum;
/// the operation is commutative and associative so the final value is independent of worker
/// count and scheduling.
///
/// Result `label_out[v]` is bit-identical across runs and worker counts for a given graph +
/// sources_sorted + labels_in input.
template<typename StopPredicate>
void bfs_label(
	const CsrGraph &adjacency,
	const Vector<U32> &sources_sorted,
	const Vector<U32> &labels_in,
	StopPredicate &&stop,
	Vector<U32> &label_out
) {
	const Size num_nodes = adjacency.num_nodes();
	label_out.assign(num_nodes, k_bfs_unlabeled);
	if (num_nodes == 0 || sources_sorted.empty() || labels_in.size() != sources_sorted.size())
		return;

	constexpr U64 k_max = static_cast<U64>(-1);
	Vector<Catomic::Atomic<U64>> packed(num_nodes);
	for (Size i = 0; i < num_nodes; ++i)
		packed[i].store(k_max);

	Vector<U32> frontier;
	frontier.reserve(sources_sorted.size());
	for (Size i = 0; i < sources_sorted.size(); ++i) {
		const U32 s = sources_sorted[i];
		if (s >= num_nodes || stop(s))
			continue;
		const U64 prop = static_cast<U64>(labels_in[i]); // depth 0 in high bits = 0
		U64 cur = packed[s].load();
		while (prop < cur && !packed[s].compare_exchange_weak(cur, prop)) { }
		frontier.push_back(s);
	}
	Calgorithm::sort(frontier.begin(), frontier.end());
	frontier.erase(Calgorithm::unique(frontier.begin(), frontier.end()), frontier.end());

	const U32 workers = effective_worker_count();
	Vector<Vector<U32>> buckets(workers);

	U32 depth = 1;
	while (!frontier.empty()) {
		for (auto &b : buckets)
			b.clear();

		for_workers(0, frontier.size(), [&](Size idx, U32 worker_idx) {
			const U32 parent = frontier[idx];
			const U32 parent_label = static_cast<U32>(packed[parent].load() & 0xffffffffu);
			const U64 prop = (static_cast<U64>(depth) << 32) | static_cast<U64>(parent_label);
			const U32 *nb_begin = adjacency.neighbours_begin(parent);
			const U32 *nb_end = adjacency.neighbours_end(parent);
			for (const U32 *p = nb_begin; p != nb_end; ++p) {
				const U32 nb = *p;
				if (nb >= num_nodes || stop(nb))
					continue;
				U64 cur = packed[nb].load();
				while (prop < cur && !packed[nb].compare_exchange_weak(cur, prop)) { }
				buckets[worker_idx].push_back(nb);
			}
		});

		Size total = 0;
		for (const auto &b : buckets)
			total += b.size();
		Vector<U32> raw;
		raw.reserve(total);
		for (auto &b : buckets)
			raw.insert(raw.end(), b.begin(), b.end());
		Calgorithm::sort(raw.begin(), raw.end());
		raw.erase(Calgorithm::unique(raw.begin(), raw.end()), raw.end());

		// Keep only nodes that were first reached at exactly this depth.
		Vector<U32> next;
		next.reserve(raw.size());
		const U64 depth_hi = static_cast<U64>(depth) << 32;
		for (U32 v : raw) {
			const U64 val = packed[v].load();
			if ((val & 0xffffffff00000000ull) == depth_hi)
				next.push_back(v);
		}
		frontier.swap(next);
		++depth;
	}

	for (Size i = 0; i < num_nodes; ++i) {
		const U64 v = packed[i].load();
		label_out[i] = (v == k_max) ? k_bfs_unlabeled : static_cast<U32>(v & 0xffffffffu);
	}
}

inline void bfs_label(
	const CsrGraph &adjacency,
	const Vector<U32> &sources_sorted,
	const Vector<U32> &labels_in,
	Vector<U32> &label_out
) {
	bfs_label(adjacency, sources_sorted, labels_in, [](U32) { return false; }, label_out);
}

} // namespace parallel
} // namespace growth
