#include "world/ChunkStore.hpp"
#include "base/gateway/Cmath.hpp"
#include "base/gateway/Calgorithm.hpp"
#include "base/gateway/Cutility.hpp"

namespace growth {

Chunk &ChunkStore::get_or_create(ChunkCoord coord) {
	int64_t k = key(coord);
	auto it = chunks_.find(k);
	if (it != chunks_.end()) return it->second;
	Chunk &ch = chunks_[k];
	return ch;
}

void ChunkStore::request_chunks(ChunkCoord center, int radius, DiffQueue &out_diffs) {
	for (int ox = -radius; ox <= radius; ++ox) {
		for (int oz = -radius; oz <= radius; ++oz) {
			ChunkCoord c{ center.x + ox, center.z + oz };
			if (is_loaded(c)) continue;
			Chunk &ch = get_or_create(c);
			Diff d;
			d.type = DiffType::ChunkLoaded;
			d.chunk_loaded.coord = c;
			d.chunk_loaded.height_samples = ch.height_samples();
			out_diffs.push(Cutility::move(d));
		}
	}
}

void ChunkStore::unload_chunk(ChunkCoord coord, DiffQueue &out_diffs) {
	int64_t k = key(coord);
	auto it = chunks_.find(k);
	if (it == chunks_.end()) return;
	chunks_.erase(it);
	Diff d;
	d.type = DiffType::ChunkUnloaded;
	d.chunk_unloaded.coord = coord;
	out_diffs.push(Cutility::move(d));
}

bool ChunkStore::is_loaded(ChunkCoord coord) const {
	return chunks_.find(key(coord)) != chunks_.end();
}

} // namespace growth
