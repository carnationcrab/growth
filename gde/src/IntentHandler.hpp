#pragma once

#include <godot_cpp/variant/dictionary.hpp>
#include "bridge/SimBridge.hpp"

namespace godot {

namespace growth_sim {

/// Parse intent dictionary and apply to the bridge (e.g. dig). No-op if type unknown or bridge null.
void apply_intent(growth::SimBridge *bridge, const Dictionary &intent_dict);

} // namespace growth_sim

} // namespace godot
