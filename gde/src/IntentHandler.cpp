#include "IntentHandler.hpp"
#include "world/WorldCoord.hpp"
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/vector2i.hpp>

namespace godot {

namespace growth_sim {

void apply_intent(growth::SimBridge *bridge, const Dictionary &intent_dict) {
	if (!bridge) return;
	Variant type_var = intent_dict.get("type", Variant());
	String type_str = (type_var.get_type() == Variant::STRING) ? type_var.operator String() : String();
	if (type_str != "dig") return;
	Variant coord_var = intent_dict.get("chunk_coord", Variant());
	Variant local_var = intent_dict.get("local_xz", Variant());
	Vector2i coord = coord_var.get_type() == Variant::VECTOR2I ? coord_var : Vector2i();
	Vector2i local = local_var.get_type() == Variant::VECTOR2I ? local_var : Vector2i();
	bridge->apply_intent_dig(
		growth::ChunkCoord{ coord.x, coord.y },
		local.x, local.y
	);
}

} // namespace growth_sim

} // namespace godot
