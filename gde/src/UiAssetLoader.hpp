#pragma once

#include <godot_cpp/variant/string.hpp>
#include <string>
#include <unordered_map>

namespace godot {

namespace growth_sim {

/// Load asset_id -> path from defs/ui_assets.xml under the given base path. Returns map (empty on failure).
std::unordered_map<std::string, std::string> load_ui_assets(const String &defdb_base_path);

} // namespace growth_sim

} // namespace godot
