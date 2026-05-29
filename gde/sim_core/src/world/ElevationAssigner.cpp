#include "world/ElevationAssigner.hpp"
#include "math/Vec3.hpp"
#include "util/FractalNoise.hpp"
#include "util/Parallel.hpp"
#include "util/ParallelBfs.hpp"
#include "util/Random.hpp"
#include "base/gateway/Calgorithm.hpp"
#include "base/gateway/Cmath.hpp"
#include "base/gateway/Cstddef.hpp"
#include "base/gateway/Climits.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

namespace {

constexpr uint64_t k_elevation_noise_stream = 0xa1b2c3d4e5f60718ULL;
constexpr float k_collision_delta_time      = 1e-2f;
constexpr float k_collision_threshold       = 0.75f * k_collision_delta_time;
constexpr float k_epsilon                   = 1e-3f;

void sort_unique_regions(Vector<U32> &regions) {
	Calgorithm::sort(regions.begin(), regions.end());
	regions.erase(Calgorithm::unique(regions.begin(), regions.end()), regions.end());
}

void add_seed(Vector<U32> &seeds, U32 region) {
	seeds.push_back(region);
}

/// Plate-boundary collision tagging aligned with Red Blob Games 1843-planet-generation:
/// land+land convergence seeds mountain distance fields at plate seed regions, not every boundary cell.
void find_boundary_seeds(const SphereTopology &topology,
                           const CsrGraph &region_neighbours,
                           const TectonicPlates &tectonic_plates,
                           const PlateProperties &plate_properties,
                           Vector<U32> &mountain_seeds,
                           Vector<U32> &coastline_seeds,
                           Vector<U32> &ocean_seeds) {
	const size_t num_regions           = topology.sites.size();
	const Vec3 *sites                  = topology.sites.data();
	const Vector<int32_t> &r2p         = tectonic_plates.region_to_plate;
	const Vector<U32> &plate_seed      = tectonic_plates.plate_seed_region;
	const Vector<Vec3> &plate_omega    = plate_properties.plate_omega;
	const Vector<bool> &plate_is_ocean = plate_properties.plate_is_ocean;

	mountain_seeds.clear();
	coastline_seeds.clear();
	ocean_seeds.clear();

	auto plate_seed_for = [&](int32_t plate_id) -> U32 {
		if (plate_id < 0 || static_cast<size_t>(plate_id) >= plate_seed.size())
			return static_cast<U32>(-1);
		return plate_seed[static_cast<size_t>(plate_id)];
	};

	// Serial collision pass (fast relative to BFS; keeps seed collection deterministic).
	for (size_t current_r = 0; current_r < num_regions; ++current_r) {
		const int32_t plate_id = r2p[current_r];
		if (plate_id < 0)
			continue;
		const Vec3 pos_cur = sites[current_r];
		const Vec3 vel_cur = plate_omega[static_cast<size_t>(plate_id)].cross(pos_cur);

		float best_compression = -1e9f;
		int best_neighbor_r    = -1;

		const U32 *nb_b = region_neighbours.neighbours_begin(current_r);
		const U32 *nb_e = region_neighbours.neighbours_end(current_r);
		for (const U32 *it = nb_b; it != nb_e; ++it) {
			const size_t neighbor_r = static_cast<size_t>(*it);
			if (neighbor_r >= num_regions || r2p[neighbor_r] == plate_id)
				continue;
			const int32_t other_plate = r2p[neighbor_r];
			if (other_plate < 0)
				continue;

			const Vec3 pos_neigh       = sites[neighbor_r];
			const Vec3 vel_neigh       = plate_omega[static_cast<size_t>(other_plate)].cross(pos_neigh);
			const float dist_before    = (pos_neigh - pos_cur).length();
			const float dist_after     = (pos_neigh + vel_neigh * k_collision_delta_time - (pos_cur + vel_cur * k_collision_delta_time)).length();
			const float compression    = dist_before - dist_after;

			if (compression > best_compression) {
				best_compression = compression;
				best_neighbor_r  = static_cast<int>(neighbor_r);
			}
		}

		if (best_neighbor_r < 0)
			continue;

		const bool collided    = best_compression > k_collision_threshold;
		const bool cur_ocean   = plate_is_ocean[static_cast<size_t>(plate_id)];
		const int32_t other_plate = r2p[static_cast<size_t>(best_neighbor_r)];
		const bool other_ocean = plate_is_ocean[static_cast<size_t>(other_plate)];

		if (cur_ocean && other_ocean) {
			add_seed(collided ? coastline_seeds : ocean_seeds, static_cast<U32>(current_r));
		} else if (!cur_ocean && !other_ocean) {
			if (collided) {
				const U32 seed_cur   = plate_seed_for(plate_id);
				const U32 seed_other = plate_seed_for(other_plate);
				if (seed_cur != static_cast<U32>(-1))
					add_seed(mountain_seeds, seed_cur);
				if (seed_other != static_cast<U32>(-1))
					add_seed(mountain_seeds, seed_other);
			}
		} else if (collided) {
			add_seed(mountain_seeds, static_cast<U32>(current_r));
		} else {
			add_seed(coastline_seeds, static_cast<U32>(current_r));
		}
	}

	// Each plate seed is ocean or coastline when not already a mountain seed (RBG loop over r_plate[r] === r).
	for (size_t p = 0; p < tectonic_plates.num_plates; ++p) {
		const U32 seed_r = plate_seed_for(static_cast<int32_t>(p));
		if (seed_r >= num_regions)
			continue;
		const bool is_ocean = plate_is_ocean[p];
		add_seed(is_ocean ? ocean_seeds : coastline_seeds, seed_r);
	}

	sort_unique_regions(mountain_seeds);
	sort_unique_regions(coastline_seeds);
	sort_unique_regions(ocean_seeds);
}

Vec3 normalised_site(const Vec3 &site) {
	const float len = site.length();
	return (len > 1e-6f) ? (site / len) : Vec3(0.0f, 0.0f, 1.0f);
}

} // namespace

