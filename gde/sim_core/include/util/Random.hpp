#pragma once

#include <cstdint>
#include <cstddef>

namespace growth {

namespace random {

	/// Deterministic RNG (xorshift64*). Same seed gives same sequence.
	class RNG {
	public:
		explicit RNG(uint64_t seed = 0) { set_seed(seed); }

		void set_seed(uint64_t seed);

		uint64_t next_u64();
		float next_float();   // [0.f, 1.f)
		double next_double(); // [0.0, 1.0)
		int64_t next_int(int64_t min_inclusive, int64_t max_inclusive);

	private:
		uint64_t state_ = 0;
	};

	/// Non-deterministic: obtain a random seed (e.g. when user leaves seed field empty).
	uint64_t random_seed();

} // namespace random

} // namespace growth
