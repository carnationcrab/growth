#include "world/PlanetGlobeGenerator.hpp"
#include "world/VoronoiSphereGenerator.hpp"

namespace growth {

void PlanetGlobeGenerator::run(const WorldSeed &world_seed, const PlanetGenome &planet_genome, VoronoiSphere &voronoi_sphere_out, size_t num_sites) const {
	VoronoiSphereGenerator voronoi_gen;
	voronoi_sphere_out = voronoi_gen.generate(world_seed, num_sites);

	// TODO: (2) Elevation/noise on sphere cells.
	// TODO: (3) Tectonic plates and plate movement.
	// TODO: (4) Biomes from climate/elevation.
	// TODO: (5) Rivers and drainage.
	(void)planet_genome;
}

} // namespace growth
