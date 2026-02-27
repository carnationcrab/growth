#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace growth {

/// Encapsulates the Bowyer–Watson incremental Delaunay triangulation algorithm.
/// Works on 2D point sets given as separate x,y arrays; outputs triangle indices.
class BowyerWatson {
public:
	/// Triangulate the point set (px[i], py[i]). Appends triangles (each three
	/// indices into the point set) to triangles_out. Requires n >= 3; no-op otherwise.
	static void triangulate(const std::vector<double>& px,
	                        const std::vector<double>& py,
	                        std::vector<std::array<size_t, 3>>& triangles_out);
};

} // namespace growth
