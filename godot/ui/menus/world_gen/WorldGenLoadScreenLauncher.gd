class_name WorldGenLoadScreenLauncher
extends RefCounted
## Instantiates the world-gen load screen and starts async generation.


static func start(ui_layer: Node, form_dict: Dictionary, on_finished: Callable) -> void:
	var load_path: String = SimAPI.get_presentation_scene_path("world_gen_load_screen")
	if load_path.is_empty():
		push_error("[WorldGen] no world_gen_load_screen in game profile")
		return
	var load_screen_scene := load(load_path) as PackedScene
	if not load_screen_scene:
		push_error("[WorldGen] failed to load load screen: ", load_path)
		return
	var load_screen: Node = load_screen_scene.instantiate()
	ui_layer.add_child(load_screen)
	load_screen.generation_finished.connect(on_finished)
	load_screen.run_generation(form_dict)
