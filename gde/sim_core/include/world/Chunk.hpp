#pragma once

#include "WorldCoord.hpp"
#include <vector>
#include <cstddef>

namespace growth {

constexpr int CHUNK_SIZE = 16;
constexpr int CHUNK_VERT_SIDE = CHUNK_SIZE + 1;
constexpr size_t CHUNK_HEIGHT_SAMPLES = static_cast<size_t>(CHUNK_VERT_SIDE) * CHUNK_VERT_SIDE;

class Chunk {
public:
	Chunk() { height_samples_.resize(CHUNK_HEIGHT_SAMPLES, 0.0f); }

	const float *height_data() const { return height_samples_.data(); }
	float *height_data() { return height_samples_.data(); }
	const std::vector<float> &height_samples() const { return height_samples_; }
	std::vector<float> &height_samples() { return height_samples_; }

	static size_t height_sample_count() { return CHUNK_HEIGHT_SAMPLES; }

private:
	std::vector<float> height_samples_;
};

} // namespace growth
