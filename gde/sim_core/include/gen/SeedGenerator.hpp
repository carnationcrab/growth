#pragma once

#include "WorldSeed.hpp"
#include <unordered_map>
#include <string>

namespace growth {

/// Produces a WorldSeed (and params) from user input. Reusable for form submission or any "world params from user input" path.
class SeedGenerator {
public:
	/// Build world params from a raw user seed (e.g. "randomise" or form seed field). Uses default scales/octaves/frequency.
	WorldSeed from_seed(uint64_t user_seed) const;

	/// Build world params from form answers. Keys and values are combined deterministically into the seed; known keys set scale/octaves/frequency.
	WorldSeed from_form(const std::unordered_map<std::string, int64_t> &answers) const;
};

} // namespace growth
