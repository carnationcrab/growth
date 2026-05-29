#include "world/PlateElevationAssigner.hpp"
#include "util/Random.hpp"
#include "base/gateway/Cmath.hpp"
#include "base/gateway/Cstddef.hpp"

namespace growth {

namespace {

	constexpr uint64_t k_plate_elev_stream = 0x2d4e6f8a1b3c5d7eULL;

	float hash_noise(uint64_t seed, size_t plate_id) {
		uint64_t h = seed ^ (plate_id * 0x9e3779b97f4a7c15ULL);
		h ^= h >> 33;
		h *= 0xff51afd7ed558ccdULL;
		h ^= h >> 33;
		h *= 0xc4ceb9fe1a85ec53ULL;
		h ^= h >> 33;
		return (static_cast<float>(h & 0xFFFFu) / 65535.0f - 0.5f) * 0.15f;
	}

} // namespace

void PlateElevationAssigner::assign(const TectonicPlates &tectonic_plates, const PlateProperties &plate_properties, const WorldSeed &world_seed, PlateClimate &out) const {
	const size_t num_plates = tectonic_plates.num_plates;
	out.num_plates = num_plates;
	out.plate_elevation.clear();
	if (num_plates == 0) return;
	out.plate_elevation.resize(num_plates, 0.0f);

	const uint64_t noise_seed = world_seed.value ^ k_plate_elev_stream;
	for (size_t p = 0; p < num_plates; ++p) {
		if (plate_properties.plate_is_ocean[p])
			out.plate_elevation[p] = -0.5f + hash_noise(noise_seed, p);
		else
			out.plate_elevation[p] = 0.1f + hash_noise(noise_seed, p);
	}
}

} // namespace growth
