#include "world/SphereTopologyEngine.hpp"
#include "world/IcosphereEngine.hpp"
#include "util/Random.hpp"
#include "Types.hpp"
#include "base/gateway/Cmath.hpp"

namespace growth {

namespace {

	const double k_max_jitter_radius = 0.25;

	void apply_vertex_jitter(Vector<Vec3> &sites, U64 seed, double jitter_percent) {
		if (jitter_percent <= 0.0 || sites.empty()) return;
		const double scale = (jitter_percent / 100.0) * k_max_jitter_radius;
		random::RNG rng(seed);
		for (Vec3 &p : sites) {
			Vec3 axis(0.0, 0.0, 1.0);
			if (Cmath::abs(p.z) > 0.9) axis = Vec3(1.0, 0.0, 0.0);
			Vec3 u = p.cross(axis);
			if (u.length() < 1e-6) continue;
			u           = u.normalised();
			Vec3 v      = p.cross(u).normalised();
			const double r1 = rng.next_double() * 2.0 - 1.0;
			const double r2 = rng.next_double() * 2.0 - 1.0;
			Vec3 offset = u * (r1 * scale) + v * (r2 * scale);
			p           = (p + offset).normalised();
		}
	}

} // namespace

SphereTopology SphereTopologyEngine::generate(const WorldSeed &world_seed, Size target_regions, double jitter_percent) const {
	SphereTopology out;
	const int subdiv = IcosphereEngine::subdivision_for_target(target_regions);
	IcosphereEngine::build(subdiv, out);
	if (jitter_percent > 0.0)
		apply_vertex_jitter(out.sites, world_seed.value, jitter_percent);
	return out;
}

} // namespace growth
