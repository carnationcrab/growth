#pragma once

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include "bridge/Diff.hpp"

namespace godot {

namespace growth_sim {

/// Convert one growth::Diff to a Godot Dictionary for the sim API contract.
Dictionary diff_to_dictionary(const growth::Diff &d);

} // namespace growth_sim

} // namespace godot
