#include "util/Random.hpp"
#include "base/gateway/Cstdint.hpp"
#include "base/gateway/Cctime.hpp"
#include "base/gateway/Cwin.hpp"
#include "base/gateway/Cposix.hpp"

#if defined(_WIN32) || defined(_WIN64)
#pragma comment(lib, "advapi32.lib")
#else
#endif

namespace growth {

namespace random {

	void RNG::set_seed(uint64_t seed) {
		state_ = seed != 0 ? seed : 1u;
	}

	uint64_t RNG::next_u64() {
		state_ ^= state_ >> 12;
		state_ ^= state_ << 25;
		state_ ^= state_ >> 27;
		return state_ * 0x2545f4914f6cdd1dULL;
	}

	float RNG::next_float() {
		return static_cast<float>(next_u64() >> 40) / static_cast<float>(1ULL << 24);
	}

	double RNG::next_double() {
		return static_cast<double>(next_u64() >> 11) / static_cast<double>(1ULL << 53);
	}

	int64_t RNG::next_int(int64_t min_inclusive, int64_t max_inclusive) {
		uint64_t range = static_cast<uint64_t>(max_inclusive - min_inclusive + 1);
		if (range == 0) return min_inclusive;
		uint64_t r = next_u64() % range;
		return min_inclusive + static_cast<int64_t>(r);
	}

	uint64_t random_seed() {
#if defined(_WIN32) || defined(_WIN64)
		uint64_t out = 0;
		HCRYPTPROV prov = 0;
		if (CryptAcquireContextW(&prov, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) && prov) {
			if (CryptGenRandom(prov, sizeof(out), reinterpret_cast<BYTE *>(&out)))
				; // use out
			CryptReleaseContext(prov, 0);
		}
		if (out == 0) {
			// Fallback: tick count + stack address mixed
			out = static_cast<uint64_t>(GetTickCount64()) ^ (reinterpret_cast<uintptr_t>(&out) * 0x9e3779b97f4a7c15ULL);
		}
		return out;
#else
		uint64_t out = 0;
		int fd = open("/dev/urandom", O_RDONLY);
		if (fd >= 0) {
			ssize_t n = read(fd, &out, sizeof(out));
		if (n == static_cast<ssize_t>(sizeof(out))) {
				close(fd);
				return out;
			}
			close(fd);
		}
		// Fallback: time + stack (non-cryptographic)
		out = static_cast<uint64_t>(time(nullptr)) ^ (reinterpret_cast<uintptr_t>(&out) * 0x9e3779b97f4a7c15ULL);
		return out;
#endif
	}

} // namespace random

} // namespace growth
