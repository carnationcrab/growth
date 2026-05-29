#include "base/gateway/Cstdint.hpp"
#include "base/gateway/Cstring.hpp"
#pragma once


namespace growth {

namespace string_util {

	/// Deterministic 64-bit hash of a string (FNV-1a style).
	inline uint64_t hash(const String &s) {
		uint64_t h = 0xcbf29ce484222325ULL;
		for (unsigned char c : s) {
			h ^= static_cast<uint64_t>(c);
			h *= 0x100000001b3ULL;
		}
		return h;
	}

	/// Mix two 64-bit values into one (deterministic, for combining hashes).
	inline uint64_t mix_u64(uint64_t a, uint64_t b) {
		a ^= b;
		a *= 0xff51afd7ed558ccdULL;
		a ^= a >> 32;
		return a;
	}

} // namespace string_util

} // namespace growth
