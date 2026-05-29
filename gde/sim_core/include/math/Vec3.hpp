#pragma once

#include "Vec2.hpp"
#include "base/gateway/Cmath.hpp"

namespace growth {

struct Vec3 {
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;

	Vec3() = default;
	Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
	explicit Vec3(Vec2 v, float z_ = 0.0f) : x(v.x), y(v.y), z(z_) {}

	Vec3 operator+(Vec3 o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
	Vec3 operator-(Vec3 o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
	Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
	Vec3 operator/(float s) const { return Vec3(x / s, y / s, z / s); }

	Vec3 &operator+=(Vec3 o) { x += o.x; y += o.y; z += o.z; return *this; }
	Vec3 &operator-=(Vec3 o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
	Vec3 &operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
	Vec3 &operator/=(float s) { x /= s; y /= s; z /= s; return *this; }

	bool operator==(Vec3 o) const { return x == o.x && y == o.y && z == o.z; }

	float dot(Vec3 o) const { return x * o.x + y * o.y + z * o.z; }
	float length_squared() const { return dot(*this); }
	float length() const { return Cmath::sqrt(length_squared()); }

	Vec3 cross(Vec3 o) const {
		return Vec3(
			y * o.z - z * o.y,
			z * o.x - x * o.z,
			x * o.y - y * o.x
		);
	}

	Vec3 normalised() const {
		float l = length();
		return (l > 0.0f) ? (*this / l) : Vec3(0.0f, 0.0f, 0.0f);
	}

	Vec2 xy() const { return Vec2(x, y); }
	Vec2 xz() const { return Vec2(x, z); }
};

inline Vec3 operator*(float s, Vec3 v) { return v * s; }

} // namespace growth
