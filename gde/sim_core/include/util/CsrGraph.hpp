#pragma once

#include "Types.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

/// Compressed-sparse-row adjacency. Replaces vector<vector<Size>> for graph neighbours
/// across the world-gen pipeline. Two flat arrays: contiguous in memory, deterministic
/// traversal order, no per-node allocation. neighbours_of(r) returns the slice
/// [neighbours.begin() + offsets[r], neighbours.begin() + offsets[r+1]).
struct CsrGraph {
	Vector<U32> offsets;
	Vector<U32> neighbours;

	Size num_nodes() const {
		return offsets.empty() ? 0 : offsets.size() - 1;
	}

	Size num_edges() const { return neighbours.size(); }

	U32 degree(Size node) const {
		return offsets[node + 1] - offsets[node];
	}

	const U32 *neighbours_begin(Size node) const {
		return neighbours.data() + offsets[node];
	}

	const U32 *neighbours_end(Size node) const {
		return neighbours.data() + offsets[node + 1];
	}

	/// Iterator-pair adaptor for range-based for loops over a node's neighbours.
	struct NeighbourRange {
		const U32 *first;
		const U32 *last;
		const U32 *begin() const { return first; }
		const U32 *end() const { return last; }
		Size size() const { return static_cast<Size>(last - first); }
		bool empty() const { return first == last; }
	};

	NeighbourRange neighbours_of(Size node) const {
		return NeighbourRange{ neighbours_begin(node), neighbours_end(node) };
	}

	void clear() {
		offsets.clear();
		neighbours.clear();
	}

	bool empty() const { return num_nodes() == 0; }
};

/// Builder helpers: degrees -> offsets via exclusive prefix sum (sequential; O(N), cheap).
namespace csr_build {

inline void scan_offsets_exclusive(const Vector<U32> &degrees, Vector<U32> &offsets_out) {
	const Size n = degrees.size();
	offsets_out.assign(n + 1, 0u);
	U32 acc = 0;
	for (Size i = 0; i < n; ++i) {
		offsets_out[i] = acc;
		acc += degrees[i];
	}
	offsets_out[n] = acc;
}

} // namespace csr_build

/// Optional adapter: convert CSR back to jagged vector-of-vector. Used only by debug/preview
/// export paths that still consume the legacy shape. Not used in any hot path.
inline void csr_to_vec_of_vec(const CsrGraph &csr, Vector<Vector<Size>> &out) {
	const Size n = csr.num_nodes();
	out.assign(n, {});
	for (Size r = 0; r < n; ++r) {
		const U32 *b = csr.neighbours_begin(r);
		const U32 *e = csr.neighbours_end(r);
		out[r].reserve(static_cast<Size>(e - b));
		for (const U32 *it = b; it != e; ++it)
			out[r].push_back(static_cast<Size>(*it));
	}
}

} // namespace growth
