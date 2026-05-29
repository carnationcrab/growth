#pragma once

#include "gen/WorldSeed.hpp"
#include "math/Vec3.hpp"
#include "base/gateway/Cstdint.hpp"

namespace growth {

/// Deterministic 3D value-noise fractal Brownian motion in approximately [-1, 1].
float fbm3(const Vec3 &position, float frequency, int octaves, uint64_t seed);

/// RBG 1843-planet-generation elevation detail: 0.1 * fbm(normalised_site * scale).
float elevation_fbm_detail(const Vec3 &unit_site, const WorldSeed &world_seed, uint64_t stream_offset);

} // namespace growth
