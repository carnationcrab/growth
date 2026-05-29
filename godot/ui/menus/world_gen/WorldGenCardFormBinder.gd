class_name WorldGenCardFormBinder
extends RefCounted
## Binds world-gen card controls: populate options, wire sliders, read form dict for SimAPI.


var _vbox: VBoxContainer


func bind(menu_root: Control) -> bool:
	_vbox = menu_root.get_node_or_null(WorldGenCardPaths.CARD_VBOX) as VBoxContainer
	return _vbox != null


func setup_defaults() -> void:
	if not _vbox:
		return
	_setup_preset_option()
	_setup_size_option()
	_setup_mode_option()
	_setup_simd_default()
	_setup_points_and_terrain_defaults()


func connect_actions(on_random_seed: Callable, on_generate: Callable) -> void:
	if not _vbox:
		return
	var random_btn := _node(WorldGenCardPaths.RANDOM_SEED_BTN) as Button
	if random_btn:
		random_btn.pressed.connect(on_random_seed)
	var generate_btn := _node(WorldGenCardPaths.GENERATE_BTN) as Button
	if generate_btn:
		generate_btn.pressed.connect(on_generate)
	_connect_percent_slider(WorldGenCardPaths.TEMP_SLIDER, WorldGenCardPaths.TEMP_VALUE)
	_connect_percent_slider(WorldGenCardPaths.PRECIP_SLIDER, WorldGenCardPaths.PRECIP_VALUE)
	_connect_int_slider(WorldGenCardPaths.POINTS_SLIDER, WorldGenCardPaths.POINTS_VALUE)
	_connect_int_slider(WorldGenCardPaths.PLATES_SLIDER, WorldGenCardPaths.PLATES_VALUE)
	_connect_percent_slider(WorldGenCardPaths.JITTER_SLIDER, WorldGenCardPaths.JITTER_VALUE)


func fill_random_seed() -> void:
	var seed_edit := _node(WorldGenCardPaths.SEED_EDIT) as LineEdit
	if seed_edit:
		seed_edit.text = str(randi())


func collect_form() -> Dictionary:
	if not _vbox:
		return {}

	var seed_edit := _node(WorldGenCardPaths.SEED_EDIT) as LineEdit
	var preset_opt := _node(WorldGenCardPaths.PRESET_OPT) as OptionButton
	var size_opt := _node(WorldGenCardPaths.SIZE_OPT) as OptionButton
	var mode_opt := _node(WorldGenCardPaths.MODE_OPT) as OptionButton
	var use_simd_check := _node(WorldGenCardPaths.USE_SIMD_CHECK) as CheckBox
	var planet_terrain_mesh_check := _node(WorldGenCardPaths.PLANET_TERRAIN_MESH_CHECK) as CheckBox
	var temp_slider := _node(WorldGenCardPaths.TEMP_SLIDER) as HSlider
	var precip_slider := _node(WorldGenCardPaths.PRECIP_SLIDER) as HSlider
	var points_slider := _node(WorldGenCardPaths.POINTS_SLIDER) as HSlider
	var jitter_slider := _node(WorldGenCardPaths.JITTER_SLIDER) as HSlider
	var plates_slider := _node(WorldGenCardPaths.PLATES_SLIDER) as HSlider

	var seed_text: String = seed_edit.text if seed_edit else ""
	var preset_idx: int = preset_opt.selected if preset_opt else 0
	var planet_preset: String = _id_at(WorldGenCardPaths.PLANET_PRESET_IDS, preset_idx, WorldGenCardPaths.DEFAULT_PLANET_PRESET)
	var world_size_idx: int = size_opt.selected if size_opt else 1
	var temperature: float = temp_slider.value if temp_slider else 0.0
	var precipitation: float = precip_slider.value if precip_slider else 0.0
	var num_points: int = int(points_slider.value) if points_slider else WorldGenCardPaths.DEFAULT_VORONOI_SITES
	var jitter: float = jitter_slider.value if jitter_slider else 0.0
	var num_plates: int = int(plates_slider.value) if plates_slider else WorldGenCardPaths.DEFAULT_NUM_PLATES
	var use_planet_terrain_mesh: bool = (
		planet_terrain_mesh_check.button_pressed if planet_terrain_mesh_check else true)
	var mode_idx: int = mode_opt.selected if mode_opt else 0
	var world_gen_mode: String = _id_at(
		WorldGenCardPaths.WORLD_GEN_MODE_IDS,
		mode_idx,
		WorldGenCardPaths.DEFAULT_WORLD_GEN_MODE)

	var use_simd: bool = bool(ProjectSettings.get_setting("application/config/world_gen_use_simd", true))
	if use_simd_check:
		use_simd = use_simd_check.button_pressed

	var form_dict: Dictionary = {
		"seed": seed_text,
		"world_size": world_size_idx,
		"temperature": temperature,
		"precipitation": precipitation,
		"planet_preset": planet_preset,
		"voronoi_sites": num_points,
		"jitter": jitter,
		"num_plate_regions": num_plates,
		"use_planet_terrain_mesh": use_planet_terrain_mesh,
		"world_gen_mode": world_gen_mode,
		"use_simd": use_simd,
	}

	var sim_sites: int = _id_at_int(WorldGenCardPaths.SIM_SITES_BY_SIZE, world_size_idx, WorldGenCardPaths.DEFAULT_SIM_SITES)
	if num_points > sim_sites:
		form_dict["sim_voronoi_sites"] = sim_sites
	return form_dict


