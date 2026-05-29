#pragma once

#include "../math/Vec2.hpp"
#include "base/gateway/Carray.hpp"
#include "base/gateway/Cstddef.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

/// 2D Delaunay triangulation. Delegates to BowyerWatson; used after stereographic projection for sphere Delaunay.
class Delaunay2D {
public:
	/// Triangulate points; append triangles (each three indices into points) to triangles_out.
	static void triangulate(const Vector<Vec2>& points, Vector<Array<size_t, 3>>& triangles_out);
};

} // namespace growth
