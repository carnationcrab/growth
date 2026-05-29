#include "world/PlanetGlobePipeline.hpp"

#include "world/ElevationAssigner.hpp"
#include "world/MoistureAssigner.hpp"
#include "world/PlanetGlobeFieldUpsampler.hpp"
#include "world/TriangleValues.hpp"
#include "world/PlanetTerrainMesh.hpp"
#include "world/PlatePropertiesAssigner.hpp"
#include "world/RegionNeighbourIndex.hpp"
#include "world/HydraulicErosion.hpp"
#include "world/RegionElevationSmooth.hpp"
#include "world/RegionPriorityFlood.hpp"
#include "world/RiverCarve.hpp"
#include "world/RiverFlow.hpp"
#include "world/SphereDual.hpp"
#include "world/SphereHalfEdgeMesh.hpp"
#include "world/SphereTopologyEngine.hpp"
#include "world/TectonicPlateAssigner.hpp"
#include "world/TriangleValues.hpp"
#include "util/PerformanceLogger.hpp"
#include "base/gateway/Cunordered_map.hpp"
#include "base/gateway/Cutility.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

namespace {

	using StageFn = void (*)(PlanetGlobePipelineContext &);

	void stage_topology(PlanetGlobePipelineContext &ctx) {
		SphereTopologyEngine engine;
		ctx.globe->topology = engine.generate(*ctx.world_seed, ctx.num_sites, ctx.jitter_percent);
	}

	void stage_half_edge_mesh(PlanetGlobePipelineContext &ctx) {
		build_sphere_half_edge_mesh(ctx.globe->topology, ctx.globe->mesh);
	}

	void stage_region_neighbours(PlanetGlobePipelineContext &ctx) {
		RegionNeighbourIndex::build(ctx.globe->mesh, ctx.globe->region_neighbours);
		SphereDual::build_region_triangle_rings(ctx.globe->mesh, ctx.globe->region_triangle_rings);
	}

	void stage_coarse_sim_upsample(PlanetGlobePipelineContext &ctx) {
		if (!ctx.use_coarse_sim_upsample())
			return;
		PlanetGlobeFieldUpsampler::run_coarse_sim_and_upsample(
			*ctx.world_seed,
			*ctx.planet_genome,
			ctx.sim_num_sites,
			ctx.jitter_percent,
			ctx.num_plate_regions,
			ctx.temperature_01,
			ctx.precipitation_01,
			*ctx.globe,
			ctx.perf);
		ctx.coarse_sim_applied = true;
	}

	void stage_tectonic_plates(PlanetGlobePipelineContext &ctx) {
		TectonicPlateAssigner assigner;
		assigner.assign(ctx.globe->topology, ctx.globe->region_neighbours, *ctx.world_seed, ctx.num_plate_regions, ctx.globe->plates);
	}

	void stage_plate_properties(PlanetGlobePipelineContext &ctx) {
		PlatePropertiesAssigner assigner;
		assigner.assign(ctx.globe->topology, ctx.globe->region_neighbours, ctx.globe->plates, *ctx.world_seed, ctx.globe->plate_properties);
	}

	void stage_elevation(PlanetGlobePipelineContext &ctx) {
		ElevationAssigner assigner;
		assigner.assign(ctx.globe->topology, ctx.globe->region_neighbours, ctx.globe->plates, ctx.globe->plate_properties, *ctx.world_seed, ctx.globe->region_elevation);
		smooth_region_elevation_laplacian(ctx.globe->region_neighbours, ctx.globe->region_elevation.region_elevation, 2);
	}

	void stage_moisture(PlanetGlobePipelineContext &ctx) {
		MoistureAssigner assigner;
		assigner.assign(ctx.globe->topology, ctx.globe->region_neighbours, ctx.globe->plates, ctx.globe->region_elevation, *ctx.world_seed,
		                ctx.temperature_01, ctx.precipitation_01, ctx.globe->region_moisture);
	}

	void stage_triangle_values(PlanetGlobePipelineContext &ctx) {
		assign_triangle_values_from_regions(ctx.globe->topology, ctx.globe->region_elevation, ctx.globe->region_moisture, ctx.globe->triangle_values);
	}

