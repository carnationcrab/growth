#pragma once

#include "base/gateway/Cmath.hpp"


namespace growth {

namespace constants {
	constexpr float pi = 3.14159265358979323846f;
	constexpr float two_pi = 2.0f * pi;
	constexpr float half_pi = 0.5f * pi;
}

inline float clamp(float v, float lo, float hi) {
	return (v < lo) ? lo : (v > hi) ? hi : v;
}

inline float lerp(float a, float b, float t) {
	return a + (b - a) * t;
}

inline float min(float a, float b) { return (a < b) ? a : b; }
inline float max(float a, float b) { return (a > b) ? a : b; }

inline float saturate(float v) { return clamp(v, 0.0f, 1.0f); }

inline float smoothstep(float edge0, float edge1, float x) {
	float t = saturate((x - edge0) / (edge1 - edge0));
	return t * t * (3.0f - 2.0f * t);
}

} // namespace growth
