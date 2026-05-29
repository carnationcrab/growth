#pragma once

#if defined(_WIN32) || defined(_WIN64)

#include "Cstddef.hpp"
#include "Cstdint.hpp"

// Sole include of Windows crypto/timing headers in sim_core.
#include <windows.h>
#include <wincrypt.h>

namespace growth {

struct Cwin final {
	Cwin() = delete;

	using CryptProv = HCRYPTPROV;
	using Byte      = BYTE;

	static bool crypt_acquire_context(CryptProv *prov) {
		return CryptAcquireContextW(prov, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) != 0;
	}

	static bool crypt_gen_random(CryptProv prov, std::size_t count, Byte *buffer) {
		return CryptGenRandom(prov, static_cast<DWORD>(count), buffer) != 0;
	}

	static void crypt_release_context(CryptProv prov) { CryptReleaseContext(prov, 0); }

	static Cstdint::U64 tick_count64() { return static_cast<Cstdint::U64>(GetTickCount64()); }
};

} // namespace growth

#endif
