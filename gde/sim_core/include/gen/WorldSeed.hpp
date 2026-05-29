#include "base/gateway/Cstdint.hpp"
#pragma once


namespace growth {

/// Combined seed and world-generation parameters from user input (e.g. form answers).
struct WorldSeed {
	/// Deterministic seed value for terrain/noise.
	uint64_t value = 0;
	/// Height scale for terrain (e.g. vertical scale of noise).
	float height_scale = 1.0f;
	/// Number of octaves for fractal noise.
	int octaves = 5;
	/// Base frequency for noise (e.g. world-space scale).
	float frequency = 0.025f;
};

} // namespace growth
