#pragma once

#include "api/ParsedWorldGenForm.hpp"
#include "data/DefDatabase.hpp"
#include "gen/PlanetGenome.hpp"
#include "gen/WorldSeed.hpp"
#include "util/PerformanceLogger.hpp"
#include "world/PlanetGlobe.hpp"
#include "world/PlanetTerrainMesh.hpp"
#include "base/gateway/Catomic.hpp"
#include "base/gateway/Cmutex.hpp"
#include "base/gateway/Cthread.hpp"
#include "base/gateway/Cstring.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

/// Snapshot for UI polling (main thread only).
struct WorldGenJobStatus {
	bool running     = false;
	bool done        = false;
	bool cancelled   = false;
	bool failed      = false;
	int stage_index  = 0;
	int stage_count  = 1;
	double progress_01 = 0.0;
	String stage_id;
	String status_text;
	String error_message;
};

/// Completed globe generation (moved out on take_result).
struct WorldGenJobResult {
	bool ok = false;
	ParsedWorldGenForm parsed;
	WorldSeed world_seed;
	PlanetGenome planet_genome;
	PlanetGlobe globe;
	PlanetTerrainMesh planet_terrain_mesh;
	PlanetTerrainMesh planet_terrain_mesh_pre_erosion;
	Vector<U32> planet_terrain_triangle_region;
	perf::PerformanceLogger perf;
	Vector<String> log_lines;
};

/// Background planet globe generation (sim_core only; no Godot types).
class WorldGenJob {
public:
	WorldGenJob()  = default;
	~WorldGenJob();

	WorldGenJob(const WorldGenJob &)            = delete;
	WorldGenJob &operator=(const WorldGenJob &) = delete;

	/// Starts worker if idle. Returns false if a job is already active.
	bool start(const ParsedWorldGenForm &parsed, const DefDatabase &defs, const String &pipeline_id);

	void request_cancel();

	WorldGenJobStatus poll_status() const;

	/// Joins worker if still running; moves result on success. Clears job state.
	bool take_result(WorldGenJobResult &out);

	bool has_active_job() const;

private:
	struct SharedState;

	void worker_main(SharedState *state);
	static void on_pipeline_stage(void *userdata, const char *stage_id, int stage_index, int stage_count);

	Catomic::Atomic<bool> running_{ false };
	Catomic::Atomic<bool> done_{ false };
	Catomic::Atomic<bool> cancelled_{ false };
	Catomic::Atomic<bool> failed_{ false };
	Catomic::Atomic<bool> cancel_requested_{ false };
	Catomic::Atomic<int> stage_index_{ 0 };
	Catomic::Atomic<int> stage_count_{ 1 };

	mutable Cmutex::Mutex status_mutex_;
	String current_stage_id_;
	String error_message_;

	Cthread::Thread worker_;
	SharedState *shared_ = nullptr;
	bool result_ready_   = false;
	WorldGenJobResult result_;
};

} // namespace growth
