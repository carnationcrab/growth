#include "base/gateway/Carray.hpp"
#include "base/gateway/Cstddef.hpp"
#include "base/gateway/Cvector.hpp"
#pragma once


namespace growth {

/// Encapsulates the Bowyer–Watson incremental Delaunay triangulation algorithm.
/// Works on 2D point sets given as separate x,y arrays; outputs triangle indices.
class BowyerWatson {
public:
	/// Triangulate the point set (px[i], py[i]). Appends triangles (each three
	/// indices into the point set) to triangles_out. Requires n >= 3; no-op otherwise.
	static void triangulate(const Vector<double>& px,
	                        const Vector<double>& py,
	                        Vector<Array<size_t, 3>>& triangles_out);
};

} // namespace growth
