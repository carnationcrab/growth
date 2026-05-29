#include "world/RiverFlow.hpp"
#include "base/gateway/Calgorithm.hpp"
#include "base/gateway/Cutility.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

namespace {

constexpr size_t k_downflow_unset = SphereHalfEdgeMesh::k_invalid;

/// Min-heap by elevation (lowest first), tie-break by triangle index for determinism.
struct ElevationMinQueue {
	Vector<Pair<float, size_t>> heap;

	void clear() { heap.clear(); }

	void push(float priority, size_t triangle) {
		heap.emplace_back(priority, triangle);
		size_t i = heap.size() - 1u;
		while (i > 0u) {
			const size_t parent = (i - 1u) / 2u;
			if (less(heap[i], heap[parent]))
				break;
			Cutility::swap(heap[i], heap[parent]);
			i = parent;
		}
	}

	size_t pop() {
		const Pair<float, size_t> top = heap.front();
		const Pair<float, size_t> last = heap.back();
		heap.pop_back();
		if (!heap.empty()) {
			heap[0] = last;
			sift_down(0);
		}
		return top.second;
	}

	bool empty() const { return heap.empty(); }

private:
	static bool less(const Pair<float, size_t> &a, const Pair<float, size_t> &b) {
		if (a.first != b.first)
			return a.first < b.first;
		return a.second < b.second;
	}

	static bool greater(const Pair<float, size_t> &a, const Pair<float, size_t> &b) {
		if (a.first != b.first)
			return a.first > b.first;
		return a.second > b.second;
	}

	void sift_down(size_t i) {
		const size_t n = heap.size();
		while (true) {
			const size_t left  = 2u * i + 1u;
			const size_t right = 2u * i + 2u;
			size_t smallest    = i;
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

bool downflow_is_set(const Vector<size_t> &downflow, size_t t) {
	return t < downflow.size() && downflow[t] != k_downflow_unset;
}

size_t trunk_triangle_from_side(const SphereHalfEdgeMesh &mesh, size_t flow_s) {
	return mesh.s_outer_t[flow_s];
}

} // namespace

void assign_downflow(
	const SphereHalfEdgeMesh &mesh,
	const Vector<float> &triangle_elevation,
	RiverFlow &out) {
	const size_t num_tri   = mesh.num_triangles();
	const size_t num_sides = mesh.num_sides();
	out.t_downflow_s.assign(num_tri, k_downflow_unset);
	out.order_t.clear();
	out.order_t.reserve(num_tri);
	out.s_flow.assign(num_sides, 0.0f);

	ElevationMinQueue queue;

	// Ocean triangles (below sea level): drain to lowest neighbour, seed the priority queue.
	for (size_t t = 0; t < num_tri; ++t) {
		const float e_t = (t < triangle_elevation.size()) ? triangle_elevation[t] : 0.0f;
		if (e_t >= 0.0f)
			continue;

		size_t best_s   = k_downflow_unset;
		float best_elev = e_t;
		for (int edge = 0; edge < 3; ++edge) {
			const size_t s = t * 3u + static_cast<size_t>(edge);
			const size_t outer_t = mesh.s_outer_t[s];
			if (outer_t == SphereHalfEdgeMesh::k_invalid)
				continue;
			const float e_outer = (outer_t < triangle_elevation.size()) ? triangle_elevation[outer_t] : 0.0f;
			if (e_outer < best_elev) {
				best_elev = e_outer;
				best_s    = s;
			}
		}
		out.order_t.push_back(t);
		out.t_downflow_s[t] = best_s;
		queue.push(e_t, t);
	}

	// Land triangles: visit in increasing elevation; assign downflow from uphill neighbours.
	for (size_t queue_out = 0; queue_out < num_tri; ++queue_out) {
		if (queue.empty())
			break;
		const size_t current_t = queue.pop();
		for (int edge = 0; edge < 3; ++edge) {
			const size_t s           = current_t * 3u + static_cast<size_t>(edge);
			const size_t neighbor_t  = mesh.s_outer_t[s];
			if (neighbor_t == SphereHalfEdgeMesh::k_invalid || neighbor_t >= num_tri)
				continue;
			if (downflow_is_set(out.t_downflow_s, neighbor_t))
				continue;
			const float e_neighbor = (neighbor_t < triangle_elevation.size()) ? triangle_elevation[neighbor_t] : 0.0f;
			if (e_neighbor < 0.0f)
				continue;

			const size_t twin = mesh.s_twin_s[s];
			if (twin == SphereHalfEdgeMesh::k_invalid)
				continue;
			out.t_downflow_s[neighbor_t] = twin;
			out.order_t.push_back(neighbor_t);
			queue.push(e_neighbor, neighbor_t);
		}
	}
}

void assign_flow(
	const SphereHalfEdgeMesh &mesh,
	const RiverFlow &downflow,
	const Vector<float> &triangle_elevation,
	const Vector<float> &triangle_moisture,
	RiverFlow &out) {
	const size_t num_tri   = mesh.num_triangles();
	const size_t num_sides = mesh.num_sides();
	out.t_downflow_s = downflow.t_downflow_s;
	out.order_t      = downflow.order_t;
	out.s_flow.assign(num_sides, 0.0f);

	Vector<float> t_flow(num_tri, 0.0f);
	for (size_t t = 0; t < num_tri; ++t) {
		const float e_t = (t < triangle_elevation.size()) ? triangle_elevation[t] : 0.0f;
		if (e_t >= 0.0f) {
			const float m = (t < triangle_moisture.size()) ? triangle_moisture[t] : 0.0f;
			t_flow[t]     = 0.5f * m * m;
		}
	}

	const Vector<size_t> &order = downflow.order_t.empty() ? out.order_t : downflow.order_t;
	for (size_t i = order.size(); i > 0; --i) {
		const size_t tributary_t = order[i - 1];
		if (tributary_t >= num_tri)
			continue;
		const size_t flow_s = out.t_downflow_s[tributary_t];
		if (flow_s == k_downflow_unset)
			continue;
		const size_t trunk_t = trunk_triangle_from_side(mesh, flow_s);
		if (trunk_t == SphereHalfEdgeMesh::k_invalid || trunk_t >= num_tri)
			continue;
		t_flow[trunk_t] += t_flow[tributary_t];
		out.s_flow[flow_s] += t_flow[tributary_t];
	}
}

float river_flow_visual_threshold(const Vector<float> &s_flow) {
	Vector<float> positive;
	positive.reserve(s_flow.size() / 8u);
	for (float f : s_flow) {
		if (f > 0.0f)
			positive.push_back(f);
	}
	if (positive.empty())
		return 1e9f;
	const Size idx = Calgorithm::min(positive.size() * 90u / 100u, positive.size() - 1u);
	Calgorithm::nth_element(positive.begin(), positive.begin() + idx, positive.end());
	return positive[idx];
}

} // namespace growth
