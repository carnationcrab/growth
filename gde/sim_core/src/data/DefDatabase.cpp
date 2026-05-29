#include "data/DefDatabase.hpp"
#include "data/XmlLite.hpp"
#include "base/gateway/Calgorithm.hpp"
#include "base/gateway/Ccctype.hpp"
#include "base/gateway/Csstream.hpp"
#include "base/gateway/Cutility.hpp"

namespace growth {

namespace {

	String join_path(const String &base, const String &rel) {
		String root = base;
		if (!root.empty() && root.back() != '/' && root.back() != '\\')
			root += '/';
		String r = rel;
		while (!r.empty() && (r.front() == '/' || r.front() == '\\'))
			r.erase(r.begin());
		return root + r;
	}

	String file_basename(const String &path) {
		const auto slash = path.find_last_of("/\\");
		return slash == String::npos ? path : path.substr(slash + 1);
	}

	double parse_decimal(const String &s) {
		try {
			return Cstring::stod(s);
		} catch (...) {
			return 0.0;
		}
	}

} // namespace

bool DefDatabase::load(const String &data_root, const String &profile_id) {
	data_root_         = data_root;
	active_profile_id_ = profile_id;
	if (!data_root_.empty() && data_root_.back() != '/' && data_root_.back() != '\\')
		data_root_ += '/';

	const String manifest_path = data_root_ + "manifest.xml";
	const String manifest_xml  = xml_lite::read_file(manifest_path);
	if (manifest_xml.empty())
		return false;

	const auto packs = xml_lite::parse_pack_refs(manifest_xml);
	for (const auto &pack : packs)
		load_pack(join_path(data_root_, pack.path));

	resolve_active_profile(profile_id);
	return !game_profiles_.empty() || !ui_assets_.empty();
}

void DefDatabase::load_pack(const String &pack_dir) {
	String dir = pack_dir;
	if (!dir.empty() && dir.back() != '/' && dir.back() != '\\')
		dir += '/';

	const String pack_xml = xml_lite::read_file(dir + "manifest.xml");
	if (pack_xml.empty())
		return;

	for (const auto &rel : xml_lite::element_texts(pack_xml, "file")) {
		const String full = join_path(dir, rel);
		if (full.find("/defs/") != String::npos || full.find("\\defs\\") != String::npos)
			ingest_def_file(full);
	}
}

void DefDatabase::ingest_def_file(const String &path) {
	const String xml = xml_lite::read_file(path);
	if (xml.empty())
		return;

	const String base = file_basename(path);
	if (base == "ui_assets.xml")
		merge_ui_assets(xml);
	else if (base == "interactions.xml")
		merge_interactions(xml);
	else if (base == "buildings.xml")
		merge_buildings(xml);
	else if (base == "scene_registry.xml")
		merge_scene_registry(xml);
	else if (base == "game_profile.xml")
		merge_game_profiles(xml);
	else if (base == "world_gen_pipelines.xml")
		merge_world_gen_pipelines(xml);
}

void DefDatabase::merge_ui_assets(const String &xml) {
	for (const auto &line : xml_lite::lines_containing(xml, "<asset")) {
		UiAssetDef def;
		if (auto id = xml_lite::attr(line, "asset_id")) def.asset_id = *id;
		if (auto p = xml_lite::attr(line, "path")) def.path = *p;
		if (!def.asset_id.empty() && !def.path.empty())
			ui_assets_[def.asset_id] = Cutility::move(def);
	}
}

void DefDatabase::merge_interactions(const String &xml) {
	for (const auto &line : xml_lite::lines_containing(xml, "<interaction_def")) {
		InteractionDef def;
		if (auto v = xml_lite::attr(line, "interaction_id")) def.interaction_id = *v;
		if (auto v = xml_lite::attr(line, "action_type")) def.action_type = *v;
		if (auto v = xml_lite::attr(line, "display_name")) def.display_name = *v;
		if (auto v = xml_lite::attr(line, "condition_set_id")) def.condition_set_id = *v;
		if (auto v = xml_lite::attr(line, "tool_id")) def.tool_id = *v;
		if (auto v = xml_lite::attr(line, "ui_hint")) def.ui_hint = *v;
		if (!def.interaction_id.empty())
			interactions_[def.interaction_id] = Cutility::move(def);
	}
}

void DefDatabase::merge_buildings(const String &xml) {
	BuildingDef *current = nullptr;
	IStringStream stream(xml);
	String line;
	while (getline(stream, line)) {
		if (line.find("<building") != String::npos) {
			BuildingDef def;
			if (auto v = xml_lite::attr(line, "building_id")) def.building_id = *v;
			if (auto v = xml_lite::attr(line, "display_name")) def.display_name = *v;
			if (auto v = xml_lite::attr(line, "footprint_pattern_id")) def.footprint_pattern_id = *v;
			if (auto v = xml_lite::attr(line, "placement_rules_id")) def.placement_rules_id = *v;
			if (auto v = xml_lite::attr(line, "construction_recipe_id")) def.construction_recipe_id = *v;
			if (auto v = xml_lite::attr(line, "deterioration_profile_id")) def.deterioration_profile_id = *v;
			if (auto v = xml_lite::attr(line, "visual_prefab_tag")) def.visual_prefab_tag = *v;
			if (!def.building_id.empty()) {
				buildings_[def.building_id] = def;
				current                       = &buildings_[def.building_id];
			} else {
				current = nullptr;
			}
		} else if (line.find("</building>") != String::npos) {
			current = nullptr;
		} else if (current != nullptr && line.find("<effect_ref") != String::npos) {
			EffectRefDef eff;
			if (auto v = xml_lite::attr(line, "effect_id")) eff.effect_id = *v;
			if (auto v = xml_lite::attr(line, "magnitude")) {
				eff.magnitude      = parse_decimal(*v);
				eff.has_magnitude  = true;
			}
			if (auto v = xml_lite::attr(line, "duration")) {
				eff.duration      = parse_decimal(*v);
				eff.has_duration  = true;
			}
			if (!eff.effect_id.empty())
				current->ongoing_effects.push_back(Cutility::move(eff));
		}
	}
}

void DefDatabase::merge_scene_registry(const String &xml) {
	for (const auto &line : xml_lite::lines_containing(xml, "<scene")) {
		if (line.find("<script") != String::npos) continue;
		SceneDef def;
		if (auto v = xml_lite::attr(line, "scene_id")) def.scene_id = *v;
		if (auto v = xml_lite::attr(line, "path")) def.path = *v;
		if (!def.scene_id.empty() && !def.path.empty())
			scenes_[def.scene_id] = Cutility::move(def);
	}
	for (const auto &line : xml_lite::lines_containing(xml, "<script")) {
		ScriptDef def;
		if (auto v = xml_lite::attr(line, "script_id")) def.script_id = *v;
		if (auto v = xml_lite::attr(line, "path")) def.path = *v;
		if (!def.script_id.empty() && !def.path.empty())
			scripts_[def.script_id] = Cutility::move(def);
	}
}

void DefDatabase::merge_game_profiles(const String &xml) {
	String current_profile;
	bool in_opening_tag = false;

	IStringStream stream(xml);
	String line;
	while (getline(stream, line)) {
		if (line.find("<game_profile") != String::npos) {
			in_opening_tag  = true;
			current_profile = {};
		}

		if (in_opening_tag) {
			if (auto v = xml_lite::attr(line, "profile_id"))
				current_profile = *v;
			if (line.find('>') != String::npos) {
				in_opening_tag = false;
				if (!current_profile.empty())
					game_profiles_[current_profile].profile_id = current_profile;
			}
			continue;
		}

		if (line.find("</game_profile>") != String::npos) {
			current_profile.clear();
			continue;
		}

		if (current_profile.empty())
			continue;

		auto &prof = game_profiles_[current_profile];
		if (line.find("<entry_menu") != String::npos) {
			if (auto v = xml_lite::attr(line, "scene_id")) prof.entry_menu_scene_id = *v;
		} else if (line.find("<world_gen_load_screen") != String::npos) {
			if (auto v = xml_lite::attr(line, "scene_id")) prof.world_gen_load_screen_scene_id = *v;
		} else if (line.find("<world_view") != String::npos) {
			if (auto v = xml_lite::attr(line, "script_id")) prof.world_view_script_id = *v;
		} else if (line.find("<planet_preview") != String::npos) {
			if (auto v = xml_lite::attr(line, "scene_id")) prof.planet_preview_scene_id = *v;
		} else if (line.find("<world_gen") != String::npos && line.find("world_gen_load_screen") == String::npos) {
			if (auto v = xml_lite::attr(line, "pipeline_id")) prof.world_gen_pipeline_id = *v;
		}
	}
}

void DefDatabase::merge_world_gen_pipelines(const String &xml) {
	String current_pipeline;
	IStringStream stream(xml);
	String line;
	while (getline(stream, line)) {
		if (line.find("<pipeline") != String::npos) {
			WorldGenPipelineDef def;
			if (auto v = xml_lite::attr(line, "pipeline_id")) def.pipeline_id = *v;
			if (!def.pipeline_id.empty()) {
				current_pipeline = def.pipeline_id;
				world_gen_pipelines_[current_pipeline] = Cutility::move(def);
			}
		} else if (line.find("</pipeline>") != String::npos) {
			current_pipeline.clear();
		} else if (!current_pipeline.empty() && line.find("<stage") != String::npos) {
			WorldGenStageRef stage;
			if (auto v = xml_lite::attr(line, "ref")) stage.stage_id = *v;
			if (auto v = xml_lite::attr(line, "optional")) {
				String o = *v;
				Calgorithm::transform(o.begin(), o.end(), o.begin(), [](unsigned char c) { return static_cast<char>(Ccctype::tolower(c)); });
				stage.optional = (o == "true" || o == "1");
			}
			if (!stage.stage_id.empty())
				world_gen_pipelines_[current_pipeline].stages.push_back(Cutility::move(stage));
		}
	}
}

void DefDatabase::resolve_active_profile(const String &profile_id) {
	const auto it = game_profiles_.find(profile_id);
	if (it != game_profiles_.end()) {
		active_profile_ = it->second;
		return;
	}
	if (!game_profiles_.empty())
		active_profile_ = game_profiles_.begin()->second;
}

const UiAssetDef *DefDatabase::ui_asset(const String &asset_id) const {
	const auto it = ui_assets_.find(asset_id);
	return it == ui_assets_.end() ? nullptr : &it->second;
}

const InteractionDef *DefDatabase::interaction(const String &interaction_id) const {
	const auto it = interactions_.find(interaction_id);
	return it == interactions_.end() ? nullptr : &it->second;
}

const BuildingDef *DefDatabase::building(const String &building_id) const {
	const auto it = buildings_.find(building_id);
	return it == buildings_.end() ? nullptr : &it->second;
}

const SceneDef *DefDatabase::scene(const String &scene_id) const {
	const auto it = scenes_.find(scene_id);
	return it == scenes_.end() ? nullptr : &it->second;
}

const ScriptDef *DefDatabase::script(const String &script_id) const {
	const auto it = scripts_.find(script_id);
	return it == scripts_.end() ? nullptr : &it->second;
}

const WorldGenPipelineDef *DefDatabase::world_gen_pipeline(const String &pipeline_id) const {
	const auto it = world_gen_pipelines_.find(pipeline_id);
	return it == world_gen_pipelines_.end() ? nullptr : &it->second;
}

String DefDatabase::ui_asset_path(const String &asset_id) const {
	const auto *def = ui_asset(asset_id);
	return def ? def->path : String{};
}

String DefDatabase::scene_path(const String &scene_id) const {
	const auto *def = scene(scene_id);
	return def ? def->path : String{};
}

String DefDatabase::script_path(const String &script_id) const {
	const auto *def = script(script_id);
	return def ? def->path : String{};
}

} // namespace growth
