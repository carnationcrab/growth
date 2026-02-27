#include "world/Delaunay2D.hpp"
#include "world/BowyerWatson.hpp"
#include <array>
#include <cstddef>
#include <vector>

namespace growth {

void Delaunay2D::triangulate(const std::vector<Vec2>& points,
                             std::vector<std::array<size_t, 3>>& triangles_out) {
	triangles_out.clear();
	if (points.size() < 3) return;

	std::vector<double> px(points.size()), py(points.size());
	for (size_t i = 0; i < points.size(); ++i) {
		px[i] = static_cast<double>(points[i].x);
		py[i] = static_cast<double>(points[i].y);
	}

	BowyerWatson::triangulate(px, py, triangles_out);
}

} // namespace growth
