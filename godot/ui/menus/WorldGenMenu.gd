extends Control
## World generation menu. UI assets (panel, buttons) are resolved via SimAPI from data/core/defs/ui_assets.xml.
## Starts as the game entry; Generate logs settings and switches to the main game scene.

func _ready() -> void:
	_apply_ui_assets()
	_setup_menu()
	_connect_signals()


func _setup_menu() -> void:
	var vbox := get_node_or_null("MarginContainer/CenterContainer/Card/CardPadding/CardVBox") as VBoxContainer
	if not vbox:
		return
	var size_opt := vbox.get_node_or_null("SizeRow/SizeOpt") as OptionButton
	if size_opt and size_opt.item_count == 0:
		size_opt.add_item("Small", 0)
		size_opt.add_item("Medium", 1)
		size_opt.add_item("Large", 2)
		size_opt.select(1)
	var points_slider := vbox.get_node_or_null("PointsBlock/PointsSlider") as HSlider
	if points_slider:
		points_slider.max_value = 500000.0
	var planet_terrain_mesh_check := vbox.get_node_or_null(
		"PlanetTerrainMeshRow/PlanetTerrainMeshCheckBox") as CheckBox
	if planet_terrain_mesh_check:
		planet_terrain_mesh_check.button_pressed = true


func _connect_signals() -> void:
	var vbox := get_node_or_null("MarginContainer/CenterContainer/Card/CardPadding/CardVBox") as VBoxContainer
	if not vbox:
		return
	var random_btn := vbox.get_node_or_null("SeedRow/RandomSeedBtn") as Button
	if random_btn:
		random_btn.pressed.connect(_on_random_seed_pressed)
	var generate_btn := vbox.get_node_or_null("BottomRow/GenerateBtn") as Button
	if generate_btn:
		generate_btn.pressed.connect(_on_generate_pressed)
	var temp_slider := vbox.get_node_or_null("TempBlock/TempSlider") as HSlider
	var temp_value := vbox.get_node_or_null("TempBlock/TempTopRow/TempValue") as Label
	if temp_slider and temp_value:
		temp_value.text = _format_percent(temp_slider.value)
		temp_slider.value_changed.connect(func(v: float) -> void: temp_value.text = _format_percent(v))
	var precip_slider := vbox.get_node_or_null("PrecipBlock/PrecipSlider") as HSlider
	var precip_value := vbox.get_node_or_null("PrecipBlock/PrecipTopRow/PrecipValue") as Label
	if precip_slider and precip_value:
		precip_value.text = _format_percent(precip_slider.value)
		precip_slider.value_changed.connect(func(v: float) -> void: precip_value.text = _format_percent(v))
	var points_slider := vbox.get_node_or_null("PointsBlock/PointsSlider") as HSlider
	var points_value := vbox.get_node_or_null("PointsBlock/PointsTopRow/PointsValue") as Label
	if points_slider and points_value:
		points_value.text = str(int(points_slider.value))
		points_slider.value_changed.connect(func(v: float) -> void: points_value.text = str(int(v)))
	var plates_slider := vbox.get_node_or_null("PlatesBlock/PlatesSlider") as HSlider
	var plates_value := vbox.get_node_or_null("PlatesBlock/PlatesTopRow/PlatesValue") as Label
	if plates_slider and plates_value:
		plates_value.text = str(int(plates_slider.value))
		plates_slider.value_changed.connect(func(v: float) -> void: plates_value.text = str(int(v)))
	var jitter_slider := vbox.get_node_or_null("JitterBlock/JitterSlider") as HSlider
	var jitter_value := vbox.get_node_or_null("JitterBlock/JitterTopRow/JitterValue") as Label
	if jitter_slider and jitter_value:
		jitter_value.text = _format_percent(jitter_slider.value)
		jitter_slider.value_changed.connect(func(v: float) -> void: jitter_value.text = _format_percent(v))


func _format_percent(v: float) -> String:
	return "%.0f%%" % v


func _on_random_seed_pressed() -> void:
	var vbox := get_node_or_null("MarginContainer/CenterContainer/Card/CardPadding/CardVBox") as VBoxContainer
	var seed_edit := vbox.get_node_or_null("SeedRow/SeedEdit") as LineEdit if vbox else null
	if seed_edit:
		seed_edit.text = str(randi())


