#pragma once

// Sole include of <algorithm> in sim_core.
#include <algorithm>

namespace growth {

struct Calgorithm final {
	Calgorithm() = delete;

	template <typename... Args>
	static void sort(Args &&...args) {
		std::sort(std::forward<Args>(args)...);
	}

	template <typename... Args>
	static auto min_element(Args &&...args) {
		return std::min_element(std::forward<Args>(args)...);
	}

	template <typename... Args>
	static auto max_element(Args &&...args) {
		return std::max_element(std::forward<Args>(args)...);
	}

	template <typename... Args>
	static void transform(Args &&...args) {
		std::transform(std::forward<Args>(args)...);
	}

	template <typename... Args>
	static auto lower_bound(Args &&...args) {
		return std::lower_bound(std::forward<Args>(args)...);
	}

	template <typename... Args>
	static auto upper_bound(Args &&...args) {
		return std::upper_bound(std::forward<Args>(args)...);
	}

	template <typename... Args>
	static void stable_sort(Args &&...args) {
		std::stable_sort(std::forward<Args>(args)...);
	}

	template <typename... Args>
	static void fill(Args &&...args) {
		std::fill(std::forward<Args>(args)...);
	}

	template <typename... Args>
	static void copy(Args &&...args) {
		std::copy(std::forward<Args>(args)...);
	}

	template <typename... Args>
	static auto unique(Args &&...args) {
		return std::unique(std::forward<Args>(args)...);
	}

	template <typename... Args>
	static auto min(Args &&...args) {
		return std::min(std::forward<Args>(args)...);
	}

	template <typename... Args>
	static auto max(Args &&...args) {
		return std::max(std::forward<Args>(args)...);
	}

	template <typename... Args>
	static auto clamp(Args &&...args) {
		return std::clamp(std::forward<Args>(args)...);
	}

	template <typename... Args>
	static void nth_element(Args &&...args) {
		std::nth_element(std::forward<Args>(args)...);
	}
};

} // namespace growth
