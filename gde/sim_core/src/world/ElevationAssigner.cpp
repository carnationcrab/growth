#include "world/ElevationAssigner.hpp"
#include "world/VoronoiSphere.hpp"
#include "math/Vec3.hpp"
#include "util/Random.hpp"
#include <cmath>
#include <cstddef>
#include <limits>
#include <queue>
#include <set>
#include <unordered_set>
#include <vector>

namespace growth {

namespace {

	constexpr uint64_t k_elevation_noise_stream = 0xa1b2c3d4e5f60718ULL;
	constexpr float k_collision_delta_time = 1e-2f;
	constexpr float k_collision_threshold = 0.75f * k_collision_delta_time;
	constexpr float k_epsilon = 1e-3f;

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

	void assign_distance_field(const std::vector<std::vector<size_t>> &neighbours, const std::set<size_t> &seeds_r, const std::set<size_t> &stop_r, std::vector<float> &r_distance) {
		const size_t num_regions = neighbours.size();
		r_distance.assign(num_regions, std::numeric_limits<float>::infinity());
		std::queue<size_t> queue;
		for (size_t r : seeds_r) {
			if (r < num_regions) {
				r_distance[r] = 0.0f;
				queue.push(r);
			}
		}
		while (!queue.empty()) {
			size_t current_r = queue.front();
			queue.pop();
			float d_cur = r_distance[current_r];
			for (size_t neighbor_r : neighbours[current_r]) {
				if (neighbor_r >= num_regions) continue;
				if (stop_r.count(neighbor_r)) continue;
				float &d_neigh = r_distance[neighbor_r];
				if (d_neigh <= d_cur + 0.5f) continue;
				d_neigh = d_cur + 1.0f;
				queue.push(neighbor_r);
			}
		}
	}

	void find_collisions(const VoronoiSphere &voronoi_sphere, const std::vector<std::vector<size_t>> &site_neighbours, const TectonicPlates &tectonic_plates, const PlateProperties &plate_properties, std::set<size_t> &mountain_r, std::set<size_t> &coastline_r, std::set<size_t> &ocean_r) {
		const size_t num_regions = voronoi_sphere.sites.size();
		const Vec3 *sites = voronoi_sphere.sites.data();
		const std::vector<int32_t> &r2p = tectonic_plates.region_to_plate;
		const std::vector<Vec3> &plate_vec = plate_properties.plate_vector;
		const std::vector<bool> &plate_is_ocean = plate_properties.plate_is_ocean;

		mountain_r.clear();
		coastline_r.clear();
		ocean_r.clear();

		for (size_t current_r = 0; current_r < num_regions; ++current_r) {
			int32_t plate_id = r2p[current_r];
			if (plate_id < 0) continue;
			Vec3 pos_cur(sites[current_r].x, sites[current_r].y, sites[current_r].z);
			Vec3 vel_cur = plate_vec[static_cast<size_t>(plate_id)];

			float best_compression = -1e9f;
			int best_neighbor_r = -1;

			for (size_t neighbor_r : site_neighbours[current_r]) {
				if (neighbor_r >= num_regions) continue;
				if (r2p[neighbor_r] == plate_id) continue;
				int32_t other_plate = r2p[neighbor_r];
				if (other_plate < 0) continue;

				Vec3 pos_neigh(sites[neighbor_r].x, sites[neighbor_r].y, sites[neighbor_r].z);
				Vec3 vel_neigh = plate_vec[static_cast<size_t>(other_plate)];

				float dist_before = (pos_neigh - pos_cur).length();
				Vec3 pos_cur_after = pos_cur + vel_cur * k_collision_delta_time;
				Vec3 pos_neigh_after = pos_neigh + vel_neigh * k_collision_delta_time;
				float dist_after = (pos_neigh_after - pos_cur_after).length();
				float compression = dist_before - dist_after;

				if (compression > best_compression) {
					best_compression = compression;
					best_neighbor_r = static_cast<int>(neighbor_r);
				}
			}

			if (best_neighbor_r < 0) continue;

			bool collided = best_compression > k_collision_threshold;
			bool cur_ocean = plate_is_ocean[static_cast<size_t>(plate_id)];
			bool other_ocean = plate_is_ocean[static_cast<size_t>(r2p[static_cast<size_t>(best_neighbor_r)])];

			if (cur_ocean && other_ocean) {
				if (collided) coastline_r.insert(current_r);
				else ocean_r.insert(current_r);
			} else if (!cur_ocean && !other_ocean) {
				if (collided) mountain_r.insert(current_r);
			} else {
				if (collided) mountain_r.insert(current_r);
				else coastline_r.insert(current_r);
			}
		}

		for (size_t plate_id = 0; plate_id < tectonic_plates.num_plates; ++plate_id) {
			size_t seed_r = static_cast<size_t>(-1);
			for (size_t r = 0; r < num_regions; ++r) {
				if (static_cast<size_t>(r2p[r]) == plate_id) {
					seed_r = r;
					break;
				}
			}
			if (seed_r == static_cast<size_t>(-1)) continue;
			if (plate_properties.plate_is_ocean[plate_id])
				ocean_r.insert(seed_r);
			else
				coastline_r.insert(seed_r);
		}
	}

