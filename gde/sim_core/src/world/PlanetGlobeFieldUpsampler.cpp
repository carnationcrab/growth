#include "world/PlanetGlobeFieldUpsampler.hpp"
#include "world/ElevationAssigner.hpp"
#include "world/IcosphereEngine.hpp"
#include "world/MoistureAssigner.hpp"
#include "world/PlatePropertiesAssigner.hpp"
#include "world/RegionNeighbourIndex.hpp"
#include "world/SphereDual.hpp"
#include "world/SphereHalfEdgeMesh.hpp"
#include "world/SphereTopologyEngine.hpp"
#include "world/TectonicPlateAssigner.hpp"
#include "util/Parallel.hpp"
#include "util/ParallelBfs.hpp"
#include "util/PerformanceLogger.hpp"
#include "base/gateway/Carray.hpp"

namespace growth {

namespace {

// Deterministic multi-source BFS on the fine region graph: each fine region inherits the id of
// its nearest coarse seed region (graph distance, smallest-id tie-break). O(N_fine), parallel.
// Works with jitter — unlike positional prefix matching, which falsely fell back to O(N_fine × N_coarse) NN.
void label_fine_to_coarse_via_bfs(const PlanetGlobe &fine, Size num_coarse, Vector<U32> &fine_to_coarse_out) {
	const Size num_fine = fine.topology.sites.size();
	Vector<U32> sources;
	Vector<U32> labels;
	sources.reserve(num_coarse);
	labels.reserve(num_coarse);
	for (U32 c = 0; c < static_cast<U32>(num_coarse); ++c) {
		sources.push_back(c);
		labels.push_back(c);
	}
	parallel::bfs_label(fine.region_neighbours, sources, labels, fine_to_coarse_out);
	for (Size r = 0; r < num_fine; ++r) {
		if (fine_to_coarse_out[r] == parallel::k_bfs_unlabeled)
			fine_to_coarse_out[r] = 0u;
	}
}

// Rare fallback when icosphere embedding does not apply.
void label_fine_to_coarse_via_nn(const PlanetGlobe &fine,
                                 const SphereTopology &coarse,
                                 Vector<U32> &fine_to_coarse_out) {
	const Size num_fine   = fine.topology.sites.size();
	const Size num_coarse = coarse.sites.size();
	fine_to_coarse_out.assign(num_fine, 0u);
	if (num_coarse == 0)
		return;
	const Vec3 *fine_sites   = fine.topology.sites.data();
	const Vec3 *coarse_sites = coarse.sites.data();
	parallel::for_range(0, num_fine, 256, [&](Size r) {
		const Vec3 &p = fine_sites[r];
		U32 best        = 0;
		double best_dot = -2.0;
		for (Size c = 0; c < num_coarse; ++c) {
			const double d = p.dot(coarse_sites[c]);
			if (d > best_dot) {
				best_dot = d;
				best     = static_cast<U32>(c);
			}
		}
		fine_to_coarse_out[r] = best;
	});
}

void upsample_region_fields(const PlanetGlobe &coarse, PlanetGlobe &fine) {
	const Size num_fine   = fine.topology.sites.size();
	const Size num_coarse = coarse.topology.sites.size();
	fine.plates.region_to_plate.assign(num_fine, -1);
	fine.region_elevation.region_elevation.assign(num_fine, 0.0f);
	fine.region_moisture.region_moisture.assign(num_fine, 0.0f);

	Vector<U32> fine_to_coarse;
	const bool use_bfs = num_coarse > 0 && num_fine >= num_coarse && !fine.region_neighbours.empty()
	                     && IcosphereEngine::fine_embeds_coarse(num_fine, num_coarse);
	if (use_bfs)
		label_fine_to_coarse_via_bfs(fine, num_coarse, fine_to_coarse);
	else
		label_fine_to_coarse_via_nn(fine, coarse.topology, fine_to_coarse);

	const Vector<float> &coarse_elev  = coarse.region_elevation.region_elevation;
	const Vector<float> &coarse_moist = coarse.region_moisture.region_moisture;
	const Vector<int32_t> &coarse_r2p = coarse.plates.region_to_plate;

	parallel::for_range(0, num_fine, 1024, [&](Size r) {
		const Size c = static_cast<Size>(fine_to_coarse[r]);
		if (c < coarse_r2p.size())
			fine.plates.region_to_plate[r] = coarse_r2p[c];
		if (c < coarse_elev.size())
			fine.region_elevation.region_elevation[r] = coarse_elev[c];
		if (c < coarse_moist.size())
			fine.region_moisture.region_moisture[r] = coarse_moist[c];
	});
	fine.plates.num_plates = coarse.plates.num_plates;
	fine.plate_properties  = coarse.plate_properties;
}

void run_coarse_pipeline(const WorldSeed &world_seed,
                         const PlanetGenome &planet_genome,
                         Size coarse_sites,
                         double jitter_percent,
                         Size num_plate_regions,
                         double temperature_01,
                         double precipitation_01,
                         PlanetGlobe &coarse_out) {
	(void)planet_genome;
	SphereTopologyEngine topology_engine;
	coarse_out.topology = topology_engine.generate(world_seed, coarse_sites, jitter_percent);
	build_sphere_half_edge_mesh(coarse_out.topology, coarse_out.mesh);
	RegionNeighbourIndex::build(coarse_out.mesh, coarse_out.region_neighbours);
	SphereDual::build_region_triangle_rings(coarse_out.mesh, coarse_out.region_triangle_rings);

	TectonicPlateAssigner plate_assigner;
	plate_assigner.assign(
		coarse_out.topology, coarse_out.region_neighbours, world_seed, num_plate_regions, coarse_out.plates);

	PlatePropertiesAssigner props_assigner;
	props_assigner.assign(
		coarse_out.topology, coarse_out.region_neighbours, coarse_out.plates, world_seed, coarse_out.plate_properties);

	ElevationAssigner elev_assigner;
	elev_assigner.assign(coarse_out.topology,
	                     coarse_out.region_neighbours,
	                     coarse_out.plates,
	                     coarse_out.plate_properties,
	                     world_seed,
	                     coarse_out.region_elevation);

	MoistureAssigner moist_assigner;
	moist_assigner.assign(coarse_out.topology,
	                      coarse_out.region_neighbours,
	                      coarse_out.plates,
	                      coarse_out.region_elevation,
	                      world_seed,
	                      temperature_01,
	                      precipitation_01,
	                      coarse_out.region_moisture);
}

} // namespace

void PlanetGlobeFieldUpsampler::run_coarse_sim_and_upsample(const WorldSeed &world_seed,
                                                            const PlanetGenome &planet_genome,
                                                            Size coarse_sites,
                                                            double jitter_percent,
                                                            Size num_plate_regions,
                                                            double temperature_01,
                                                            double precipitation_01,
                                                            PlanetGlobe &fine_globe,
                                                            perf::PerformanceLogger *perf) {
	if (coarse_sites == 0 || fine_globe.topology.sites.size() <= coarse_sites)
		return;

	(void)perf;
	PlanetGlobe coarse;
	run_coarse_pipeline(
		world_seed, planet_genome, coarse_sites, jitter_percent, num_plate_regions, temperature_01, precipitation_01, coarse);

	upsample_region_fields(coarse, fine_globe);

	Vector<Vec3>().swap(coarse.topology.sites);
	Vector<Array<Size, 3>>().swap(coarse.topology.triangles);
	Vector<Vec3>().swap(coarse.mesh.r_xyz);
	Vector<Vec3>().swap(coarse.mesh.t_xyz);
	Vector<Size>().swap(coarse.mesh.s_begin_r);
	Vector<Size>().swap(coarse.mesh.s_end_r);
	Vector<Size>().swap(coarse.mesh.s_inner_t);
	Vector<Size>().swap(coarse.mesh.s_outer_t);
	Vector<Size>().swap(coarse.mesh.s_next_s);
	Vector<Size>().swap(coarse.mesh.s_twin_s);
	coarse.region_neighbours.clear();
	coarse.region_triangle_rings.clear();
}

} // namespace growth
