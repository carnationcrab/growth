#pragma once

#include "Cstring.hpp"

// Sole include of <sstream> in sim_core.
#include <sstream>

namespace growth {

struct Csstream final {
	Csstream() = delete;

	using StringStream      = std::stringstream;
	using OStringStream     = std::ostringstream;
	using IStringStream     = std::istringstream;
};

using StringStream  = Csstream::StringStream;
using OStringStream = Csstream::OStringStream;
using IStringStream = Csstream::IStringStream;

inline bool getline(IStringStream &stream, String &line) {
	return static_cast<bool>(std::getline(stream, line));
}

} // namespace growth
