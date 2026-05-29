#include "gen/SeedGenerator.hpp"
#include "util/String.hpp"
#include "base/gateway/Cstdint.hpp"

namespace growth {

namespace {

	constexpr const char *key_seed = "seed";
	constexpr const char *key_height_scale = "height_scale";
	constexpr const char *key_octaves = "octaves";
	constexpr const char *key_frequency = "frequency";
}

WorldSeed SeedGenerator::from_seed(uint64_t user_seed) const {
	WorldSeed ws;
	ws.value = user_seed;
	ws.height_scale = 1.0f;
	ws.octaves = 5;
	ws.frequency = 0.025f;
	return ws;
}

WorldSeed SeedGenerator::from_form(const UnorderedMap<String, int64_t> &answers) const {
	WorldSeed ws = from_seed(0);

	uint64_t combined = 0;
	for (const auto &p : answers) {
		uint64_t kh = string_util::hash(p.first);
		uint64_t v = static_cast<uint64_t>(p.second);
		combined = string_util::mix_u64(combined, string_util::mix_u64(kh, v));
	}
	ws.value = combined;

	auto it = answers.find(key_seed);
	if (it != answers.end())
		ws.value = string_util::mix_u64(ws.value, static_cast<uint64_t>(it->second));

	it = answers.find(key_height_scale);
	if (it != answers.end())
		ws.height_scale = static_cast<float>(it->second);
	if (ws.height_scale <= 0.0f) ws.height_scale = 1.0f;

	it = answers.find(key_octaves);
	if (it != answers.end())
		ws.octaves = static_cast<int>(it->second);
	if (ws.octaves < 1) ws.octaves = 1;
	if (ws.octaves > 16) ws.octaves = 16;

	it = answers.find(key_frequency);
	if (it != answers.end())
		ws.frequency = static_cast<float>(it->second) * 0.001f;
	if (ws.frequency <= 0.0f) ws.frequency = 0.01f;

	return ws;
}

} // namespace growth
