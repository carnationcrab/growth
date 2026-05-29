#include "world/PlanetSurfaceAtlasBuilder.hpp"

namespace growth {

void PlanetSurfaceAtlasBuilder::build_from_globe(const PlanetGlobe &globe, PlanetSurfaceAtlas &out) {
	out.sites = globe.topology.sites;
	out.region_neighbours = globe.region_neighbours;
	out.region_to_plate = globe.plates.region_to_plate;
	out.region_elevation = globe.region_elevation.region_elevation;
	out.region_moisture = globe.region_moisture.region_moisture;
	out.region_count = static_cast<U32>(out.sites.size());

	out.topology_triangles.clear();
	const Size num_tri = globe.topology.triangles.size();
	out.topology_triangles.reserve(num_tri * 3);
	for (Size i = 0; i < num_tri; ++i) {
		const auto &t = globe.topology.triangles[i];
		out.topology_triangles.push_back(static_cast<U32>(t[0]));
		out.topology_triangles.push_back(static_cast<U32>(t[1]));
		out.topology_triangles.push_back(static_cast<U32>(t[2]));
	}
}

} // namespace growth
