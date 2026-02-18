#include "sim_api_gdextension.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/xml_parser.hpp>
#include "bridge/SimBridge.hpp"
#include "bridge/Diff.hpp"
#include "world/WorldCoord.hpp"
#include <memory>
#include <string>

namespace godot {

struct GrowthSim::BridgeHolder {
	growth::SimBridge bridge;
};

void GrowthSim::_bind_methods() {
	ClassDB::bind_method(D_METHOD("boot", "defdb_path"), &GrowthSim::boot);
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
	bridge_->bridge.boot(0, path);

	_ui_asset_paths.clear();
	String base = defdb_path.ends_with("/") ? defdb_path : defdb_path + "/";
	String asset_file = base + "defs/ui_assets.xml";
	Ref<FileAccess> f = FileAccess::open(asset_file, FileAccess::READ);
	if (f.is_valid()) {
		PackedByteArray buf = f->get_buffer((int64_t)f->get_length());
		Ref<XMLParser> parser;
		parser.instantiate();
		if (parser->open_buffer(buf) == OK) {
			while (parser->read() == OK) {
				if (parser->get_node_type() == XMLParser::NODE_ELEMENT && parser->get_node_name() == "asset") {
					String id = parser->get_named_attribute_value_safe("asset_id");
					String path_val = parser->get_named_attribute_value_safe("path");
					if (id.length() > 0 && path_val.length() > 0)
						_ui_asset_paths[id.utf8().get_data()] = path_val.utf8().get_data();
				}
			}
		}
	}
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
	if (bridge_) { /* optional: bridge_->bridge.set_speed(mode); */ }
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
		Dictionary dict;
		if (d.type == growth::DiffType::ChunkLoaded) {
			dict["type"] = String("ChunkLoaded");
			dict["coord"] = Vector2i(d.chunk_loaded.coord.x, d.chunk_loaded.coord.z);
			PackedFloat32Array arr;
			arr.resize((int)d.chunk_loaded.height_samples.size());
			for (size_t i = 0; i < d.chunk_loaded.height_samples.size(); ++i)
				arr[(int)i] = d.chunk_loaded.height_samples[i];
			dict["height_samples"] = arr;
		} else if (d.type == growth::DiffType::ChunkUnloaded) {
			dict["type"] = String("ChunkUnloaded");
			dict["coord"] = Vector2i(d.chunk_unloaded.coord.x, d.chunk_unloaded.coord.z);
		} else if (d.type == growth::DiffType::CellChanged) {
			dict["type"] = String("CellChanged");
			dict["chunk_coord"] = Vector2i(d.cell_changed.chunk_coord.x, d.cell_changed.chunk_coord.z);
			dict["local_xz"] = Vector2i(d.cell_changed.local_x, d.cell_changed.local_z);
			dict["layer"] = d.cell_changed.layer;
			dict["new_value"] = d.cell_changed.new_value;
		}
		out.push_back(dict);
		++n;
	}
	return out;
}

void GrowthSim::apply_intent(const Dictionary &intent_dict) {
	if (!bridge_) return;
	Variant type_var = intent_dict.get("type", Variant());
	String type_str = type_var.is_string() ? type_var : String();
	if (type_str == "dig") {
		Variant coord_var = intent_dict.get("chunk_coord", Variant());
		Variant local_var = intent_dict.get("local_xz", Variant());
		Vector2i coord = coord_var.get_type() == Variant::VECTOR2I ? coord_var : Vector2i();
		Vector2i local = local_var.get_type() == Variant::VECTOR2I ? local_var : Vector2i();
		bridge_->bridge.apply_intent_dig(
			growth::ChunkCoord{ coord.x, coord.y },
			local.x, local.y
		);
	}
}

String GrowthSim::get_ui_asset_path(const String &p_asset_id) {
	auto it = _ui_asset_paths.find(p_asset_id.utf8().get_data());
	if (it == _ui_asset_paths.end())
		return String();
	return String(it->second.c_str());
}

} // namespace godot
