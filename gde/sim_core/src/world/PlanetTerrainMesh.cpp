#include "world/PlanetTerrainMesh.hpp"
#include "world/PlanetGlobe.hpp"
#include "world/VoronoiSphere.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

namespace growth {

static Vec3 normalised_vec3(const Vec3 &v) {
	float l = v.length();
	return (l > 0.0f) ? (v / l) : Vec3(0.0f, 0.0f, 0.0f);
}

void generate_planet_terrain_mesh_voronoi_triangles(const PlanetGlobe &globe, PlanetTerrainMesh &out) {
	out.vertices.clear();
	out.normals.clear();
	out.indices.clear();
	out.river_strength.clear();

	const VoronoiSphere &v = globe.voronoi;
	const size_t num_regions = v.sites.size();
	const size_t num_tri = v.circumcenters.size();
	if (num_regions == 0 || num_tri == 0)
		return;

	// Vertices: [region centres, triangle centres]
	out.vertices.reserve(num_regions + num_tri);
	out.normals.reserve(num_regions + num_tri);
	for (size_t i = 0; i < num_regions; ++i) {
		Vec3 p = v.sites[i];
		out.vertices.push_back(p);
		out.normals.push_back(normalised_vec3(p));
	}
	for (size_t i = 0; i < num_tri; ++i) {
		Vec3 p = v.circumcenters[i];
		out.vertices.push_back(p);
		out.normals.push_back(normalised_vec3(p));
	}

	// Triangles: each Voronoi cell = fan from region centre to circumcenters (cell list)
	for (size_t r = 0; r < v.cells.size() && r < num_regions; ++r) {
		const std::vector<size_t> &cell = v.cells[r];
		if (cell.size() < 2)
			continue;
		for (size_t j = 0; j < cell.size(); ++j) {
			size_t c0 = cell[j];
			size_t c1 = cell[(j + 1) % cell.size()];
			if (c0 >= num_tri || c1 >= num_tri)
				continue;
			uint32_t i_r = static_cast<uint32_t>(r);
			uint32_t i_c0 = static_cast<uint32_t>(num_regions + c0);
			uint32_t i_c1 = static_cast<uint32_t>(num_regions + c1);
			out.indices.push_back(i_r);
			out.indices.push_back(i_c0);
			out.indices.push_back(i_c1);
			out.river_strength.push_back(0.0f);
		}
	}
}

// Displacement scale: radius = 1 + k * elevation; land bulges out, ocean dips in.
static constexpr float k_elevation_scale = 0.08f;

void generate_planet_terrain_mesh_quad(const PlanetGlobe &globe, PlanetTerrainMesh &out) {
	out.vertices.clear();
	out.normals.clear();
	out.indices.clear();
	out.river_strength.clear();

	const VoronoiSphere &voronoi = globe.voronoi;
	const std::vector<float> &r_elevation = globe.region_elevation.region_elevation;
	const RiverFlow &river = globe.river_flow;
	const size_t num_regions = voronoi.sites.size();
	const size_t num_tri = voronoi.triangles.size();
	std::cerr << "[PlanetTerrainMesh] displaced sphere: num_regions=" << num_regions << " num_tri=" << num_tri << "\n";
	if (num_regions == 0 || num_tri == 0) {
		std::cerr << "[PlanetTerrainMesh] early return: no sites or triangles\n";
		return;
	}

	// 1) Vertices: one per Voronoi site. Start from unit sphere, displace radially by elevation.
	out.vertices.reserve(num_regions);
	out.normals.reserve(num_regions);
	for (size_t r = 0; r < num_regions; ++r) {
		Vec3 dir = normalised_vec3(voronoi.sites[r]);
		float elev = (r < r_elevation.size()) ? r_elevation[r] : 0.0f;
		float radius = 1.0f + k_elevation_scale * elev;
		Vec3 pos = dir * radius;
		out.vertices.push_back(pos);
		out.normals.push_back(normalised_vec3(pos)); // normal follows displaced surface
	}

	// 2) Indices: Delaunay triangles (voronoi.triangles = 3 site indices per triangle). Guaranteed connected.
	out.indices.reserve(num_tri * 3);
	for (size_t t = 0; t < num_tri; ++t) {
		const auto &tri = voronoi.triangles[t];
		out.indices.push_back(static_cast<uint32_t>(tri[0]));
		out.indices.push_back(static_cast<uint32_t>(tri[1]));
		out.indices.push_back(static_cast<uint32_t>(tri[2]));
	}

	// 3) River strength per triangle: average flow on the three edges (half-edges 3*t, 3*t+1, 3*t+2).
	out.river_strength.reserve(num_tri);
	for (size_t t = 0; t < num_tri; ++t) {
		float sum = 0.0f;
		for (int e = 0; e < 3; ++e) {
			size_t s = t * 3u + static_cast<size_t>(e);
			if (s < river.s_flow.size())
				sum += river.s_flow[s];
		}
		out.river_strength.push_back(sum / 3.0f);
	}

	std::cerr << "[PlanetTerrainMesh] displaced sphere done: vertices=" << out.vertices.size() << " indices=" << (out.indices.size()) << " triangles=" << num_tri << "\n";
}

} // namespace growth
