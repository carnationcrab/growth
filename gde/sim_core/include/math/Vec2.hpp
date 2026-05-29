#include "base/gateway/Cmath.hpp"
#pragma once


namespace growth {

struct Vec2 {
	float x = 0.0f;
	float y = 0.0f;

	Vec2() = default;
	Vec2(float x_, float y_) : x(x_), y(y_) {}

	Vec2 operator+(Vec2 o) const { return Vec2(x + o.x, y + o.y); }
	Vec2 operator-(Vec2 o) const { return Vec2(x - o.x, y - o.y); }
	Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
	Vec2 operator/(float s) const { return Vec2(x / s, y / s); }

	Vec2 &operator+=(Vec2 o) { x += o.x; y += o.y; return *this; }
	Vec2 &operator-=(Vec2 o) { x -= o.x; y -= o.y; return *this; }
	Vec2 &operator*=(float s) { x *= s; y *= s; return *this; }
	Vec2 &operator/=(float s) { x /= s; y /= s; return *this; }

	bool operator==(Vec2 o) const { return x == o.x && y == o.y; }

	float dot(Vec2 o) const { return x * o.x + y * o.y; }
	float length_squared() const { return dot(*this); }
	float length() const { return Cmath::sqrt(length_squared()); }

	Vec2 normalised() const {
		float l = length();
		return (l > 0.0f) ? (*this / l) : Vec2(0.0f, 0.0f);
	}
};

inline Vec2 operator*(float s, Vec2 v) { return v * s; }

} // namespace growth