void ElevationAssigner::assign(const SphereTopology &topology,
                               const CsrGraph &region_neighbours,
                               const TectonicPlates &tectonic_plates,
                               const PlateProperties &plate_properties,
                               const WorldSeed &world_seed,
                               RegionElevation &out) const {
	const size_t num_regions = topology.sites.size();
	out.region_elevation.clear();
	if (num_regions == 0)
		return;
	out.region_elevation.resize(num_regions, 0.0f);

	Vector<U32> mountain_seeds, coastline_seeds, ocean_seeds;
	find_boundary_seeds(topology, region_neighbours, tectonic_plates, plate_properties,
	                    mountain_seeds, coastline_seeds, ocean_seeds);

	auto contains = [](const Vector<U32> &sorted, U32 v) {
		const auto it = Calgorithm::lower_bound(sorted.begin(), sorted.end(), v);
		return it != sorted.end() && *it == v;
	};

	Vector<U32> dist_a, dist_b, dist_c;
	// RBG: mountain field stops at ocean seeds; ocean field stops at coastline; coastline stops at any seed.
	parallel::bfs_distance(region_neighbours, mountain_seeds,
	                       [&](U32 v) { return contains(ocean_seeds, v); }, dist_a);
	parallel::bfs_distance(region_neighbours, ocean_seeds,
	                       [&](U32 v) { return contains(coastline_seeds, v); }, dist_b);
	parallel::bfs_distance(region_neighbours, coastline_seeds,
	                       [&](U32 v) {
		                       return contains(mountain_seeds, v) || contains(ocean_seeds, v) || contains(coastline_seeds, v);
	                       },
	                       dist_c);

	const uint64_t noise_seed = world_seed.value ^ k_elevation_noise_stream;
	const Vec3 *sites_ptr     = topology.sites.data();
	parallel::for_range(0, num_regions, 4096, [&](size_t r) {
		const U32 da = (r < dist_a.size()) ? dist_a[r] : parallel::k_bfs_unvisited;
		const U32 db = (r < dist_b.size()) ? dist_b[r] : parallel::k_bfs_unvisited;
		const U32 dc = (r < dist_c.size()) ? dist_c[r] : parallel::k_bfs_unvisited;
		const Vec3 unit = normalised_site(sites_ptr[r]);
		const float noise = elevation_fbm_detail(unit, world_seed, noise_seed);
		if (da == parallel::k_bfs_unvisited && db == parallel::k_bfs_unvisited) {
			out.region_elevation[r] = 0.1f + noise;
			return;
		}
		const float a = (da == parallel::k_bfs_unvisited ? 1e6f : static_cast<float>(da)) + k_epsilon;
		const float b = (db == parallel::k_bfs_unvisited ? 1e6f : static_cast<float>(db)) + k_epsilon;
		const float c = (dc == parallel::k_bfs_unvisited ? 1e6f : static_cast<float>(dc)) + k_epsilon;
		const float inv_a = 1.0f / a;
		const float inv_b = 1.0f / b;
		const float inv_c = 1.0f / c;
		out.region_elevation[r] = (inv_a - inv_b) / (inv_a + inv_b + inv_c) + noise;
	});
}

} // namespace growth
