#include "world/BowyerWatson.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <utility>
#include <vector>

namespace growth {

namespace {

using Edge = std::pair<size_t, size_t>;

Edge ordered_edge(size_t a, size_t b) {
	return a < b ? Edge{a, b} : Edge{b, a};
}

void circumcircle(double ax, double ay, double bx, double by, double cx, double cy,
                  double& out_cx, double& out_cy, double& out_rsq) {
	const double d = 2.0 * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));
	if (std::fabs(d) < 1e-20) {
		out_rsq = -1.0;
		return;
	}
	out_cx = ((ax * ax + ay * ay) * (by - cy) + (bx * bx + by * by) * (cy - ay) +
	          (cx * cx + cy * cy) * (ay - by)) /
	         d;
	out_cy = ((ax * ax + ay * ay) * (cx - bx) + (bx * bx + by * by) * (ax - cx) +
	          (cx * cx + cy * cy) * (bx - ax)) /
	         d;
	const double dx = ax - out_cx, dy = ay - out_cy;
	out_rsq = dx * dx + dy * dy;
}

bool in_circle(double ax, double ay, double bx, double by, double cx, double cy,
               double px, double py) {
	double ccx, ccy, rsq;
	circumcircle(ax, ay, bx, by, cx, cy, ccx, ccy, rsq);
	if (rsq <= 0.0) return false;
	const double dpx = px - ccx, dpy = py - ccy;
	return dpx * dpx + dpy * dpy < rsq;
}

} // namespace

void BowyerWatson::triangulate(const std::vector<double>& px,
                               const std::vector<double>& py,
                               std::vector<std::array<size_t, 3>>& triangles_out) {
	triangles_out.clear();
	const size_t n = px.size();
	if (n != py.size() || n < 3) return;

	double min_x = px[0], max_x = px[0], min_y = py[0], max_y = py[0];
	for (size_t i = 1; i < n; ++i) {
		min_x = std::min(min_x, px[i]);
		max_x = std::max(max_x, px[i]);
		min_y = std::min(min_y, py[i]);
		max_y = std::max(max_y, py[i]);
	}
	const double dx = max_x - min_x, dy = max_y - min_y;
	const double margin = std::max(1.0, 0.2 * std::max(dx, dy));
	const double L = std::max(dx, dy) + 2.0 * margin;

	const size_t si0 = n, si1 = n + 1, si2 = n + 2;
	const double sx0 = min_x - margin;
	const double sy0 = min_y - margin;
	const double sx1 = min_x - margin + 2.0 * L;
	const double sy1 = min_y - margin;
	const double sx2 = min_x - margin + L;
	const double sy2 = min_y - margin + 2.0 * L;

	std::vector<std::array<size_t, 3>> tri;
	tri.push_back({{si0, si1, si2}});

	auto get_x = [&](size_t i) -> double {
		if (i == si0) return sx0;
		if (i == si1) return sx1;
		if (i == si2) return sx2;
		return px[i];
	};
	auto get_y = [&](size_t i) -> double {
		if (i == si0) return sy0;
		if (i == si1) return sy1;
		if (i == si2) return sy2;
		return py[i];
	};

	for (size_t p = 0; p < n; ++p) {
		const double pxp = px[p], pyp = py[p];
		std::map<Edge, int> edge_count;
		std::vector<std::array<size_t, 3>> new_tri;

		for (const auto& t : tri) {
			size_t i = t[0], j = t[1], k = t[2];
			double ix = get_x(i), iy = get_y(i);
			double jx = get_x(j), jy = get_y(j);
			double kx = get_x(k), ky = get_y(k);
			if (in_circle(ix, iy, jx, jy, kx, ky, pxp, pyp)) {
				edge_count[ordered_edge(i, j)]++;
				edge_count[ordered_edge(j, k)]++;
				edge_count[ordered_edge(k, i)]++;
				continue;
			}
			new_tri.push_back(t);
		}

		for (const auto& kv : edge_count) {
			if (kv.second != 1) continue;
			const Edge& e = kv.first;
			new_tri.push_back({{e.first, e.second, p}});
		}
		tri = std::move(new_tri);
	}

	for (const auto& t : tri) {
		if (t[0] < n && t[1] < n && t[2] < n)
			triangles_out.push_back(t);
	}
}

} // namespace growth
