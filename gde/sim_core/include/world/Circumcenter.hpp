#pragma once

#include "math/Vec3.hpp"

namespace growth {

/// Circumcenter of triangle (a,b,c) on the unit sphere: centre of circumcircle in plane of triangle,
/// projected onto the sphere. Chooses the point in the same hemisphere as the triangle (so it lies
/// "inside" the small triangle, not the antipodal point).
/// \return true if circumcenter computed; false if degenerate (collinear), in which case out is unchanged.
inline bool circumcenter_on_sphere(const Vec3 &a, const Vec3 &b, const Vec3 &c, Vec3 &out) {
	const Vec3 ac = c - a;
	const Vec3 ab = b - a;
	const Vec3 abXac = ab.cross(ac);
	const float denom = 2.0f * abXac.length_squared();
	if (denom < 1e-12f)
		return false;
	const Vec3 to_center = (abXac.cross(ab) * ac.length_squared() + ac.cross(abXac) * ab.length_squared()) / denom;
	out = (a + to_center).normalised();
	// Planar circumcenter normalised can land on the far side of the sphere for large triangles.
	// Keep the circumcenter in the same hemisphere as the triangle (use centroid as "inside" direction).
	const Vec3 centroid = (a + b + c).normalised();
	if (out.dot(centroid) < 0.0f)
		out = out * -1.0f;
	return true;
}

} // namespace growth
