#include "bridge/SimBridge.hpp"
#include "world/ChunkStore.hpp"

namespace growth {

void SimBridge::boot(Seed seed, const std::string &defs_path) {
	seed_ = seed;
	defs_path_ = defs_path;
	diff_queue_.clear();
	// DefDatabase load from defs_path can be added later
}

void SimBridge::step(double /* dt */) {
	// TimeSystem / ecology / hydrology tick can be added later
}

void SimBridge::request_chunks(ChunkCoord center, int radius) {
	chunk_store_.request_chunks(center, radius, diff_queue_);
}

bool SimBridge::poll_diff(Diff &out) {
	return diff_queue_.pop(out);
}

void SimBridge::apply_intent_dig(ChunkCoord chunk, int local_x, int local_z) {
	// For now: optionally push CellChanged or just mutate chunk height in ChunkStore.
	// Minimal: no-op or enqueue CellChanged for surface layer.
	(void)chunk;
	(void)local_x;
	(void)local_z;
}

} // namespace growth
