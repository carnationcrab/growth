#pragma once

#include <cstdint>

namespace growth {

/// Combined seed and world-generation parameters from user input (e.g. form answers).
struct WorldSeed {
	/// Deterministic seed value for terrain/noise.
	uint64_t value = 0;
	/// Height scale for terrain (e.g. vertical scale of noise).
	float height_scale = 1.0f;
	/// Number of octaves for fractal noise.
	int octaves = 4;
	/// Base frequency for noise (e.g. world-space scale).
	float frequency = 0.01f;
};

} // namespace growth
