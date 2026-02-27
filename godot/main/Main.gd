extends Node3D
## Main scene: initialises sim bridge, ticks sim, drives world streaming.
## Milestone: run game and see a stable scene (no sim yet).

@onready var world_root: Node3D = $WorldRoot
@onready var focus: Node3D = $Focus
@onready var camera: Camera3D = $Focus/Camera3D

var _world_view: Node = null  # WorldViewManager (script or node)


func _ready() -> void:
	_init_sim_bridge()
	_init_world_streaming()
	_show_world_gen_menu()


func _show_world_gen_menu() -> void:
	var layer := CanvasLayer.new()
	layer.name = "UILayer"
	add_child(layer)
	var menu_scene := load("res://godot/ui/menus/worldGenMenu.tscn") as PackedScene
	if menu_scene:
		var menu := menu_scene.instantiate()
		layer.add_child(menu)


func _process(delta: float) -> void:
	_tick_sim(delta)
	_update_world_streaming()


func _init_sim_bridge() -> void:
	# Boot with path to data defs (core pack). Single door: SimAPI autoload.
	var defdb_path: String = ProjectSettings.globalize_path("res://data/core/")
	SimAPI.boot(defdb_path)


func _tick_sim(delta: float) -> void:
	SimAPI.step(delta, 1.0)


func _init_world_streaming() -> void:
	# WorldViewManager as script-only helper (C#).
	var script_res = load("res://godot/world/WorldViewManager.cs") as CSharpScript
	if script_res:
		var node = Node.new()
		node.set_script(script_res)
		add_child(node)
		_world_view = node
		if _world_view and _world_view.has_method("setup"):
			_world_view.setup(world_root, focus)
	if _world_view == null:
		push_warning("WorldViewManager not found; world streaming disabled.")


func _update_world_streaming() -> void:
	# When sphere preview is shown, don't stream chunk terrain (no flat MeshInstance3Ds).
	if get_node_or_null("SpherePreview") != null:
		return
	if _world_view == null or not _world_view.has_method("update_streaming"):
		return
	var focus_pos: Vector3 = focus.global_position
	_world_view.update_streaming(focus_pos)
