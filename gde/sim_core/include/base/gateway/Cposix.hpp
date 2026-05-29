#pragma once

#if !defined(_WIN32) && !defined(_WIN64)

#include "Cstddef.hpp"
#include "Cstdint.hpp"

// Sole include of POSIX I/O headers in sim_core.
#include <fcntl.h>
#include <unistd.h>

namespace growth {

struct Cposix final {
	Cposix() = delete;

	static int open_read_only(const char *path) { return open(path, O_RDONLY); }
	static ssize_t read(int fd, void *buf, Size count) { return ::read(fd, buf, count); }
	static int close(int fd) { return ::close(fd); }
};

} // namespace growth

#endif