func _node(relative_path: String) -> Node:
	return _vbox.get_node_or_null(relative_path) if _vbox else null


func _setup_preset_option() -> void:
	var preset_opt := _node(WorldGenCardPaths.PRESET_OPT) as OptionButton
	if not preset_opt or preset_opt.item_count > 0:
		return
	for i in WorldGenCardPaths.PLANET_PRESET_LABELS.size():
		preset_opt.add_item(WorldGenCardPaths.PLANET_PRESET_LABELS[i], i)
	preset_opt.select(0)


func _setup_size_option() -> void:
	var size_opt := _node(WorldGenCardPaths.SIZE_OPT) as OptionButton
	if not size_opt or size_opt.item_count > 0:
		return
	size_opt.add_item("Small", 0)
	size_opt.add_item("Medium", 1)
	size_opt.add_item("Large", 2)
	size_opt.select(1)


func _setup_mode_option() -> void:
	var mode_opt := _node(WorldGenCardPaths.MODE_OPT) as OptionButton
	if not mode_opt or mode_opt.item_count > 0:
		return
	for i in WorldGenCardPaths.WORLD_GEN_MODE_LABELS.size():
		mode_opt.add_item(WorldGenCardPaths.WORLD_GEN_MODE_LABELS[i], i)
	var configured_mode := str(ProjectSettings.get_setting(
		"application/config/world_gen_mode",
		WorldGenCardPaths.DEFAULT_WORLD_GEN_MODE))
	var configured_idx := WorldGenCardPaths.WORLD_GEN_MODE_IDS.find(configured_mode)
	mode_opt.select(configured_idx if configured_idx >= 0 else 0)


func _setup_simd_default() -> void:
	var use_simd_check := _node(WorldGenCardPaths.USE_SIMD_CHECK) as CheckBox
	if use_simd_check:
		use_simd_check.button_pressed = bool(ProjectSettings.get_setting("application/config/world_gen_use_simd", true))


func _setup_points_and_terrain_defaults() -> void:
	var points_slider := _node(WorldGenCardPaths.POINTS_SLIDER) as HSlider
	if points_slider:
		points_slider.max_value = float(WorldGenCardPaths.MAX_VORONOI_SITES)
		if points_slider.value <= 1024.0:
			points_slider.value = float(WorldGenCardPaths.DEFAULT_VORONOI_SITES)
	var plates_slider := _node(WorldGenCardPaths.PLATES_SLIDER) as HSlider
	if plates_slider and plates_slider.value <= 25.0:
		plates_slider.value = float(WorldGenCardPaths.DEFAULT_NUM_PLATES)
	var planet_terrain_mesh_check := _node(WorldGenCardPaths.PLANET_TERRAIN_MESH_CHECK) as CheckBox
	if planet_terrain_mesh_check:
		planet_terrain_mesh_check.button_pressed = true

func _connect_percent_slider(slider_path: String, label_path: String) -> void:
	var slider := _node(slider_path) as HSlider
	var label := _node(label_path) as Label
	if not slider or not label:
		return
	label.text = _format_percent(slider.value)
	slider.value_changed.connect(func(v: float) -> void: label.text = _format_percent(v))


func _connect_int_slider(slider_path: String, label_path: String) -> void:
	var slider := _node(slider_path) as HSlider
	var label := _node(label_path) as Label
	if not slider or not label:
		return
	label.text = str(int(slider.value))
	slider.value_changed.connect(func(v: float) -> void: label.text = str(int(v)))


static func _format_percent(v: float) -> String:
	return "%.0f%%" % v


static func _id_at(ids: Array, index: int, fallback: String) -> String:
	if index >= 0 and index < ids.size():
		return str(ids[index])
	return fallback


static func _id_at_int(values: Array, index: int, fallback: int) -> int:
	if index >= 0 and index < values.size():
		return int(values[index])
	return fallback
