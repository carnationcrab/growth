#include "util/FractalNoise.hpp"
#include "base/gateway/Cmath.hpp"
#include "base/gateway/Cstddef.hpp"

namespace growth {

namespace {

constexpr float k_rbg_position_scale  = 4.0f;
constexpr float k_rbg_noise_amplitude = 0.1f;

float fade(float t) {
	return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float lerp(float a, float b, float t) {
	return a + (b - a) * t;
}

uint64_t lattice_hash(int xi, int yi, int zi, uint64_t seed) {
	uint64_t h = seed;
	h ^= static_cast<uint64_t>(xi) * 0x9e3779b97f4a7c15ULL;
	h ^= static_cast<uint64_t>(yi) * 0xbf58476d1ce4e5b9ULL;
	h ^= static_cast<uint64_t>(zi) * 0x94d049bb133111ebULL;
	h ^= h >> 33;
	h *= 0xff51afd7ed558ccdULL;
	h ^= h >> 33;
	h *= 0xc4ceb9fe1a85ec53ULL;
	h ^= h >> 33;
	return h;
}

float lattice_value(int xi, int yi, int zi, uint64_t seed) {
	const uint64_t h = lattice_hash(xi, yi, zi, seed);
	return static_cast<float>(h & 0xFFFFu) / 65535.0f * 2.0f - 1.0f;
}

float noise3(float x, float y, float z, uint64_t seed) {
	const int x0 = static_cast<int>(Cmath::floor(x));
	const int y0 = static_cast<int>(Cmath::floor(y));
	const int z0 = static_cast<int>(Cmath::floor(z));
	const float tx = fade(x - static_cast<float>(x0));
	const float ty = fade(y - static_cast<float>(y0));
	const float tz = fade(z - static_cast<float>(z0));

	const float c000 = lattice_value(x0, y0, z0, seed);
	const float c100 = lattice_value(x0 + 1, y0, z0, seed);
	const float c010 = lattice_value(x0, y0 + 1, z0, seed);
	const float c110 = lattice_value(x0 + 1, y0 + 1, z0, seed);
	const float c001 = lattice_value(x0, y0, z0 + 1, seed);
	const float c101 = lattice_value(x0 + 1, y0, z0 + 1, seed);
	const float c011 = lattice_value(x0, y0 + 1, z0 + 1, seed);
	const float c111 = lattice_value(x0 + 1, y0 + 1, z0 + 1, seed);

	const float x00 = lerp(c000, c100, tx);
	const float x10 = lerp(c010, c110, tx);
	const float x01 = lerp(c001, c101, tx);
	const float x11 = lerp(c011, c111, tx);
	const float y0v = lerp(x00, x10, ty);
	const float y1v = lerp(x01, x11, ty);
	return lerp(y0v, y1v, tz);
}

} // namespace

float fbm3(const Vec3 &position, float frequency, int octaves, uint64_t seed) {
	if (octaves < 1)
		octaves = 1;

	float amplitude = 1.0f;
	float freq      = frequency;
	float sum       = 0.0f;
	float norm      = 0.0f;

	for (int i = 0; i < octaves; ++i) {
		const uint64_t octave_seed = seed ^ (static_cast<uint64_t>(i + 1) * 0xD1B54A32D192ED03ULL);
		sum += amplitude * noise3(position.x * freq, position.y * freq, position.z * freq, octave_seed);
		norm += amplitude;
		amplitude *= 0.5f;
		freq *= 2.0f;
	}

	return (norm > 0.0f) ? (sum / norm) : 0.0f;
}

float elevation_fbm_detail(const Vec3 &unit_site, const WorldSeed &world_seed, uint64_t stream_offset) {
	const uint64_t seed = world_seed.value ^ stream_offset;
	const float frequency = world_seed.frequency * k_rbg_position_scale;
	const float detail    = fbm3(unit_site, frequency, world_seed.octaves, seed);
	return k_rbg_noise_amplitude * world_seed.height_scale * detail;
}

} // namespace growth
