#include "base/gateway/Cstring.hpp"
#include "base/gateway/Cunordered_map.hpp"
#include "base/gateway/Cvector.hpp"
#pragma once


namespace growth {

struct UiAssetDef {
	String asset_id;
	String path;
};

struct InteractionDef {
	String interaction_id;
	String action_type;
	String display_name;
	String condition_set_id;
	String tool_id;
	String ui_hint;
};

struct EffectRefDef {
	String effect_id;
	double magnitude = 0.0;
	double duration  = 0.0;
	bool has_magnitude = false;
	bool has_duration  = false;
};

struct BuildingDef {
	String building_id;
	String display_name;
	String footprint_pattern_id;
	String placement_rules_id;
	String construction_recipe_id;
	String deterioration_profile_id;
	String visual_prefab_tag;
	Vector<EffectRefDef> ongoing_effects;
};

struct SceneDef {
	String scene_id;
	String path;
};

struct ScriptDef {
	String script_id;
	String path;
};

struct GameProfileDef {
	String profile_id;
	String entry_menu_scene_id;
	String world_gen_load_screen_scene_id;
	String world_view_script_id;
	String planet_preview_scene_id;
	String world_gen_pipeline_id;
};

struct WorldGenStageRef {
	String stage_id;
	bool optional = false;
};

struct WorldGenPipelineDef {
	String pipeline_id;
	Vector<WorldGenStageRef> stages;
};

} // namespace growth
