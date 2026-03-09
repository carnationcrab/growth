#include "sim_api_gdextension.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "bridge/SimBridge.hpp"
#include "bridge/Diff.hpp"
#include "world/WorldCoord.hpp"
#include "gen/WorldSeed.hpp"
#include "gen/Planet.hpp"
#include "world/PlanetGlobeGenerator.hpp"
#include "world/VoronoiSphere.hpp"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include "api/WorldGenRunner.hpp"
#include "WorldGenFormParser.hpp"
#include "UiAssetLoader.hpp"
#include "DiffConverter.hpp"
#include "IntentHandler.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace godot {

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
}

void GrowthSim::boot(const String &defdb_path) {
	if (bridge_) return;
	bridge_ = new BridgeHolder();
	std::string path(defdb_path.utf8().get_data());
	bridge_->bridge.boot(growth::WorldSeed{}, path);
	_ui_asset_paths = growth_sim::load_ui_assets(defdb_path);
}

// World-gen form submit pipeline: parse form -> WorldGenRunner -> set_world_gen -> globe gen -> return Voronoi sites + Delaunay triangles.
Dictionary GrowthSim::apply_world_gen_form(const Dictionary &form_dict) {
	Dictionary out;
	if (!bridge_) return out;
	growth::ParsedWorldGenForm parsed;
	growth_sim::parse_world_gen_form(form_dict, parsed);
	growth::WorldSeed world_seed;
	growth::PlanetGenome planet_genome;
	growth::WorldGenRunner runner;
	runner.run(parsed, world_seed, planet_genome);

	UtilityFunctions::print("[PlanetGen] generating preset=", String(parsed.planet_preset.c_str()), " seed=", (int64_t)world_seed.value,
		" temperature=", parsed.temperature, "% precipitation=", parsed.precipitation, "%");
	bridge_->bridge.set_world_gen(world_seed, planet_genome);

	growth::Planet p(planet_genome);
	UtilityFunctions::print("[Planet] seed=", (int64_t)planet_genome.seed, " S_eff=", planet_genome.S_eff, " g=", p.g(), " P_orb_days=", p.P_orb() / 86400.0,
		" T_surf_K=", p.T_surf_estimate(), " precip=", planet_genome.precipitation, " water=", planet_genome.water_fraction,
		" scale_height_m=", p.scale_height());

	UtilityFunctions::print("[PlanetGlobe] began processing (sites=", (int64_t)parsed.voronoi_sites, ")");
	growth::VoronoiSphere voronoi_sphere;
	growth::PlanetGlobeGenerator globe_gen;
	globe_gen.run(world_seed, planet_genome, voronoi_sphere, parsed.voronoi_sites);

	// Sites in sim coords (Z-up); SpherePreview converts to Godot Y-up for display.
	const auto &sites = voronoi_sphere.sites;
	PackedVector3Array sites_arr;
	sites_arr.resize(static_cast<int64_t>(sites.size()));
	for (size_t i = 0; i < sites.size(); ++i)
		sites_arr.set(static_cast<int64_t>(i), Vector3(sites[i].x, sites[i].y, sites[i].z));
	out["sites"] = sites_arr;

	const size_t num_tri = voronoi_sphere.triangles.size();
	const int64_t flat_size = static_cast<int64_t>(num_tri * 3);
	// Store in PackedInt32Array for get_last_world_gen_triangles() direct return (marshals to C# as int[])
	_last_world_gen_triangles.resize(flat_size);
	for (size_t i = 0; i < num_tri; ++i) {
		const auto &t = voronoi_sphere.triangles[i];
		_last_world_gen_triangles.set(static_cast<int64_t>(i * 3 + 0), static_cast<int32_t>(t[0]));
		_last_world_gen_triangles.set(static_cast<int64_t>(i * 3 + 1), static_cast<int32_t>(t[1]));
		_last_world_gen_triangles.set(static_cast<int64_t>(i * 3 + 2), static_cast<int32_t>(t[2]));
	}
	UtilityFunctions::print("[WorldGen] C++ triangles: ", (int64_t)num_tri, " (flat size ", flat_size, ")");
	// Also put Array in dict for GDScript consumers that read from result
	Array tri_arr;
	tri_arr.resize(flat_size);
	for (int64_t i = 0; i < flat_size; ++i)
		tri_arr[i] = _last_world_gen_triangles[i];
	out["triangles"] = tri_arr;

	// Voronoi vertices (circumcenters on unit sphere)
	const auto &circumcenters = voronoi_sphere.circumcenters;
	PackedVector3Array circumcenters_arr;
	circumcenters_arr.resize(static_cast<int64_t>(circumcenters.size()));
	for (size_t i = 0; i < circumcenters.size(); ++i)
		circumcenters_arr.set(static_cast<int64_t>(i), Vector3(circumcenters[i].x, circumcenters[i].y, circumcenters[i].z));
	out["circumcenters"] = circumcenters_arr;

	// Per-site cells: each cell is an array of circumcenter indices
	Array cells_arr;
	cells_arr.resize(static_cast<int64_t>(voronoi_sphere.cells.size()));
	for (size_t i = 0; i < voronoi_sphere.cells.size(); ++i) {
		PackedInt32Array cell;
		const auto &c = voronoi_sphere.cells[i];
		cell.resize(static_cast<int64_t>(c.size()));
		for (size_t j = 0; j < c.size(); ++j)
			cell.set(static_cast<int64_t>(j), static_cast<int32_t>(c[j]));
		cells_arr[i] = cell;
	}
	out["cells"] = cells_arr;

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
	auto it = _ui_asset_paths.find(p_asset_id.utf8().get_data());
	if (it == _ui_asset_paths.end())
		return String();
	return String(it->second.c_str());
}

} // namespace godot
