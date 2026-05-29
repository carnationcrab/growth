#include "world/Delaunay2D.hpp"
#include "world/BowyerWatson.hpp"
#include "base/gateway/Carray.hpp"
#include "base/gateway/Cstddef.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

void Delaunay2D::triangulate(const Vector<Vec2>& points,
                             Vector<Array<size_t, 3>>& triangles_out) {
	triangles_out.clear();
	if (points.size() < 3) return;

	Vector<double> px(points.size()), py(points.size());
	for (size_t i = 0; i < points.size(); ++i) {
		px[i] = static_cast<double>(points[i].x);
		py[i] = static_cast<double>(points[i].y);
	}

	BowyerWatson::triangulate(px, py, triangles_out);
}

} // namespace growth