	void stage_priority_flood(PlanetGlobePipelineContext &ctx) {
		priority_flood_region_elevation(ctx.globe->region_neighbours, ctx.globe->region_elevation.region_elevation);
		assign_triangle_values_from_regions(
			ctx.globe->topology, ctx.globe->region_elevation, ctx.globe->region_moisture, ctx.globe->triangle_values);
	}

	void stage_river_downflow(PlanetGlobePipelineContext &ctx) {
		assign_downflow(ctx.globe->mesh, ctx.globe->triangle_values.triangle_elevation, ctx.globe->river_flow);
	}

	void stage_river_flow(PlanetGlobePipelineContext &ctx) {
		assign_flow(ctx.globe->mesh,
		            ctx.globe->river_flow,
		            ctx.globe->triangle_values.triangle_elevation,
		            ctx.globe->triangle_values.triangle_moisture,
		            ctx.globe->river_flow);
	}

	// Erosion edits triangle_elevation scalars only; region_elevation is re-synced and smoothed.
	// Mesh vertices are not moved here — stage_terrain_mesh derives positions from final scalars.
	void stage_erosion(PlanetGlobePipelineContext &ctx) {
		ctx.globe->region_elevation_pre_erosion.region_elevation = ctx.globe->region_elevation.region_elevation;
		apply_hydraulic_erosion(ctx.globe->mesh, ctx.globe->river_flow, ctx.globe->triangle_values.triangle_elevation);
		sync_region_elevation_from_triangles(ctx.globe->topology,
		                                     ctx.globe->triangle_values.triangle_elevation,
		                                     ctx.globe->region_elevation.region_elevation);
		smooth_region_elevation_laplacian(ctx.globe->region_neighbours, ctx.globe->region_elevation.region_elevation, 2);
		assign_triangle_values_from_regions(ctx.globe->topology,
		                                    ctx.globe->region_elevation,
		                                    ctx.globe->region_moisture,
		                                    ctx.globe->triangle_values);
	}

	void stage_river_carve(PlanetGlobePipelineContext &ctx) {
		carve_rivers_from_flow(*ctx.globe);
	}

	void stage_terrain_mesh(PlanetGlobePipelineContext &ctx) {
		if (ctx.planet_terrain_mesh) {
			// Single export pass: pos = normalise(site) * (1 + k * region_elevation). Connectivity fixed;
			// half-edge mesh is only for river/erosion graphs (see generate_planet_terrain_mesh_dual_folded).
			generate_planet_terrain_mesh_quad(*ctx.globe, *ctx.planet_terrain_mesh);
		}
	}

	const StageFn *find_stage(const String &stage_id) {
		static const UnorderedMap<String, StageFn> k_stages = {
			{ "topology", &stage_topology },
			{ "half_edge_mesh", &stage_half_edge_mesh },
			{ "region_neighbours", &stage_region_neighbours },
			{ "coarse_sim_upsample", &stage_coarse_sim_upsample },
			{ "tectonic_plates", &stage_tectonic_plates },
			{ "plate_properties", &stage_plate_properties },
			{ "elevation", &stage_elevation },
			{ "moisture", &stage_moisture },
			{ "triangle_values", &stage_triangle_values },
			{ "priority_flood", &stage_priority_flood },
			{ "river_downflow", &stage_river_downflow },
			{ "river_flow", &stage_river_flow },
			{ "erosion", &stage_erosion },
			{ "river_carve", &stage_river_carve },
			{ "terrain_mesh", &stage_terrain_mesh },
		};
		const auto it = k_stages.find(stage_id);
		return it == k_stages.end() ? nullptr : &it->second;
	}

	bool should_skip_stage(const String &stage_id, const PlanetGlobePipelineContext &ctx) {
		if (stage_id == "terrain_mesh" && ctx.planet_terrain_mesh == nullptr)
			return true;
		if (stage_id == "coarse_sim_upsample" && !ctx.use_coarse_sim_upsample())
			return true;
		const bool coarse_fields_from_upsample = ctx.use_coarse_sim_upsample() || ctx.coarse_sim_applied;
		if (coarse_fields_from_upsample) {
			if (stage_id == "tectonic_plates" || stage_id == "plate_properties" || stage_id == "elevation" || stage_id == "moisture")
				return true;
		}
		return false;
	}

