#include "sim_api_gdextension.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "bridge/SimBridge.hpp"
#include "bridge/Diff.hpp"
#include "world/WorldCoord.hpp"
#include "gen/WorldSeed.hpp"
#include "gen/Planet.hpp"
#include "world/PlanetGlobeGenerator.hpp"
#include "world/PlanetGlobe.hpp"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include "api/WorldGenRunner.hpp"
#include "WorldGenFormParser.hpp"
#include "UiAssetLoader.hpp"
#include "DiffConverter.hpp"
#include "IntentHandler.hpp"
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

namespace godot {

namespace {
	void log_line(std::vector<std::string> &lines, const std::string &s) {
		UtilityFunctions::print(String(s.c_str()));
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
	std::vector<std::string> log_lines;
	if (!bridge_) {
		log_line(log_lines, "[WorldGen] No bridge; skipping.");
		Array log_arr;
		for (const auto &s : log_lines) log_arr.push_back(String(s.c_str()));
		out["log_lines"] = log_arr;
		return out;
	}
	log_line(log_lines, "[WorldGen] Parsing form...");
	growth::ParsedWorldGenForm parsed;
	growth_sim::parse_world_gen_form(form_dict, parsed);
	log_line(log_lines, "[WorldGen] Generating seed and planet genome...");
	growth::WorldSeed world_seed;
	growth::PlanetGenome planet_genome;
	growth::WorldGenRunner runner;
	runner.run(parsed, world_seed, planet_genome);
	{
		std::ostringstream os;
		os << "[PlanetGen] preset=" << parsed.planet_preset << " seed=" << world_seed.value;
		log_line(log_lines, os.str());
	}
	{
		std::ostringstream os;
		os << "[PlanetGen] temperature=" << parsed.temperature << "% precipitation=" << parsed.precipitation << "%";
		log_line(log_lines, os.str());
	}
	bridge_->bridge.set_world_gen(world_seed, planet_genome);

	growth::Planet p(planet_genome);
	{
		std::ostringstream os;
		os << "[Planet] S_eff=" << planet_genome.S_eff << " T_surf_K=" << p.T_surf_estimate() << " water=" << planet_genome.water_fraction;
		log_line(log_lines, os.str());
	}

	{
		std::ostringstream os;
		os << "[PlanetGlobe] Generating Voronoi sphere (sites=" << parsed.voronoi_sites << " jitter=" << parsed.jitter << "%)...";
		log_line(log_lines, os.str());
	}
	float temperature_01 = static_cast<float>(parsed.temperature / 200.0);
	float precipitation_01 = static_cast<float>(parsed.precipitation / 200.0);
	if (temperature_01 < 0.0f) temperature_01 = 0.0f;
	if (temperature_01 > 1.0f) temperature_01 = 1.0f;
	if (precipitation_01 < 0.0f) precipitation_01 = 0.0f;
	if (precipitation_01 > 1.0f) precipitation_01 = 1.0f;
	growth::PlanetGlobe globe;
	growth::PlanetGlobeGenerator globe_gen;
	globe_gen.run(world_seed, planet_genome, parsed.voronoi_sites, parsed.jitter, parsed.num_plate_regions, temperature_01, precipitation_01, globe);
	log_line(log_lines, "[PlanetGlobe] Voronoi done. Assigning tectonic plates...");
	log_line(log_lines, "[PlanetGlobe] Plates done. Computing elevation...");
	log_line(log_lines, "[PlanetGlobe] Elevation done. Computing moisture...");
	log_line(log_lines, "[PlanetGlobe] Moisture done. Marshalling output...");

	const growth::VoronoiSphere &voronoi_sphere = globe.voronoi;
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
	{
		std::ostringstream os;
		os << "[WorldGen] Triangles: " << num_tri << " (flat size " << flat_size << ")";
		log_line(log_lines, os.str());
	}
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

	// Tectonic plates (from globe pipeline)
	const growth::TectonicPlates &tectonic_plates = globe.plates;
	PackedInt32Array plate_regions_arr;
	plate_regions_arr.resize(static_cast<int64_t>(tectonic_plates.region_to_plate.size()));
	for (size_t i = 0; i < tectonic_plates.region_to_plate.size(); ++i)
		plate_regions_arr.set(static_cast<int64_t>(i), tectonic_plates.region_to_plate[i]);
	out["plate_regions"] = plate_regions_arr;
	{
		std::ostringstream os;
		os << "[WorldGen] Plates: " << tectonic_plates.num_plates << " regions=" << plate_regions_arr.size();
		log_line(log_lines, os.str());
	}

	// Per-region elevation and moisture (primary terrain data)
	const std::vector<float> &region_elevation = globe.region_elevation.region_elevation;
	const std::vector<float> &region_moisture = globe.region_moisture.region_moisture;
	PackedFloat32Array region_elev_arr;
	region_elev_arr.resize(static_cast<int64_t>(region_elevation.size()));
	for (size_t i = 0; i < region_elevation.size(); ++i)
		region_elev_arr.set(static_cast<int64_t>(i), region_elevation[i]);
	out["region_elevation"] = region_elev_arr;
	PackedFloat32Array region_moist_arr;
	region_moist_arr.resize(static_cast<int64_t>(region_moisture.size()));
	for (size_t i = 0; i < region_moisture.size(); ++i)
		region_moist_arr.set(static_cast<int64_t>(i), region_moisture[i]);
	out["region_moisture"] = region_moist_arr;

	// Per-triangle elevation and moisture (average of three regions)
	const std::vector<float> &triangle_elevation = globe.triangle_values.triangle_elevation;
	const std::vector<float> &triangle_moisture = globe.triangle_values.triangle_moisture;
	PackedFloat32Array triangle_elev_arr;
	triangle_elev_arr.resize(static_cast<int64_t>(triangle_elevation.size()));
	for (size_t i = 0; i < triangle_elevation.size(); ++i)
		triangle_elev_arr.set(static_cast<int64_t>(i), triangle_elevation[i]);
	out["triangle_elevation"] = triangle_elev_arr;
	PackedFloat32Array triangle_moist_arr;
	triangle_moist_arr.resize(static_cast<int64_t>(triangle_moisture.size()));
	for (size_t i = 0; i < triangle_moisture.size(); ++i)
		triangle_moist_arr.set(static_cast<int64_t>(i), triangle_moisture[i]);
	out["triangle_moisture"] = triangle_moist_arr;

	// Per-plate elevation/moisture (averaged from regions, for plate labels / backward compat)
	const size_t num_plates = tectonic_plates.num_plates;
	PackedFloat32Array plate_elev_arr;
	PackedFloat32Array plate_moist_arr;
	plate_elev_arr.resize(static_cast<int64_t>(num_plates));
	plate_moist_arr.resize(static_cast<int64_t>(num_plates));
	std::vector<double> plate_elev_sum(num_plates, 0.0);
	std::vector<double> plate_moist_sum(num_plates, 0.0);
	std::vector<size_t> plate_count(num_plates, 0u);
	for (size_t i = 0; i < tectonic_plates.region_to_plate.size(); ++i) {
		int32_t p = tectonic_plates.region_to_plate[i];
		if (p < 0 || static_cast<size_t>(p) >= num_plates) continue;
		if (i < region_elevation.size()) plate_elev_sum[static_cast<size_t>(p)] += static_cast<double>(region_elevation[i]);
		if (i < region_moisture.size()) plate_moist_sum[static_cast<size_t>(p)] += static_cast<double>(region_moisture[i]);
		plate_count[static_cast<size_t>(p)]++;
	}
	for (size_t p = 0; p < num_plates; ++p) {
		if (plate_count[p] > 0) {
			plate_elev_arr.set(static_cast<int64_t>(p), static_cast<float>(plate_elev_sum[p] / static_cast<double>(plate_count[p])));
			plate_moist_arr.set(static_cast<int64_t>(p), static_cast<float>(plate_moist_sum[p] / static_cast<double>(plate_count[p])));
		} else {
			plate_elev_arr.set(static_cast<int64_t>(p), 0.0f);
			plate_moist_arr.set(static_cast<int64_t>(p), 0.5f);
		}
	}
	out["plate_elevation"] = plate_elev_arr;
	out["plate_moisture"] = plate_moist_arr;

	// Half-edge mesh connectivity (for streaming / detailed map generation)
	const growth::SphereHalfEdgeMesh &mesh = globe.mesh;
	const size_t num_sides = mesh.num_sides();
	constexpr int32_t mesh_invalid = -1;
	PackedInt32Array s_begin_r_arr, s_end_r_arr, s_inner_t_arr, s_outer_t_arr, s_next_s_arr, s_twin_s_arr;
	s_begin_r_arr.resize(static_cast<int64_t>(num_sides));
	s_end_r_arr.resize(static_cast<int64_t>(num_sides));
	s_inner_t_arr.resize(static_cast<int64_t>(num_sides));
	s_outer_t_arr.resize(static_cast<int64_t>(num_sides));
	s_next_s_arr.resize(static_cast<int64_t>(num_sides));
	s_twin_s_arr.resize(static_cast<int64_t>(num_sides));
	for (size_t i = 0; i < num_sides; ++i) {
		s_begin_r_arr.set(static_cast<int64_t>(i), static_cast<int32_t>(mesh.s_begin_r[i]));
		s_end_r_arr.set(static_cast<int64_t>(i), static_cast<int32_t>(mesh.s_end_r[i]));
		s_inner_t_arr.set(static_cast<int64_t>(i), static_cast<int32_t>(mesh.s_inner_t[i]));
		s_outer_t_arr.set(static_cast<int64_t>(i), mesh.s_outer_t[i] == growth::SphereHalfEdgeMesh::k_invalid ? mesh_invalid : static_cast<int32_t>(mesh.s_outer_t[i]));
		s_next_s_arr.set(static_cast<int64_t>(i), static_cast<int32_t>(mesh.s_next_s[i]));
		s_twin_s_arr.set(static_cast<int64_t>(i), mesh.s_twin_s[i] == growth::SphereHalfEdgeMesh::k_invalid ? mesh_invalid : static_cast<int32_t>(mesh.s_twin_s[i]));
	}
	out["mesh_s_begin_r"] = s_begin_r_arr;
	out["mesh_s_end_r"] = s_end_r_arr;
	out["mesh_s_inner_t"] = s_inner_t_arr;
	out["mesh_s_outer_t"] = s_outer_t_arr;
	out["mesh_s_next_s"] = s_next_s_arr;
	out["mesh_s_twin_s"] = s_twin_s_arr;

	log_line(log_lines, "[WorldGen] Done.");
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
	auto it = _ui_asset_paths.find(p_asset_id.utf8().get_data());
	if (it == _ui_asset_paths.end())
		return String();
	return String(it->second.c_str());
}

} // namespace godot