func _on_generate_pressed() -> void:
	var vbox := get_node_or_null("MarginContainer/CenterContainer/Card/CardPadding/CardVBox") as VBoxContainer
	if not vbox:
		return
	var seed_edit := vbox.get_node_or_null("SeedRow/SeedEdit") as LineEdit
	var size_opt := vbox.get_node_or_null("SizeRow/SizeOpt") as OptionButton
	var temp_slider := vbox.get_node_or_null("TempBlock/TempSlider") as HSlider
	var precip_slider := vbox.get_node_or_null("PrecipBlock/PrecipSlider") as HSlider
	var points_slider := vbox.get_node_or_null("PointsBlock/PointsSlider") as HSlider
	var jitter_slider := vbox.get_node_or_null("JitterBlock/JitterSlider") as HSlider
	var plates_slider := vbox.get_node_or_null("PlatesBlock/PlatesSlider") as HSlider
	var planet_terrain_mesh_check := vbox.get_node_or_null("PlanetTerrainMeshRow/PlanetTerrainMeshCheckBox") as CheckBox

	var seed_text: String = seed_edit.text if seed_edit else ""
	var world_size_idx: int = size_opt.selected if size_opt else 1
	var temperature: float = temp_slider.value if temp_slider else 0.0
	var precipitation: float = precip_slider.value if precip_slider else 0.0
	var num_points: int = int(points_slider.value) if points_slider else WorldGenCardPaths.DEFAULT_VORONOI_SITES
	var jitter: float = jitter_slider.value if jitter_slider else 0.0
	var num_plates: int = int(plates_slider.value) if plates_slider else WorldGenCardPaths.DEFAULT_NUM_PLATES
	var use_planet_terrain_mesh: bool = (
		planet_terrain_mesh_check.button_pressed if planet_terrain_mesh_check else true)

	var form_dict: Dictionary = {
		"seed": seed_text,
		"world_size": world_size_idx,
		"temperature": temperature,
		"precipitation": precipitation,
		"planet_preset": "Earthlike",
		"voronoi_sites": num_points,
		"jitter": jitter,
		"num_plate_regions": num_plates,
		"use_planet_terrain_mesh": use_planet_terrain_mesh
	}
	print("[WorldGen] Form submitted: use_planet_terrain_mesh=", use_planet_terrain_mesh, " (checkbox found=", planet_terrain_mesh_check != null, ")")

	var load_screen_scene := load("res://godot/ui/menus/worldGenLoadScreen.tscn") as PackedScene
	if not load_screen_scene:
		push_error("[WorldGen] failed to load WorldGenLoadScreen.tscn")
		return
	var load_screen = load_screen_scene.instantiate()
	var ui_layer := get_parent()
	if not ui_layer:
		push_error("[WorldGen] no parent UILayer")
		return
	ui_layer.add_child(load_screen)
	load_screen.generation_finished.connect(_on_load_screen_finished)
	load_screen.run_generation(form_dict)


func _on_load_screen_finished(result: Dictionary) -> void:
	_apply_world_gen_result(result)
	queue_free()


func _apply_world_gen_result(result: Dictionary) -> void:
	var main = get_parent().get_parent() if get_parent() else null
	if not main:
		return
	WorldGenPreviewApplier.apply(main, result)


func _apply_ui_assets() -> void:
	var card := get_node_or_null("MarginContainer/CenterContainer/Card") as PanelContainer
	if card:
		var panel_path: String = SimAPI.get_ui_asset_path("panel_card")
		if not panel_path.is_empty():
			var tex = load(panel_path) as Texture2D
			if tex:
				var style := StyleBoxTexture.new()
				style.texture = tex
				card.add_theme_stylebox_override("panel", style)

	var btn_normal_path: String = SimAPI.get_ui_asset_path("button_primary")
	var btn_hover_path: String = SimAPI.get_ui_asset_path("button_secondary")
	if btn_normal_path.is_empty():
		return

	var normal_tex = load(btn_normal_path) as Texture2D
	var hover_tex = load(btn_hover_path) as Texture2D if not btn_hover_path.is_empty() else normal_tex

	var normal_style := StyleBoxTexture.new()
	normal_style.texture = normal_tex
	var hover_style := StyleBoxTexture.new()
	hover_style.texture = hover_tex
	var pressed_style := StyleBoxTexture.new()
	pressed_style.texture = hover_tex

	var vbox := get_node_or_null("MarginContainer/CenterContainer/Card/CardPadding/CardVBox") as VBoxContainer
	if not vbox:
		return
	_apply_button_style(vbox.get_node_or_null("SeedRow/RandomSeedBtn"), normal_style, hover_style, pressed_style)
	_apply_button_style(vbox.get_node_or_null("BottomRow/BackBtn"), normal_style, hover_style, pressed_style)
	_apply_button_style(vbox.get_node_or_null("BottomRow/GenerateBtn"), normal_style, hover_style, pressed_style)


func _apply_button_style(btn: Button, normal: StyleBoxTexture, hover: StyleBoxTexture, pressed: StyleBoxTexture) -> void:
	if not btn:
		return
	btn.add_theme_stylebox_override("normal", normal)
	btn.add_theme_stylebox_override("hover", hover)
	btn.add_theme_stylebox_override("pressed", pressed)
