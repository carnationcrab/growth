extends Node3D
## One chunk of terrain. Holds a MeshInstance3D named Terrain; applies height grid from sim.

const CHUNK_SIZE_CELLS := 16
const CELL_SIZE := 1.0

var _chunk_coord: Vector2i
var _terrain: MeshInstance3D


func _ready() -> void:
	_terrain = get_node_or_null("Terrain") as MeshInstance3D
	if _terrain == null:
		push_warning("ChunkNode: Terrain MeshInstance3D not found.")


## height_samples: array of float, row-major (row * (size+1) + col). Grid (size+1)x(size+1).
## If empty or too small, builds a flat quad.
func apply_height_grid(height_samples: Array, chunk_coord: Vector2i) -> void:
	_chunk_coord = chunk_coord
	if _terrain == null:
		_terrain = get_node_or_null("Terrain") as MeshInstance3D
	if _terrain == null:
		return
	var size: int = CHUNK_SIZE_CELLS
	var side: int = size + 1
	var need: int = side * side
	var heights: PackedFloat32Array
	if height_samples.size() >= need:
		heights.resize(need)
		for i in range(need):
			heights[i] = float(height_samples[i])
	else:
		heights.resize(need)
		heights.fill(0.0)
	var mesh: ArrayMesh = _build_flat_mesh(heights, side)
	_terrain.mesh = mesh


func _build_flat_mesh(heights: PackedFloat32Array, side: int) -> ArrayMesh:
	var st: SurfaceTool = SurfaceTool.new()
	st.begin(Mesh.PRIMITIVE_TRIANGLES)
	var half: float = (side - 1) * 0.5 * CELL_SIZE
	var origin_x: float = _chunk_coord.x * CHUNK_SIZE_CELLS * CELL_SIZE
	var origin_z: float = _chunk_coord.y * CHUNK_SIZE_CELLS * CELL_SIZE
	for row in range(side - 1):
		for col in range(side - 1):
			var i00: int = row * side + col
			var i10: int = row * side + (col + 1)
			var i01: int = (row + 1) * side + col
			var i11: int = (row + 1) * side + (col + 1)
			var x0: float = origin_x + col * CELL_SIZE
			var z0: float = origin_z + row * CELL_SIZE
			var x1: float = origin_x + (col + 1) * CELL_SIZE
			var z1: float = origin_z + (row + 1) * CELL_SIZE
			var y00: float = heights[i00]
			var y10: float = heights[i10]
			var y01: float = heights[i01]
			var y11: float = heights[i11]
			var v00: Vector3 = Vector3(x0, y00, z0)
			var v10: Vector3 = Vector3(x1, y10, z0)
			var v01: Vector3 = Vector3(x0, y01, z1)
			var v11: Vector3 = Vector3(x1, y11, z1)
			var n: Vector3 = (v01 - v00).cross(v10 - v00).normalized()
			st.set_normal(n)
			st.add_vertex(v00)
			st.set_normal(n)
			st.add_vertex(v10)
			st.set_normal(n)
			st.add_vertex(v01)
			st.set_normal((v10 - v01).cross(v11 - v01).normalized())
			st.add_vertex(v10)
			st.set_normal((v10 - v01).cross(v11 - v01).normalized())
			st.add_vertex(v11)
			st.set_normal((v10 - v01).cross(v11 - v01).normalized())
			st.add_vertex(v01)
	st.generate_normals()
	return st.commit()
