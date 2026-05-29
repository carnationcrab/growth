class_name WorldGenPreviewApplier
extends RefCounted
## Applies a world-gen result to SpherePreview via SimAPI (C# WorldGenPreviewApplierEngine).
## GDScript cannot call static C# classes directly; SimAPI is the facade.


static func apply(main: Node, result: Dictionary) -> void:
	SimAPI.apply_world_gen_preview(main, result)


static func apply_heavy(preview: Node, result: Dictionary) -> void:
	SimAPI.apply_world_gen_preview_heavy(preview, result)
