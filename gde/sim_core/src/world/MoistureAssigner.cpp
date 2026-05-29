#include "world/MoistureAssigner.hpp"
#include "util/Parallel.hpp"
#include "util/ParallelBfs.hpp"
#include "util/Random.hpp"
#include "base/gateway/Cstddef.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

namespace {

constexpr uint64_t k_moisture_stream = 0x1c2d3e4f5a6b7c8dULL;

} // namespace

void MoistureAssigner::assign(const SphereTopology &topology,
                              const CsrGraph &region_neighbours,
                              const TectonicPlates &tectonic_plates,
                              const RegionElevation &region_elevation,
                              const WorldSeed &world_seed,
                              double temperature_01,
                              double precipitation_01,
                              RegionMoisture &out) const {
	const size_t num_regions = topology.sites.size();
	out.region_moisture.clear();
	if (num_regions == 0)
		return;
	out.region_moisture.resize(num_regions, 0.0f);

	const Vector<int32_t> &r2p = tectonic_plates.region_to_plate;
	const Vector<float> &elev  = region_elevation.region_elevation;
	if (elev.size() < num_regions)
		return;

	// Plate-base moisture: deterministic RNG stream picks a base per plate.
	random::RNG rng(world_seed.value ^ k_moisture_stream);
	Vector<float> plate_base(tectonic_plates.num_plates, 0.0f);
	for (size_t plate_id = 0; plate_id < tectonic_plates.num_plates; ++plate_id)
		plate_base[plate_id] = static_cast<float>(rng.next_int(0, 9)) / 10.0f;

	parallel::for_range(0, num_regions, 4096, [&](size_t r) {
		const int32_t pid = r2p[r];
		if (pid >= 0 && static_cast<size_t>(pid) < plate_base.size())
			out.region_moisture[r] = plate_base[pid];
	});

	// Ocean-distance BFS: sources are all ocean regions (elev < 0). Deterministic parallel BFS.
	Vector<U32> ocean_sources;
	for (size_t r = 0; r < num_regions; ++r) {
		if (elev[r] < 0.0f)
			ocean_sources.push_back(static_cast<U32>(r));
	}

	Vector<U32> dist_ocean;
	parallel::bfs_distance(region_neighbours, ocean_sources, dist_ocean);

	constexpr float k_decay = 0.15f;
	const float temp_mult   = 1.0f - 0.2f * static_cast<float>(temperature_01);
	const float precip_mult = 0.5f + 0.5f * static_cast<float>(precipitation_01);

	parallel::for_range(0, num_regions, 4096, [&](size_t r) {
		float m = out.region_moisture[r];
		if (elev[r] >= 0.0f) {
			const U32 d = (r < dist_ocean.size()) ? dist_ocean[r] : parallel::k_bfs_unvisited;
			if (d != parallel::k_bfs_unvisited) {
				const float moisture_from_ocean = 1.0f / (1.0f + k_decay * static_cast<float>(d));
				if (moisture_from_ocean > m)
					m = moisture_from_ocean;
			}
		}
		m *= temp_mult * precip_mult;
		out.region_moisture[r] = (m < 0.0f) ? 0.0f : (m > 1.0f ? 1.0f : m);
	});
}

} // namespace growth
