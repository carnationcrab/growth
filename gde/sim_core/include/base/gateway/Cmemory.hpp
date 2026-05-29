#pragma once

// Sole include of <memory> in sim_core.
#include <memory>

namespace growth {

struct Cmemory final {
	Cmemory() = delete;

	template <typename T>
	using UniquePtr = std::unique_ptr<T>;

	template <typename T>
	using SharedPtr = std::shared_ptr<T>;

	template <typename T>
	using WeakPtr = std::weak_ptr<T>;

	template <typename T, typename... Args>
	static UniquePtr<T> make_unique(Args &&...args) {
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	static SharedPtr<T> make_shared(Args &&...args) {
		return std::make_shared<T>(std::forward<Args>(args)...);
	}
};

template <typename T>
using UniquePtr = Cmemory::UniquePtr<T>;

template <typename T>
using SharedPtr = Cmemory::SharedPtr<T>;

} // namespace growth
