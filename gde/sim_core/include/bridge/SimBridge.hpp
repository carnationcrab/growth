#pragma once

#include "DiffQueue.hpp"
#include "../data/DefDatabase.hpp"
#include "../world/WorldCoord.hpp"
#include "../world/ChunkStore.hpp"
#include "../world/PlanetSurfaceAtlas.hpp"
#include "../world/OverworldSurfaceSampler.hpp"
#include "../gen/WorldSeed.hpp"
#include "../gen/PlanetGenome.hpp"
#include "base/gateway/Coptional.hpp"
#include "base/gateway/Cstring.hpp"

namespace growth {

class SimBridge {
public:
	void boot(WorldSeed world_seed, const String &defs_path);
	/// Update world seed and planet genome (e.g. after user submits world gen form).
	void set_world_gen(WorldSeed world_seed, const PlanetGenome &planet_genome);
	void set_overworld_surface(PlanetSurfaceAtlas atlas);
	bool has_overworld() const;
	bool overworld_play_committed() const { return overworld_play_committed_; }
	const PlanetSurfaceAtlas &overworld() const;
	void commit_overworld_for_play();
	SurfaceSample sample_surface(Vec3 unit_dir) const;
	void step(double dt);
	void request_chunks(ChunkCoord center, int radius);
	bool poll_diff(Diff &out);
	void apply_intent_dig(ChunkCoord chunk, int local_x, int local_z);

	const WorldSeed &world_seed() const { return world_seed_; }
	const PlanetGenome &planet_genome() const { return planet_genome_; }
	const DefDatabase &defs() const { return defs_; }
	const String &defs_path() const { return defs_path_; }

private:
	WorldSeed world_seed_;
	PlanetGenome planet_genome_;
	Coptional::Optional<PlanetSurfaceAtlas> overworld_;
	bool overworld_play_committed_ = false;
	DefDatabase defs_;
	String defs_path_;
	DiffQueue diff_queue_;
	ChunkStore chunk_store_;
};

} // namespace growth
