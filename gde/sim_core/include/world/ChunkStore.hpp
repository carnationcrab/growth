#pragma once

#include "Chunk.hpp"
#include "WorldCoord.hpp"
#include "../bridge/Diff.hpp"
#include "../bridge/DiffQueue.hpp"
#include "base/gateway/Cunordered_map.hpp"
#include "base/gateway/Cstring.hpp"

namespace growth {

class ChunkStore {
public:
	void request_chunks(ChunkCoord center, int radius, DiffQueue &out_diffs);
	void unload_chunk(ChunkCoord coord, DiffQueue &out_diffs);
	bool is_loaded(ChunkCoord coord) const;

private:
	Chunk &get_or_create(ChunkCoord coord);

	UnorderedMap<int64_t, Chunk> chunks_;

	static int64_t key(ChunkCoord c) {
		return (static_cast<int64_t>(c.x) << 32) | (static_cast<uint32_t>(c.z));
	}
};

} // namespace growth
