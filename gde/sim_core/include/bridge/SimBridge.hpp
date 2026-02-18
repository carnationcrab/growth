#pragma once

#include "DiffQueue.hpp"
#include "../world/WorldCoord.hpp"
#include "../world/ChunkStore.hpp"
#include "../Types.hpp"
#include <string>

namespace growth {

class SimBridge {
public:
	void boot(Seed seed, const std::string &defs_path);
	void step(double dt);
	void request_chunks(ChunkCoord center, int radius);
	bool poll_diff(Diff &out);
	void apply_intent_dig(ChunkCoord chunk, int local_x, int local_z);

private:
	Seed seed_ = 0;
	std::string defs_path_;
	DiffQueue diff_queue_;
	ChunkStore chunk_store_;
};

} // namespace growth
