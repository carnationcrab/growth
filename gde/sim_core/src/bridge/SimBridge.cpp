#include "bridge/SimBridge.hpp"
#include "base/gateway/Cutility.hpp"
#include "world/ChunkStore.hpp"

namespace growth {

void SimBridge::boot(WorldSeed world_seed, const String &defs_path) {
	world_seed_ = world_seed;
	defs_path_  = defs_path;
	overworld_ = nullopt;
	overworld_play_committed_ = false;
	diff_queue_.clear();
	defs_.load(defs_path, "default");
}

void SimBridge::set_world_gen(WorldSeed world_seed, const PlanetGenome &planet_genome) {
	world_seed_ = world_seed;
	planet_genome_ = planet_genome;
}

void SimBridge::set_overworld_surface(PlanetSurfaceAtlas atlas) {
	overworld_ = Cutility::move(atlas);
	overworld_play_committed_ = false;
}

bool SimBridge::has_overworld() const {
	return overworld_.has_value();
}

const PlanetSurfaceAtlas &SimBridge::overworld() const {
	return overworld_.value();
}

void SimBridge::commit_overworld_for_play() {
	if (overworld_.has_value())
		overworld_play_committed_ = true;
}

SurfaceSample SimBridge::sample_surface(Vec3 unit_dir) const {
	if (!overworld_.has_value())
		return SurfaceSample{};
	return OverworldSurfaceSampler::sample_unit_dir(overworld_.value(), unit_dir);
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
	(void)chunk;
	(void)local_x;
	(void)local_z;
}

} // namespace growth