	bool cancel_requested(const PlanetGlobePipelineContext &ctx) {
		return ctx.progress != nullptr && ctx.progress->cancel_flag != nullptr && ctx.progress->cancel_flag->load();
	}

	void notify_stage_begin(PlanetGlobePipelineContext &ctx, const char *stage_id, int index, int count) {
		if (ctx.progress == nullptr || ctx.progress->on_stage_begin == nullptr)
			return;
		ctx.progress->on_stage_begin(ctx.progress->userdata, stage_id, index, count);
	}

	void run_stage(const char *stage_id, StageFn fn, PlanetGlobePipelineContext &ctx) {
		if (ctx.perf) {
			auto timer = ctx.perf->scope(stage_id);
			fn(ctx);
		} else {
			fn(ctx);
		}
	}

	int count_stages(const DefDatabase &defs, const String &pipeline_id, const PlanetGlobePipelineContext &ctx) {
		const WorldGenPipelineDef *pipeline = defs.world_gen_pipeline(pipeline_id);
		auto count_one = [&](const String &id, bool optional) {
			if (should_skip_stage(id, ctx))
				return 0;
			const StageFn *fn = find_stage(id);
			if (fn == nullptr || *fn == nullptr)
				return optional ? 0 : 0;
			return 1;
		};

		if (!pipeline || pipeline->stages.empty()) {
			static const char *k_default[] = {
				"topology", "half_edge_mesh", "region_neighbours", "coarse_sim_upsample",
				"tectonic_plates", "plate_properties", "elevation", "moisture", "triangle_values",
				"priority_flood", "river_downflow", "river_flow", "erosion", "river_carve", "terrain_mesh",
			};
			int n = 0;
			for (const char *id : k_default)
				n += count_one(id, id == String("terrain_mesh") || id == String("coarse_sim_upsample"));
			return n > 0 ? n : 1;
		}

		int n = 0;
		for (const auto &stage : pipeline->stages)
			n += count_one(stage.stage_id, stage.optional);
		return n > 0 ? n : 1;
	}

	bool run_stage_list(const Vector<Pair<String, StageFn>> &stages, PlanetGlobePipelineContext &ctx) {
		const int total = static_cast<int>(stages.size());
		int index       = 0;
		for (const auto &entry : stages) {
			if (cancel_requested(ctx))
				return false;
			notify_stage_begin(ctx, entry.first.c_str(), index, total);
			run_stage(entry.first.c_str(), entry.second, ctx);
			++index;
		}
		return true;
	}

	Vector<Pair<String, StageFn>> build_stage_list(const DefDatabase &defs, const String &pipeline_id, const PlanetGlobePipelineContext &ctx) {
		Vector<Pair<String, StageFn>> out;
		const WorldGenPipelineDef *pipeline = defs.world_gen_pipeline(pipeline_id);

		auto try_add = [&](const String &id, bool optional) {
			if (should_skip_stage(id, ctx))
				return;
			const StageFn *fn = find_stage(id);
			if (fn == nullptr || *fn == nullptr) {
				if (!optional)
					return;
				return;
			}
			out.emplace_back(id, *fn);
		};

		if (!pipeline || pipeline->stages.empty()) {
			static const char *k_default[] = {
				"topology", "half_edge_mesh", "region_neighbours", "coarse_sim_upsample",
				"tectonic_plates", "plate_properties", "elevation", "moisture", "triangle_values",
				"priority_flood", "river_downflow", "river_flow", "erosion", "river_carve", "terrain_mesh",
			};
			for (const char *id : k_default)
				try_add(id, id == String("terrain_mesh") || id == String("coarse_sim_upsample"));
			return out;
		}

		for (const auto &stage : pipeline->stages)
			try_add(stage.stage_id, stage.optional);
		return out;
	}

} // namespace

bool PlanetGlobePipeline::run(const DefDatabase &defs, const String &pipeline_id, PlanetGlobePipelineContext &ctx) const {
	(void)ctx.planet_genome;
	const Vector<Pair<String, StageFn>> stages = build_stage_list(defs, pipeline_id, ctx);
	if (stages.empty())
		return true;
	return run_stage_list(stages, ctx);
}

} // namespace growth
