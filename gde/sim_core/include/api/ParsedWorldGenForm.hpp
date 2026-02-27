#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace growth {

/// Parsed world-gen form inputs (platform-agnostic). Filled by a Godot or other adapter from UI/form data.
struct ParsedWorldGenForm {
	std::unordered_map<std::string, int64_t> seed_form;
	std::string planet_preset = "Earthlike";
	double temperature = 100.0;   /// 0–200 from sliders
	double precipitation = 100.0; /// 0–200 from sliders
	size_t voronoi_sites = 256;    /// Number of Fibonacci/Voronoi sites (slider; clamped in parser)
};

} // namespace growth
