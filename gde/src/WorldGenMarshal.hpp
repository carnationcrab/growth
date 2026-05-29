#pragma once

#include "api/WorldGenJob.hpp"
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>

namespace growth_sim {

/// Fills Godot dictionary keys for world-gen preview (main thread only).
/// Key strings must match godot/autoload/sim/WorldGenPreviewKeys.cs and WorldGenPreviewKeys.gd.
void marshal_world_gen_job_result(
	const growth::WorldGenJobResult &job_result,
	godot::Dictionary &out,
	godot::PackedInt32Array &last_world_gen_triangles_out);

} // namespace growth_sim
