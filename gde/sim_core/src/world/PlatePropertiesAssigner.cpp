#include "world/PlatePropertiesAssigner.hpp"
#include "world/VoronoiSphere.hpp"
#include "util/Random.hpp"
#include <cstddef>
#include <unordered_set>
#include <vector>

namespace growth {

namespace {

	constexpr uint64_t k_plate_props_stream = 0x8f5b3c1e9d6a4f2bULL;

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

void PlatePropertiesAssigner::assign(const VoronoiSphere &voronoi_sphere, const TectonicPlates &tectonic_plates, const WorldSeed &world_seed, PlateProperties &out) const {
	const size_t num_regions = voronoi_sphere.sites.size();
	const size_t num_plates = tectonic_plates.num_plates;
	out.num_plates = num_plates;
	out.plate_vector.clear();
	out.plate_is_ocean.clear();
	if (num_plates == 0 || num_regions == 0) return;

	std::vector<std::vector<size_t>> site_neighbours;
	build_site_neighbours(voronoi_sphere, site_neighbours);

	const Vec3 *sites = voronoi_sphere.sites.data();
	const std::vector<int32_t> &r2p = tectonic_plates.region_to_plate;

	out.plate_vector.resize(num_plates);
	out.plate_is_ocean.resize(num_plates);

	random::RNG rng(world_seed.value ^ k_plate_props_stream);

	for (size_t plate_id = 0; plate_id < num_plates; ++plate_id) {
		Vec3 centroid(0.0f, 0.0f, 0.0f);
		size_t plate_region_count = 0;
		for (size_t r = 0; r < num_regions; ++r) {
			if (static_cast<size_t>(r2p[r]) != plate_id) continue;
			centroid.x += sites[r].x;
			centroid.y += sites[r].y;
			centroid.z += sites[r].z;
			++plate_region_count;
		}
		if (plate_region_count == 0) {
			out.plate_vector[plate_id] = Vec3(1.0f, 0.0f, 0.0f);
			out.plate_is_ocean[plate_id] = false;
			continue;
		}
		float inv = 1.0f / static_cast<float>(plate_region_count);
		centroid.x *= inv;
		centroid.y *= inv;
		centroid.z *= inv;
		float clen = centroid.length();
		if (clen > 1e-9f)
			centroid = centroid / clen;
		else {
			out.plate_vector[plate_id] = Vec3(1.0f, 0.0f, 0.0f);
			out.plate_is_ocean[plate_id] = (rng.next_int(0, 9) < 5);
			continue;
		}

		// p0 = plate centre (centroid); p1 = a neighbour region (prefer different plate for boundary direction).
		size_t p1_r = static_cast<size_t>(-1);
		for (size_t r = 0; r < num_regions && p1_r == static_cast<size_t>(-1); ++r) {
			if (static_cast<size_t>(r2p[r]) != plate_id) continue;
			for (size_t n : site_neighbours[r]) {
				if (n >= num_regions) continue;
				if (static_cast<size_t>(r2p[n]) != plate_id) {
					p1_r = n;
					break;
				}
			}
		}
		if (p1_r == static_cast<size_t>(-1)) {
			for (size_t r = 0; r < num_regions && p1_r == static_cast<size_t>(-1); ++r) {
				if (static_cast<size_t>(r2p[r]) != plate_id) continue;
				if (!site_neighbours[r].empty())
					p1_r = site_neighbours[r][0];
			}
		}
		if (p1_r == static_cast<size_t>(-1)) {
			out.plate_vector[plate_id] = Vec3(1.0f, 0.0f, 0.0f);
		} else {
			Vec3 p1(sites[p1_r].x, sites[p1_r].y, sites[p1_r].z);
			Vec3 delta = p1 - centroid;
			float len = delta.length();
			out.plate_vector[plate_id] = (len > 1e-6f) ? delta.normalised() : Vec3(1.0f, 0.0f, 0.0f);
		}

		out.plate_is_ocean[plate_id] = (rng.next_int(0, 9) < 5);
	}
}

} // namespace growth
