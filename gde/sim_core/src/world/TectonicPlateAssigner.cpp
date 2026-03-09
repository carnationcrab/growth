#include "world/TectonicPlateAssigner.hpp"
#include "world/VoronoiSphere.hpp"
#include "util/Random.hpp"
#include <cstddef>
#include <iostream>
#include <set>
#include <unordered_set>
#include <vector>

namespace growth {

namespace {

	constexpr uint64_t k_tectonic_stream = 0x7e4a9c2b1d5f8e3aULL;

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

void TectonicPlateAssigner::assign(const VoronoiSphere &voronoi_sphere, const WorldSeed &world_seed, size_t num_plates, TectonicPlates &tectonic_out) const {
	const size_t num_regions = voronoi_sphere.sites.size();
	std::cerr << "[TectonicPlateAssigner] assign: num_regions=" << num_regions << " requested_plates=" << num_plates << "\n";
	tectonic_out.region_to_plate.clear();
	tectonic_out.num_plates = 0;
	if (num_regions == 0) {
		std::cerr << "[TectonicPlateAssigner] skip: no regions\n";
		return;
	}

	// Clamp num_plates to [1, num_regions]
	if (num_plates < 1u) num_plates = 1;
	if (num_plates > num_regions) num_plates = num_regions;

	random::RNG rng(world_seed.value ^ k_tectonic_stream);

	// 1) Pick num_plates distinct region indices as plate seeds (plate id = index into this list; seed region gets that plate id).
	std::set<size_t> chosen_r;
	while (chosen_r.size() < num_plates) {
		size_t r = static_cast<size_t>(rng.next_int(0, static_cast<int64_t>(num_regions) - 1));
		chosen_r.insert(r);
	}
	std::vector<size_t> plate_seeds(chosen_r.begin(), chosen_r.end());

	// 2) region_to_plate[site] = plate id (0 .. num_plates-1), or -1 unassigned.
	tectonic_out.region_to_plate.assign(num_regions, -1);
	for (size_t i = 0; i < plate_seeds.size(); ++i)
		tectonic_out.region_to_plate[plate_seeds[i]] = static_cast<int32_t>(i);

	// 3) Build site adjacency from Delaunay triangles.
	std::vector<std::vector<size_t>> site_neighbours;
	build_site_neighbours(voronoi_sphere, site_neighbours);

	// 4) Random fill: queue of region indices; at each step pick random element from current queue tail, expand to unassigned neighbours.
	std::vector<size_t> queue;
	queue.reserve(num_regions);
	for (size_t r : plate_seeds)
		queue.push_back(r);

	for (size_t queue_out = 0; queue_out < queue.size(); ++queue_out) {
		// Random index in [queue_out, queue.size())
		const size_t len = queue.size() - queue_out;
		if (len == 0) break;
		size_t pos = queue_out + static_cast<size_t>(rng.next_int(0, static_cast<int64_t>(len) - 1));
		size_t current_r = queue[pos];
		queue[pos] = queue[queue_out];

		const int32_t plate_id = tectonic_out.region_to_plate[current_r];
		if (plate_id < 0) continue;

		for (size_t neighbor_r : site_neighbours[current_r]) {
			if (neighbor_r >= num_regions) continue;
			if (tectonic_out.region_to_plate[neighbor_r] == -1) {
				tectonic_out.region_to_plate[neighbor_r] = plate_id;
				queue.push_back(neighbor_r);
			}
		}
	}

	tectonic_out.num_plates = num_plates;
	size_t assigned = 0;
	for (int32_t p : tectonic_out.region_to_plate)
		if (p >= 0) ++assigned;
	std::cerr << "[TectonicPlateAssigner] done: num_plates=" << tectonic_out.num_plates << " regions_assigned=" << assigned << "/" << num_regions << "\n";
}

} // namespace growth
