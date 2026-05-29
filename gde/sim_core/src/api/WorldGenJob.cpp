#include "api/WorldGenJob.hpp"
#include "api/WorldGenRunner.hpp"
#include "gen/Planet.hpp"
#include "world/PlanetGlobeGenerator.hpp"
#include "world/PlanetGlobePipeline.hpp"
#include "world/PlanetTerrainMesh.hpp"
#include "world/RegionElevationSmooth.hpp"
#include "world/TriangleValues.hpp"
#include "util/Parallel.hpp"
#include "base/gateway/Cstring.hpp"
#include "base/gateway/Cutility.hpp"

namespace growth {

namespace {

bool preset_uses_floating_islands(const ParsedWorldGenForm &form) {
	return form.planet_preset == "FloatingIslands";
}

void clamp01(double &v) {
	if (v < 0.0)
		v = 0.0;
	if (v > 1.0)
		v = 1.0;
}

} // namespace

struct WorldGenJob::SharedState {
	WorldGenJob *job = nullptr;
	ParsedWorldGenForm parsed;
	const DefDatabase *defs = nullptr;
	String pipeline_id;
	WorldGenJobResult result;
};

WorldGenJob::~WorldGenJob() {
	if (worker_.joinable())
		worker_.join();
	delete shared_;
	shared_ = nullptr;
}

bool WorldGenJob::has_active_job() const {
	return running_.load() || (done_.load() && result_ready_);
}

bool WorldGenJob::start(const ParsedWorldGenForm &parsed, const DefDatabase &defs, const String &pipeline_id) {
	if (running_.load() || (done_.load() && result_ready_))
		return false;

	if (worker_.joinable())
		worker_.join();

	delete shared_;
	shared_ = new SharedState();
	shared_->job         = this;
	shared_->parsed       = parsed;
	shared_->defs        = &defs;
	shared_->pipeline_id = pipeline_id.empty() ? "default_globe" : pipeline_id;

	result_ready_ = false;
	result_         = WorldGenJobResult{};
	cancel_requested_.store(false);
	cancelled_.store(false);
	failed_.store(false);
	done_.store(false);
	running_.store(true);
	stage_index_.store(0);
	stage_count_.store(1);
	{
		Cmutex::LockGuard lock(status_mutex_);
		current_stage_id_ = "parse_form";
		error_message_.clear();
	}

	WorldGenJob *const self = this;
	SharedState *const job_state = shared_;
	worker_                      = Cthread::Thread([self, job_state]() { self->worker_main(job_state); });
	return true;
}

void WorldGenJob::request_cancel() {
	cancel_requested_.store(true);
}

void WorldGenJob::on_pipeline_stage(void *userdata, const char *stage_id, int stage_index, int stage_count) {
	auto *state = static_cast<SharedState *>(userdata);
	if (state == nullptr || state->job == nullptr)
		return;
	WorldGenJob *job = state->job;
	job->stage_index_.store(stage_index);
	job->stage_count_.store(stage_count > 0 ? stage_count : 1);
	{
		Cmutex::LockGuard lock(job->status_mutex_);
		job->current_stage_id_ = stage_id != nullptr ? stage_id : "";
	}
}

WorldGenJobStatus WorldGenJob::poll_status() const {
	WorldGenJobStatus s;
	s.running   = running_.load();
	s.done      = done_.load();
	s.cancelled = cancelled_.load();
	s.failed    = failed_.load();
	s.stage_index = stage_index_.load();
	s.stage_count = stage_count_.load();
	if (s.stage_count < 1)
		s.stage_count = 1;
	const int idx = s.stage_index;
	s.progress_01 = (static_cast<double>(idx) + 0.5) / static_cast<double>(s.stage_count);
	if (s.progress_01 > 1.0)
		s.progress_01 = 1.0;
	if (s.progress_01 < 0.0)
		s.progress_01 = 0.0;
	{
		Cmutex::LockGuard lock(status_mutex_);
		s.stage_id       = current_stage_id_;
		s.error_message  = error_message_;
		s.status_text    = current_stage_id_;
	}
	if (s.cancelled)
		s.status_text = "Cancelled";
	else if (s.failed)
		s.status_text = "Failed";
	else if (s.running && !s.stage_id.empty())
		s.status_text = s.stage_id;
	return s;
}

bool WorldGenJob::take_result(WorldGenJobResult &out) {
	if (worker_.joinable())
		worker_.join();
	running_.store(false);
	if (cancelled_.load()) {
		done_.store(false);
		cancelled_.store(false);
		cancel_requested_.store(false);
		return false;
	}
	if (failed_.load()) {
		done_.store(false);
		failed_.store(false);
		return false;
	}
	if (!result_ready_)
		return false;
	out           = Cutility::move(result_);
	result_ready_ = false;
	result_       = WorldGenJobResult{};
	done_.store(false);
	return out.ok;
}

void WorldGenJob::worker_main(SharedState *state) {
	auto finish_cancelled = [&]() {
		cancelled_.store(true);
		done_.store(true);
		running_.store(false);
	};

	auto finish_failed = [&](const String &msg) {
		{
			Cmutex::LockGuard lock(status_mutex_);
			error_message_ = msg;
		}
		failed_.store(true);
		done_.store(true);
		running_.store(false);
	};

	auto log_line = [&](WorldGenJobResult &r, const String &line) {
		r.log_lines.push_back(line);
	};

	if (state == nullptr) {
		finish_failed("Internal error: missing job state");
		return;
	}

	WorldGenJobResult r;
	r.parsed = state->parsed;
	r.ok     = false;

	log_line(r, "[WorldGen] Generating seed and planet genome...");
	{
		Cmutex::LockGuard lock(status_mutex_);
		current_stage_id_ = "world_seed_and_genome";
	}
	WorldGenRunner runner;
	runner.run(r.parsed, r.world_seed, r.planet_genome);

	log_line(r, "[PlanetGen] preset=" + r.parsed.planet_preset + " seed=" + Cstring::to_string(r.world_seed.value));
	log_line(r, "[PlanetGen] temperature=" + Cstring::to_string(r.parsed.temperature) + "% precipitation="
	              + Cstring::to_string(r.parsed.precipitation) + "%");

	Planet p(r.planet_genome);
	log_line(r, "[Planet] S_eff=" + Cstring::to_string(r.planet_genome.S_eff) + " T_surf_K="
	              + Cstring::to_string(p.T_surf_estimate()) + " water=" + Cstring::to_string(r.planet_genome.water_fraction));

	log_line(r, "[PlanetGlobe] Generating icosphere topology (target_regions=" + Cstring::to_string(r.parsed.voronoi_sites)
	              + " jitter=" + Cstring::to_string(r.parsed.jitter) + "%)...");

	double temperature_01   = r.parsed.temperature / 200.0;
	double precipitation_01 = r.parsed.precipitation / 200.0;
	clamp01(temperature_01);
	clamp01(precipitation_01);

	const bool floating_islands = preset_uses_floating_islands(r.parsed);
	const bool use_terrain_mesh = r.parsed.use_planet_terrain_mesh || floating_islands;
	parallel::RuntimeConfig parallel_cfg;
	parallel_cfg.profile = r.parsed.world_gen_mode == "max_determinism"
		? parallel::ExecutionProfile::MaxDeterminism
		: parallel::ExecutionProfile::MaxPerformance;
	parallel_cfg.enable_simd = r.parsed.use_simd;
	parallel_cfg.worker_count_override = parallel_cfg.profile == parallel::ExecutionProfile::MaxDeterminism ? 1u : 0u;
	parallel::configure(parallel_cfg);

	PlanetGlobePipelineProgress pipeline_progress;
	pipeline_progress.cancel_flag    = &cancel_requested_;
	pipeline_progress.on_stage_begin = &WorldGenJob::on_pipeline_stage;
	pipeline_progress.userdata       = state;

	PlanetGlobePipelineContext ctx;
	ctx.world_seed              = &r.world_seed;
	ctx.planet_genome           = &r.planet_genome;
	ctx.num_sites               = r.parsed.voronoi_sites;
	ctx.sim_num_sites           = r.parsed.sim_voronoi_sites;
	ctx.jitter_percent          = r.parsed.jitter;
	ctx.num_plate_regions       = r.parsed.num_plate_regions;
	ctx.temperature_01          = temperature_01;
	ctx.precipitation_01        = precipitation_01;
	ctx.globe                   = &r.globe;
	ctx.use_planet_terrain_mesh = use_terrain_mesh;
	ctx.planet_terrain_mesh     = use_terrain_mesh ? &r.planet_terrain_mesh : nullptr;
	ctx.perf                    = &r.perf;
	ctx.progress                = &pipeline_progress;

	PlanetGlobePipeline pipeline;
	if (state->defs == nullptr) {
		finish_failed("DefDatabase not available");
		return;
	}
	const bool completed = pipeline.run(*state->defs, state->pipeline_id, ctx);
	if (!completed || cancel_requested_.load()) {
		finish_cancelled();
		return;
	}

	log_line(r, "[PlanetGlobe] Topology: regions=" + Cstring::to_string(r.globe.topology.sites.size())
	              + " triangles=" + Cstring::to_string(r.globe.topology.triangles.size()));

	// Pipeline stage terrain_mesh fills the mesh when requested; only post-process when still empty or floating islands needs a different generator.
	if (floating_islands) {
		Cmutex::LockGuard lock(status_mutex_);
		current_stage_id_ = "terrain_mesh_post";
		r.planet_terrain_mesh.vertices.clear();
		r.planet_terrain_mesh.normals.clear();
		r.planet_terrain_mesh.indices.clear();
		r.planet_terrain_mesh.river_strength.clear();
		generate_planet_terrain_mesh_floating_islands(r.globe, r.planet_terrain_mesh, &r.planet_terrain_triangle_region);
	} else if (use_terrain_mesh && r.planet_terrain_mesh.vertices.empty()) {
		generate_planet_terrain_mesh_quad(r.globe, r.planet_terrain_mesh);
	}

	if (use_terrain_mesh && !floating_islands && !r.globe.region_elevation_pre_erosion.region_elevation.empty()) {
		PlanetGlobe pre_globe;
		pre_globe.topology                = r.globe.topology;
		pre_globe.mesh                    = r.globe.mesh;
		pre_globe.region_neighbours       = r.globe.region_neighbours;
		pre_globe.region_triangle_rings   = r.globe.region_triangle_rings;
		pre_globe.region_elevation        = r.globe.region_elevation_pre_erosion;
		pre_globe.region_moisture         = r.globe.region_moisture;
		pre_globe.river_flow              = r.globe.river_flow;
		generate_planet_terrain_mesh_quad(pre_globe, r.planet_terrain_mesh_pre_erosion);
	}

	if (use_terrain_mesh) {
		const size_t num_regions = r.globe.topology.sites.size();
		const size_t num_tri     = r.globe.topology.triangles.size();
		const size_t mesh_tris   = r.planet_terrain_mesh.indices.size() / 3;
		log_line(r, "[PlanetGlobe] Terrain mesh: preset=" + r.parsed.planet_preset + " requested="
		                  + (r.parsed.use_planet_terrain_mesh ? "true" : "false") + " icosphere_regions="
		                  + Cstring::to_string(num_regions) + " triangles=" + Cstring::to_string(num_tri) + " verts="
		                  + Cstring::to_string(r.planet_terrain_mesh.vertices.size()) + " indices="
		                  + Cstring::to_string(r.planet_terrain_mesh.indices.size()) + " mesh_tris="
		                  + Cstring::to_string(mesh_tris) + " style="
		                  + (floating_islands ? String("floating_islands") : String("icosphere_quad")));
	}

	log_line(r, "[WorldGen] Done.");
	r.ok = true;

	result_       = Cutility::move(r);
	result_ready_ = true;
	done_.store(true);
	running_.store(false);
}

} // namespace growth
