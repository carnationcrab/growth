#pragma once

#include <cstdint>

namespace growth {

struct ChunkCoord {
	int32_t x = 0;
	int32_t z = 0;

	bool operator==(const ChunkCoord &o) const { return x == o.x && z == o.z; }
	bool operator<(const ChunkCoord &o) const {
		if (x != o.x) return x < o.x;
		return z < o.z;
	}
};

struct CellCoord {
	int32_t x = 0;
	int32_t z = 0;
};

} // namespace growth
