#include "world/PlateMoistureAssigner.hpp"
#include "util/Random.hpp"
#include "base/gateway/Calgorithm.hpp"
#include "base/gateway/Cmath.hpp"
#include "base/gateway/Cstddef.hpp"

namespace growth {

namespace {

	constexpr uint64_t k_plate_moisture_stream = 0x1c2d3e4f5a6b7c8dULL;

} // namespace

void PlateMoistureAssigner::assign(const TectonicPlates &tectonic_plates, const WorldSeed &world_seed, float temperature_01, float precipitation_01, PlateClimate &out) const {
	const size_t num_plates = tectonic_plates.num_plates;
	out.num_plates = num_plates;
	out.plate_moisture.clear();
	if (num_plates == 0) return;
	out.plate_moisture.resize(num_plates, 0.5f);

	random::RNG rng(world_seed.value ^ k_plate_moisture_stream);
	for (size_t p = 0; p < num_plates; ++p) {
		float base = static_cast<float>(rng.next_int(0, 9)) / 10.0f;
		out.plate_moisture[p] = base;
	}

	const float temp_mult = 1.0f - 0.2f * temperature_01;
	const float precip_mult = 0.5f + 0.5f * precipitation_01;
	for (size_t p = 0; p < num_plates; ++p) {
		float m = out.plate_moisture[p] * temp_mult * precip_mult;
		out.plate_moisture[p] = Calgorithm::max(0.0f, Calgorithm::min(1.0f, m));
	}
}

} // namespace growth