	float hash_noise(uint64_t seed, size_t r) {
		uint64_t h = seed ^ (r * 0x9e3779b97f4a7c15ULL);
		h ^= h >> 33;
		h *= 0xff51afd7ed558ccdULL;
		h ^= h >> 33;
		h *= 0xc4ceb9fe1a85ec53ULL;
		h ^= h >> 33;
		return (static_cast<float>(h & 0xFFFFu) / 65535.0f - 0.5f) * 0.2f;
	}

} // namespace

void ElevationAssigner::assign(const VoronoiSphere &voronoi_sphere, const TectonicPlates &tectonic_plates, const PlateProperties &plate_properties, const WorldSeed &world_seed, RegionElevation &out) const {
	const size_t num_regions = voronoi_sphere.sites.size();
	out.region_elevation.clear();
	if (num_regions == 0) return;
	out.region_elevation.resize(num_regions, 0.0f);

	std::vector<std::vector<size_t>> site_neighbours;
	build_site_neighbours(voronoi_sphere, site_neighbours);

	std::set<size_t> mountain_r, coastline_r, ocean_r;
	find_collisions(voronoi_sphere, site_neighbours, tectonic_plates, plate_properties, mountain_r, coastline_r, ocean_r);

	std::set<size_t> stop_r;
	for (size_t r : mountain_r) stop_r.insert(r);
	for (size_t r : coastline_r) stop_r.insert(r);
	for (size_t r : ocean_r) stop_r.insert(r);

	std::vector<float> r_distance_a, r_distance_b, r_distance_c;
	assign_distance_field(site_neighbours, mountain_r, ocean_r, r_distance_a);
	assign_distance_field(site_neighbours, ocean_r, coastline_r, r_distance_b);
	assign_distance_field(site_neighbours, coastline_r, stop_r, r_distance_c);

	const uint64_t noise_seed = world_seed.value ^ k_elevation_noise_stream;
	for (size_t r = 0; r < num_regions; ++r) {
		float a = r_distance_a[r] + k_epsilon;
		float b = r_distance_b[r] + k_epsilon;
		float c = r_distance_c[r] + k_epsilon;
		if (a == std::numeric_limits<float>::infinity() && b == std::numeric_limits<float>::infinity()) {
			out.region_elevation[r] = 0.1f + hash_noise(noise_seed, r);
		} else {
			float inv_a = 1.0f / a;
			float inv_b = 1.0f / b;
			float inv_c = 1.0f / c;
			out.region_elevation[r] = (inv_a - inv_b) / (inv_a + inv_b + inv_c) + hash_noise(noise_seed, r);
		}
	}
}

} // namespace growth
