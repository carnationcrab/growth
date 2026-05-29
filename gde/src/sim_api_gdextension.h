#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/string.hpp>
#include "base/gateway/Cstring.hpp"

namespace godot {

class GrowthSim : public RefCounted {
	GDCLASS(GrowthSim, RefCounted)

	struct BridgeHolder;

	BridgeHolder *bridge_ = nullptr;
	PackedInt32Array _last_world_gen_triangles;

protected:
	static void _bind_methods();

public:
	GrowthSim();
	~GrowthSim();

	/// Initialise the sim (bridge + defs path only). No generation; call apply_world_gen_form when the user submits the form.
	void boot(const String &defdb_path);
	/// Run world gen; returns Dictionary with "sites" and "triangles". Triangles also available via get_last_world_gen_triangles() for reliable marshalling.
	Dictionary apply_world_gen_form(const Dictionary &form_dict);
	/// Return the triangles (flat indices) from the last apply_world_gen_form call. PackedInt32Array direct return marshals to C#.
	PackedInt32Array get_last_world_gen_triangles() const;
	void step(double dt, double speed);
	void set_speed(int mode);
	void request_chunks(Vector2i center_chunk, int radius);
	Array poll_diffs(int max_count);
	void apply_intent(const Dictionary &intent_dict);

	/// <summary>Return res:// path for a UI asset id from data/core/defs/ui_assets.xml. Empty if unknown.</summary>
	String get_ui_asset_path(const String &p_asset_id);

	bool has_overworld() const;
	void commit_overworld_for_play();
	Dictionary sample_surface(Vector3 unit_dir);
};

} // namespace godot
