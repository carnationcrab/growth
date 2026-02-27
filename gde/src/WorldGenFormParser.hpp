#pragma once

#include <godot_cpp/variant/dictionary.hpp>
#include "api/ParsedWorldGenForm.hpp"
#include <string>
#include <unordered_map>

namespace godot {

namespace growth_sim {

/// Parse Godot world-gen form dictionary into platform-agnostic ParsedWorldGenForm.
void parse_world_gen_form(const Dictionary &form_dict, growth::ParsedWorldGenForm &out);

} // namespace growth_sim

} // namespace godot
