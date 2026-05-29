#include "world/RegionPriorityFlood.hpp"
#include "base/gateway/Cutility.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

namespace {

/// Min-heap by elevation (lowest first), tie-break by region index for determinism.
struct ElevationMinQueue {
	Vector<Pair<float, Size>> heap;

	void clear() { heap.clear(); }

	void push(float priority, Size region) {
		heap.emplace_back(priority, region);
		Size i = heap.size() - 1u;
		while (i > 0u) {
			const Size parent = (i - 1u) / 2u;
			if (less(heap[i], heap[parent]))
				break;
			Cutility::swap(heap[i], heap[parent]);
			i = parent;
		}
	}

	Size pop() {
		const Pair<float, Size> top = heap.front();
		const Pair<float, Size> last = heap.back();
		heap.pop_back();
		if (!heap.empty()) {
			heap[0] = last;
			sift_down(0);
		}
		return top.second;
	}

	bool empty() const { return heap.empty(); }

private:
	static bool less(const Pair<float, Size> &a, const Pair<float, Size> &b) {
		if (a.first != b.first)
			return a.first < b.first;
		return a.second < b.second;
	}

	void sift_down(Size i) {
		const Size n = heap.size();
		while (true) {
			const Size left  = 2u * i + 1u;
			const Size right = 2u * i + 2u;
			Size smallest    = i;
			if (left < n && less(heap[left], heap[smallest]))
				smallest = left;
			if (right < n && less(heap[right], heap[smallest]))
				smallest = right;
			if (smallest == i)
				return;
			Cutility::swap(heap[i], heap[smallest]);
			i = smallest;
		}
	}
};

constexpr float k_breach_eps   = 1e-5f;
constexpr float k_saddle_band  = 0.08f;

} // namespace

void priority_flood_region_elevation(const CsrGraph &region_neighbours, Vector<float> &region_elevation) {
	const Size num_regions = region_elevation.size();
	if (num_regions == 0 || region_neighbours.num_nodes() < num_regions)
		return;

	Vector<uint8_t> closed(num_regions, 0);
	ElevationMinQueue queue;

	for (Size r = 0; r < num_regions; ++r) {
		if (region_elevation[r] < 0.0f)
			queue.push(region_elevation[r], r);
	}

	while (!queue.empty()) {
		const Size c = queue.pop();
		if (closed[c])
			continue;
		closed[c] = 1;

		const float ec = region_elevation[c];
		const U32 *nb_b = region_neighbours.neighbours_begin(c);
		const U32 *nb_e = region_neighbours.neighbours_end(c);
		for (const U32 *it = nb_b; it != nb_e; ++it) {
			const Size n = static_cast<Size>(*it);
			if (n >= num_regions || closed[n])
				continue;

			float en = region_elevation[n];
			if (en < ec)
				region_elevation[n] = ec;
			else if ((en - ec) <= k_saddle_band)
				region_elevation[n] = ec + k_breach_eps;

			queue.push(region_elevation[n], n);
		}
	}
}

} // namespace growth
