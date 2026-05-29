#pragma once

#include "base/gateway/Cstddef.hpp"
#include "base/gateway/Cstdint.hpp"
#include "base/gateway/Cstring.hpp"
#include "base/gateway/Cunordered_map.hpp"

namespace growth {

/// Parsed world-gen form inputs (platform-agnostic). Filled by the Godot adapter from UI/form data.
struct ParsedWorldGenForm {
	UnorderedMap<String, int64_t> seed_form;
	String planet_preset = "Earthlike";
	double temperature   = 100.0;   ///< 0-200 slider value
	double precipitation = 100.0;   ///< 0-200 slider value
	size_t voronoi_sites      = 16384; ///< total fine-grid sites (interactive default; slider goes much higher)
	size_t sim_voronoi_sites  = 0;    ///< when > 0 and < voronoi_sites, run coarse sim then upsample
	double jitter             = 0.0;  ///< 0-100 percent tangent jitter applied to icosphere vertices
	size_t num_plate_regions  = 32;   ///< tectonic plate count (RBG typical 10–50)
	bool use_planet_terrain_mesh = true; ///< build the renderable terrain mesh

	/// Runtime tuning: parallel execution profile and SIMD opt-in. "max_determinism" forces a
	/// single worker thread so output is bit-identical across machines; default is "max_performance".
	String world_gen_mode = "max_performance";
	bool use_simd = true;
};

} // namespace growth
