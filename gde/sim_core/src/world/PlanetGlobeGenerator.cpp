#include "world/PlanetGlobeGenerator.hpp"
#include "world/ElevationAssigner.hpp"
#include "world/MoistureAssigner.hpp"
#include "world/PlatePropertiesAssigner.hpp"
#include "world/SphereHalfEdgeMesh.hpp"
#include "world/TectonicPlateAssigner.hpp"
#include "world/TriangleValues.hpp"
#include "world/VoronoiSphereGenerator.hpp"

namespace growth {

void PlanetGlobeGenerator::run(const WorldSeed &world_seed, const PlanetGenome &planet_genome, size_t num_sites, double jitter_percent, size_t num_plate_regions, float temperature_01, float precipitation_01, PlanetGlobe &out) const {
	(void)planet_genome;

	VoronoiSphereGenerator voronoi_gen;
	out.voronoi = voronoi_gen.generate(world_seed, num_sites, static_cast<float>(jitter_percent));

	build_sphere_half_edge_mesh(out.voronoi, out.mesh);

	TectonicPlateAssigner plate_assigner;
	plate_assigner.assign(out.voronoi, world_seed, num_plate_regions, out.plates);

	PlatePropertiesAssigner props_assigner;
	props_assigner.assign(out.voronoi, out.plates, world_seed, out.plate_properties);

	ElevationAssigner elevation_assigner;
	elevation_assigner.assign(out.voronoi, out.plates, out.plate_properties, world_seed, out.region_elevation);

	MoistureAssigner moisture_assigner;
	moisture_assigner.assign(out.voronoi, out.plates, out.region_elevation, world_seed, temperature_01, precipitation_01, out.region_moisture);

	assign_triangle_values_from_regions(out.voronoi, out.region_elevation, out.region_moisture, out.triangle_values);
}

} // namespace growth
