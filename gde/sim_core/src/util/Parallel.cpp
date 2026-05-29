#include "util/Parallel.hpp"

namespace growth {
namespace parallel {

namespace {
	RuntimeConfig g_config{};
}

void configure(const RuntimeConfig &config) {
	g_config = config;
}

RuntimeConfig current_config() {
	return g_config;
}

} // namespace parallel
} // namespace growth
