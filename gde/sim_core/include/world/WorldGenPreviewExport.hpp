#pragma once
#include "Types.hpp"

#include "PlanetGlobe.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

/// Limits heavy Godot preview payloads (nested cell arrays) for large globes.
struct WorldGenPreviewExport {
	/// Max regions for nested cell rings and legacy triangles Array export (matches Godot defer threshold).
	static constexpr Size k_max_regions_with_cell_rings = 65536;
	/// Max regions for half-edge / river debug arrays in Godot dict.
	static constexpr Size k_max_regions_with_debug_mesh = 65536;

	static bool include_cell_rings(Size num_regions);
	static bool include_debug_mesh(Size num_regions);

	/// Triangle centroids for preview (same order as mesh triangles).
	static void triangle_centroids_for_preview(const PlanetGlobe &globe, Vector<Vec3> &out);

	/// Per-region triangle index rings; only call when include_cell_rings is true.
	static void region_cell_rings_for_preview(const PlanetGlobe &globe, Vector<Vector<Size>> &out);

	/// Displaced dual-mesh river segments (pairs of points in sim Z-up unit-sphere space).
	static void river_lines_for_preview(const PlanetGlobe &globe, Vector<Vec3> &out_segment_endpoints);
};

} // namespace growth
