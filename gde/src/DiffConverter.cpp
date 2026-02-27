#include "DiffConverter.hpp"
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/vector2i.hpp>

namespace godot {

namespace growth_sim {

Dictionary diff_to_dictionary(const growth::Diff &d) {
	Dictionary dict;
	if (d.type == growth::DiffType::ChunkLoaded) {
		dict[Variant(String("type"))] = String("ChunkLoaded");
		dict[Variant(String("coord"))] = Vector2i(d.chunk_loaded.coord.x, d.chunk_loaded.coord.z);
		PackedFloat32Array arr;
		arr.resize((int)d.chunk_loaded.height_samples.size());
		for (size_t i = 0; i < d.chunk_loaded.height_samples.size(); ++i)
			arr[(int)i] = d.chunk_loaded.height_samples[i];
		dict[Variant(String("height_samples"))] = arr;
	} else if (d.type == growth::DiffType::ChunkUnloaded) {
		dict[Variant(String("type"))] = String("ChunkUnloaded");
		dict[Variant(String("coord"))] = Vector2i(d.chunk_unloaded.coord.x, d.chunk_unloaded.coord.z);
	} else if (d.type == growth::DiffType::CellChanged) {
		dict[Variant(String("type"))] = String("CellChanged");
		dict[Variant(String("chunk_coord"))] = Vector2i(d.cell_changed.chunk_coord.x, d.cell_changed.chunk_coord.z);
		dict[Variant(String("local_xz"))] = Vector2i(d.cell_changed.local_x, d.cell_changed.local_z);
		dict[Variant(String("layer"))] = d.cell_changed.layer;
		dict[Variant(String("new_value"))] = d.cell_changed.new_value;
	}
	return dict;
}

} // namespace growth_sim

} // namespace godot
