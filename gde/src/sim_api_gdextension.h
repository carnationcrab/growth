#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>
#include <unordered_map>
#include <string>

namespace godot {

class GrowthSim : public RefCounted {
	GDCLASS(GrowthSim, RefCounted)

	struct BridgeHolder;

	BridgeHolder *bridge_ = nullptr;

protected:
	static void _bind_methods();

public:
	GrowthSim();
	~GrowthSim();

	void boot(const String &defdb_path);
	void step(double dt, double speed);
	void set_speed(int mode);
	void request_chunks(Vector2i center_chunk, int radius);
	Array poll_diffs(int max_count);
	void apply_intent(const Dictionary &intent_dict);

	/// <summary>Return res:// path for a UI asset id from data/core/defs/ui_assets.xml. Empty if unknown.</summary>
	String get_ui_asset_path(const String &p_asset_id);

private:
	std::unordered_map<std::string, std::string> _ui_asset_paths;
};

} // namespace godot
