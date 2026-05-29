#pragma once

#include "../world/WorldCoord.hpp"
#include "base/gateway/Cvector.hpp"
#include "base/gateway/Cvariant.hpp"
#include "base/gateway/Cstdint.hpp"

namespace growth {

struct ChunkLoadedDiff {
	ChunkCoord coord;
	Vector<float> height_samples;  // (size+1)*(size+1), row-major
};

struct ChunkUnloadedDiff {
	ChunkCoord coord;
};

struct CellChangedDiff {
	ChunkCoord chunk_coord;
	int32_t local_x = 0;
	int32_t local_z = 0;
	uint8_t layer = 0;
	int32_t new_value = 0;
};

enum class DiffType { ChunkLoaded, ChunkUnloaded, CellChanged };

struct Diff {
	DiffType type;
	ChunkLoadedDiff chunk_loaded;
	ChunkUnloadedDiff chunk_unloaded;
	CellChangedDiff cell_changed;
};

} // namespace growth
