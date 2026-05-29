class_name WorldGenCardUiStyling
extends RefCounted
## Applies panel and button textures from SimAPI ui_assets to the world-gen card.


static func apply(menu_root: Control) -> void:
	var card := menu_root.get_node_or_null(WorldGenCardPaths.CARD_PANEL) as PanelContainer
	if card:
		_apply_panel_card(card)

	var vbox := menu_root.get_node_or_null(WorldGenCardPaths.CARD_VBOX) as VBoxContainer
	if not vbox:
		return

	var btn_styles := _load_button_styles()
	if btn_styles.is_empty():
		return

	_apply_button_style(vbox.get_node_or_null(WorldGenCardPaths.RANDOM_SEED_BTN), btn_styles)
	_apply_button_style(vbox.get_node_or_null(WorldGenCardPaths.BACK_BTN), btn_styles)
	_apply_button_style(vbox.get_node_or_null(WorldGenCardPaths.GENERATE_BTN), btn_styles)


static func _apply_panel_card(card: PanelContainer) -> void:
	var panel_path: String = SimAPI.get_ui_asset_path("panel_card")
	if panel_path.is_empty():
		return
	var tex := load(panel_path) as Texture2D
	if not tex:
		return
	var style := StyleBoxTexture.new()
	style.texture = tex
	card.add_theme_stylebox_override("panel", style)


static func _load_button_styles() -> Dictionary:
	var btn_normal_path: String = SimAPI.get_ui_asset_path("button_primary")
	if btn_normal_path.is_empty():
		return {}

	var normal_tex := load(btn_normal_path) as Texture2D
	if not normal_tex:
		return {}

	var btn_hover_path: String = SimAPI.get_ui_asset_path("button_secondary")
	var hover_tex := load(btn_hover_path) as Texture2D if not btn_hover_path.is_empty() else normal_tex

	var normal_style := StyleBoxTexture.new()
	normal_style.texture = normal_tex
	var hover_style := StyleBoxTexture.new()
	hover_style.texture = hover_tex
	var pressed_style := StyleBoxTexture.new()
	pressed_style.texture = hover_tex

	return {
		"normal": normal_style,
		"hover": hover_style,
		"pressed": pressed_style,
	}


static func _apply_button_style(btn: Button, styles: Dictionary) -> void:
	if not btn:
		return
	btn.add_theme_stylebox_override("normal", styles["normal"])
	btn.add_theme_stylebox_override("hover", styles["hover"])
	btn.add_theme_stylebox_override("pressed", styles["pressed"])
