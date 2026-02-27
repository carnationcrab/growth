#pragma once

#include "../math/Vec2.hpp"
#include <array>
#include <cstddef>
#include <vector>

namespace growth {

/// 2D Delaunay triangulation. Delegates to BowyerWatson; used after stereographic projection for sphere Delaunay.
class Delaunay2D {
public:
	/// Triangulate points; append triangles (each three indices into points) to triangles_out.
	static void triangulate(const std::vector<Vec2>& points, std::vector<std::array<size_t, 3>>& triangles_out);
};

} // namespace growth
