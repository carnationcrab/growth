#include "base/gateway/Cstddef.hpp"
#include "base/gateway/Ccstdlib.hpp"
#pragma once


namespace growth {

namespace memory {

	/// Allocate uninitialised bytes. Returns nullptr on failure.
	inline void *allocate_bytes(Size size) {
		if (size == 0) return nullptr;
		return Ccstdlib::malloc(size);
	}

	/// Deallocate memory from allocate_bytes. No-op if ptr is nullptr.
	inline void deallocate_bytes(void *ptr, Size /* size */) {
		if (ptr) Ccstdlib::free(ptr);
	}

	/// Owning buffer: ptr + size, frees on destroy.
	struct Buffer {
		void *data = nullptr;
		Size size = 0;

		Buffer() = default;
		Buffer(void *data_, Size size_) : data(data_), size(size_) {}
		~Buffer() {
			if (data) {
				deallocate_bytes(data, size);
				data = nullptr;
				size = 0;
			}
		}

		Buffer(Buffer &&o) noexcept : data(o.data), size(o.size) { o.data = nullptr; o.size = 0; }
		Buffer &operator=(Buffer &&o) noexcept {
			if (data) deallocate_bytes(data, size);
			data = o.data; size = o.size;
			o.data = nullptr; o.size = 0;
			return *this;
		}

		Buffer(const Buffer &) = delete;
		Buffer &operator=(const Buffer &) = delete;
	};

} // namespace memory

} // namespace growth
