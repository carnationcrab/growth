#include "UiAssetLoader.hpp"
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/xml_parser.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

namespace godot {

namespace growth_sim {

std::unordered_map<std::string, std::string> load_ui_assets(const String &defdb_base_path) {
	std::unordered_map<std::string, std::string> out;
	String base = defdb_base_path.ends_with("/") ? defdb_base_path : (defdb_base_path + String("/"));
	String asset_file = base + String("defs/ui_assets.xml");
	Ref<FileAccess> f = FileAccess::open(asset_file, FileAccess::READ);
	if (!f.is_valid()) return out;

	PackedByteArray buf = f->get_buffer((int64_t)f->get_length());
	Ref<XMLParser> parser;
	parser.instantiate();
	if (parser->open_buffer(buf) != OK) return out;

	while (parser->read() == OK) {
		if (parser->get_node_type() == XMLParser::NODE_ELEMENT && parser->get_node_name() == "asset") {
			String id = parser->get_named_attribute_value_safe("asset_id");
			String path_val = parser->get_named_attribute_value_safe("path");
			if (id.length() > 0 && path_val.length() > 0)
				out[id.utf8().get_data()] = path_val.utf8().get_data();
		}
	}
	return out;
}

} // namespace growth_sim

} // namespace godot
