#pragma once

#include "math/Vec3.hpp"

namespace growth {

/// Circumcenter of triangle (a,b,c) in R³, projected onto the unit sphere.
/// Formula (Shewchuk): centre of circumcircle in plane of triangle, then normalised.
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
	return true;
}

} // namespace growth
