#pragma once

#include "Defs.hpp"
#include "base/gateway/Cstring.hpp"
#include "base/gateway/Cunordered_map.hpp"

namespace growth {

/// Merged view of all definition packs. Single XML entry point for sim_core.
class DefDatabase {
public:
	/// Load data root manifest and packs in order. Later pack overrides same ids.
	bool load(const String &data_root, const String &profile_id = "default");

	const String &data_root() const { return data_root_; }
	const String &active_profile_id() const { return active_profile_id_; }

	const GameProfileDef &game_profile() const { return active_profile_; }

	const UiAssetDef *ui_asset(const String &asset_id) const;
	const InteractionDef *interaction(const String &interaction_id) const;
	const BuildingDef *building(const String &building_id) const;
	const SceneDef *scene(const String &scene_id) const;
	const ScriptDef *script(const String &script_id) const;
	const WorldGenPipelineDef *world_gen_pipeline(const String &pipeline_id) const;

	String ui_asset_path(const String &asset_id) const;
	String scene_path(const String &scene_id) const;
	String script_path(const String &script_id) const;

	const UnorderedMap<String, UiAssetDef> &ui_assets() const { return ui_assets_; }
	const UnorderedMap<String, InteractionDef> &interactions() const { return interactions_; }
	const UnorderedMap<String, BuildingDef> &buildings() const { return buildings_; }

private:
	void load_pack(const String &pack_dir);
	void ingest_def_file(const String &path);

	void merge_ui_assets(const String &xml);
	void merge_interactions(const String &xml);
	void merge_buildings(const String &xml);
	void merge_scene_registry(const String &xml);
	void merge_game_profiles(const String &xml);
	void merge_world_gen_pipelines(const String &xml);

	void resolve_active_profile(const String &profile_id);

	String data_root_;
	String active_profile_id_;
	GameProfileDef active_profile_;

	UnorderedMap<String, UiAssetDef> ui_assets_;
	UnorderedMap<String, InteractionDef> interactions_;
	UnorderedMap<String, BuildingDef> buildings_;
	UnorderedMap<String, SceneDef> scenes_;
	UnorderedMap<String, ScriptDef> scripts_;
	UnorderedMap<String, GameProfileDef> game_profiles_;
	UnorderedMap<String, WorldGenPipelineDef> world_gen_pipelines_;
};

} // namespace growth
