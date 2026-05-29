#include "sim_api_gdextension.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "bridge/SimBridge.hpp"
#include "bridge/Diff.hpp"
#include "world/WorldCoord.hpp"
#include "api/WorldGenJob.hpp"
#include "world/PlanetSurfaceAtlasBuilder.hpp"
#include "WorldGenFormParser.hpp"
#include "WorldGenMarshal.hpp"
#include "DiffConverter.hpp"
#include "IntentHandler.hpp"
#include "base/gateway/Cmemory.hpp"
#include "base/gateway/Cutility.hpp"
#include "base/gateway/Csstream.hpp"
#include "base/gateway/Cstring.hpp"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot {

namespace {
	void log_line(growth::Vector<growth::String> &lines, const growth::String &s) {
		UtilityFunctions::print(godot::String(s.c_str()));
		lines.push_back(s);
	}
} // namespace

struct GrowthSim::BridgeHolder {
	growth::SimBridge bridge;
};

void GrowthSim::_bind_methods() {
	ClassDB::bind_method(D_METHOD("boot", "defdb_path"), &GrowthSim::boot);
	ClassDB::bind_method(D_METHOD("apply_world_gen_form", "form_dict"), &GrowthSim::apply_world_gen_form);
	ClassDB::bind_method(D_METHOD("get_last_world_gen_triangles"), &GrowthSim::get_last_world_gen_triangles);
	ClassDB::bind_method(D_METHOD("step", "dt", "speed"), &GrowthSim::step);
	ClassDB::bind_method(D_METHOD("set_speed", "mode"), &GrowthSim::set_speed);
	ClassDB::bind_method(D_METHOD("request_chunks", "center_chunk", "radius"), &GrowthSim::request_chunks);
	ClassDB::bind_method(D_METHOD("poll_diffs", "max_count"), &GrowthSim::poll_diffs);
	ClassDB::bind_method(D_METHOD("apply_intent", "intent_dict"), &GrowthSim::apply_intent);
	ClassDB::bind_method(D_METHOD("get_ui_asset_path", "asset_id"), &GrowthSim::get_ui_asset_path);
	ClassDB::bind_method(D_METHOD("has_overworld"), &GrowthSim::has_overworld);
	ClassDB::bind_method(D_METHOD("commit_overworld_for_play"), &GrowthSim::commit_overworld_for_play);
	ClassDB::bind_method(D_METHOD("sample_surface", "unit_dir"), &GrowthSim::sample_surface);
}

void GrowthSim::boot(const String &defdb_path) {
	if (bridge_) return;
	bridge_ = new BridgeHolder();
	growth::String path(defdb_path.utf8().get_data());
	bridge_->bridge.boot(growth::WorldSeed{}, path);
}

Dictionary GrowthSim::apply_world_gen_form(const Dictionary &form_dict) {
	Dictionary out;
	growth::Vector<growth::String> log_lines;
	if (!bridge_) {
		log_line(log_lines, "[WorldGen] No bridge; skipping.");
		Array log_arr;
		for (const auto &s : log_lines)
			log_arr.push_back(String(s.c_str()));
		out["log_lines"] = log_arr;
		return out;
	}

	log_line(log_lines, "[WorldGen] Parsing form...");
	growth::ParsedWorldGenForm parsed;
	growth_sim::parse_world_gen_form(form_dict, parsed);

	const growth::String pipeline_id = parsed.world_gen_mode == "max_determinism" ? "deterministic_globe" : "default_globe";

	growth::WorldGenJob job;
	if (!job.start(parsed, bridge_->bridge.defs(), pipeline_id)) {
		log_line(log_lines, "[WorldGen] Job already running.");
		Array log_arr;
		for (const auto &s : log_lines)
			log_arr.push_back(String(s.c_str()));
		out["log_lines"] = log_arr;
		return out;
	}

	growth::WorldGenJobResult result;
	if (!job.take_result(result)) {
		log_line(log_lines, "[WorldGen] Generation failed or was cancelled.");
		Array log_arr;
		for (const auto &s : log_lines)
			log_arr.push_back(String(s.c_str()));
		out["log_lines"] = log_arr;
		return out;
	}

	for (const growth::String &line : result.log_lines)
		log_line(log_lines, line);

	if (result.ok) {
		bridge_->bridge.set_world_gen(result.world_seed, result.planet_genome);
		growth::PlanetSurfaceAtlas atlas;
		growth::PlanetSurfaceAtlasBuilder::build_from_globe(result.globe, atlas);
		bridge_->bridge.set_overworld_surface(growth::Cutility::move(atlas));
		::growth_sim::marshal_world_gen_job_result(result, out, _last_world_gen_triangles);
	}

	Array log_arr;
	for (const auto &s : log_lines)
		log_arr.push_back(String(s.c_str()));
	out["log_lines"] = log_arr;
	return out;
}

PackedInt32Array GrowthSim::get_last_world_gen_triangles() const {
	return _last_world_gen_triangles;
}

GrowthSim::GrowthSim() {}

GrowthSim::~GrowthSim() {
	delete bridge_;
	bridge_ = nullptr;
}

void GrowthSim::step(double dt, double /* speed */) {
	if (bridge_) bridge_->bridge.step(dt);
}

void GrowthSim::set_speed(int /* mode */) {
	if (bridge_) { /* optional */ }
}

void GrowthSim::request_chunks(Vector2i center_chunk, int radius) {
	if (!bridge_) return;
	growth::ChunkCoord c{ center_chunk.x, center_chunk.y };
	bridge_->bridge.request_chunks(c, radius);
}

Array GrowthSim::poll_diffs(int max_count) {
	Array out;
	if (!bridge_) return out;
	growth::Diff d;
	int n = 0;
	while (n < max_count && bridge_->bridge.poll_diff(d)) {
		out.push_back(growth_sim::diff_to_dictionary(d));
		++n;
	}
	return out;
}

void GrowthSim::apply_intent(const Dictionary &intent_dict) {
	growth_sim::apply_intent(bridge_ ? &bridge_->bridge : nullptr, intent_dict);
}

String GrowthSim::get_ui_asset_path(const String &p_asset_id) {
	const growth::String id(p_asset_id.utf8().get_data());
	if (!bridge_) return String();
	const growth::String path = bridge_->bridge.defs().ui_asset_path(id);
	return path.empty() ? String() : String(path.c_str());
}

bool GrowthSim::has_overworld() const {
	return bridge_ && bridge_->bridge.has_overworld();
}

void GrowthSim::commit_overworld_for_play() {
	if (bridge_)
		bridge_->bridge.commit_overworld_for_play();
}

Dictionary GrowthSim::sample_surface(Vector3 unit_dir) {
	Dictionary out;
	if (!bridge_ || !bridge_->bridge.has_overworld())
		return out;
	const growth::Vec3 dir(
		static_cast<float>(unit_dir.x),
		static_cast<float>(unit_dir.y),
		static_cast<float>(unit_dir.z));
	const growth::SurfaceSample s = bridge_->bridge.sample_surface(dir);
	out["region_id"] = static_cast<int>(s.region_id);
	out["plate_id"] = static_cast<int>(s.plate_id);
	out["elevation"] = s.elevation;
	out["moisture"] = s.moisture;
	out["temperature"] = s.temperature;
	out["biome_id"] = static_cast<int>(s.biome_id);
	return out;
}

} // namespace godot
