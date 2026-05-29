#pragma once
#include "Types.hpp"

#include "SphereTopology.hpp"
#include "../gen/WorldSeed.hpp"

namespace growth {

/// Builds scalable unit-sphere topology (geodesic icosphere + optional tangent jitter).
class SphereTopologyEngine {
public:
	/// target_regions: desired region count; actual vertices are the nearest icosphere size >= target.
	/// jitter_percent: 0–100 tangent displacement on the sphere.
	SphereTopology generate(const WorldSeed &world_seed, Size target_regions, double jitter_percent = 0.0) const;
};

} // namespace growth
