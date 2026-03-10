#include "world/MoistureAssigner.hpp"
#include "world/VoronoiSphere.hpp"
#include "util/Random.hpp"
#include <cstddef>
#include <queue>
#include <unordered_set>
#include <vector>

namespace growth {

namespace {

	constexpr uint64_t k_moisture_stream = 0x1c2d3e4f5a6b7c8dULL;

	void build_site_neighbours(const VoronoiSphere &sphere, std::vector<std::vector<size_t>> &neighbours_out) {
		const size_t n = sphere.sites.size();
		neighbours_out.assign(n, std::vector<size_t>());
		std::vector<std::unordered_set<size_t>> seen(n);
		for (const auto &t : sphere.triangles) {
			const size_t a = t[0], b = t[1], c = t[2];
			if (a >= n || b >= n || c >= n) continue;
			auto add = [&](size_t u, size_t v) {
				if (u != v && seen[u].insert(v).second)
					neighbours_out[u].push_back(v);
			};
			add(a, b);
			add(a, c);
			add(b, a);
			add(b, c);
			add(c, a);
			add(c, b);
		}
	}

} // namespace

void MoistureAssigner::assign(const VoronoiSphere &voronoi_sphere, const TectonicPlates &tectonic_plates, const RegionElevation &region_elevation, const WorldSeed &world_seed, float temperature_01, float precipitation_01, RegionMoisture &out) const {
	const size_t num_regions = voronoi_sphere.sites.size();
	out.region_moisture.clear();
	if (num_regions == 0) return;
	out.region_moisture.resize(num_regions, 0.0f);

	const std::vector<int32_t> &r2p = tectonic_plates.region_to_plate;
	const std::vector<float> &elev = region_elevation.region_elevation;
	if (elev.size() < num_regions) return;

	random::RNG rng(world_seed.value ^ k_moisture_stream);

	for (size_t plate_id = 0; plate_id < tectonic_plates.num_plates; ++plate_id) {
		float base_moisture = static_cast<float>(rng.next_int(0, 9)) / 10.0f;
		for (size_t r = 0; r < num_regions; ++r) {
			if (static_cast<size_t>(r2p[r]) == plate_id)
				out.region_moisture[r] = base_moisture;
		}
	}

	std::vector<std::vector<size_t>> site_neighbours;
	build_site_neighbours(voronoi_sphere, site_neighbours);

	std::vector<float> dist_ocean(num_regions, 1e9f);
	std::queue<size_t> queue;
	for (size_t r = 0; r < num_regions; ++r) {
		if (elev[r] < 0.0f) {
			dist_ocean[r] = 0.0f;
			queue.push(r);
		}
	}
	while (!queue.empty()) {
		size_t cur = queue.front();
		queue.pop();
		float d_cur = dist_ocean[cur];
		for (size_t nb : site_neighbours[cur]) {
			if (nb >= num_regions) continue;
			float d_new = d_cur + 1.0f;
			if (d_new >= dist_ocean[nb]) continue;
			dist_ocean[nb] = d_new;
			queue.push(nb);
		}
	}

	const float decay = 0.15f;
	for (size_t r = 0; r < num_regions; ++r) {
		if (elev[r] < 0.0f) continue;
		float d = dist_ocean[r];
		if (d < 1e8f) {
			float moisture_from_ocean = 1.0f / (1.0f + decay * d);
			if (moisture_from_ocean > out.region_moisture[r])
				out.region_moisture[r] = moisture_from_ocean;
		}
	}

	// Climate multipliers: temperature (higher -> slightly lower moisture), precipitation (higher -> more moisture).
	const float temp_mult = 1.0f - 0.2f * temperature_01;
	const float precip_mult = 0.5f + 0.5f * precipitation_01;
	for (size_t r = 0; r < num_regions; ++r) {
		float m = out.region_moisture[r] * temp_mult * precip_mult;
		out.region_moisture[r] = (m < 0.0f) ? 0.0f : (m > 1.0f ? 1.0f : m);
	}
}

} // namespace growth
