#pragma once

#include "Cios.hpp"
#include "Cstring.hpp"

// Sole include of <fstream> in sim_core.
#include <fstream>

namespace growth {

struct Cfstream final {
	Cfstream() = delete;

	using Ifstream = std::ifstream;
	using Ofstream = std::ofstream;

	static Ifstream open_input(const String &path, std::ios::openmode mode = std::ios::in) {
		return Ifstream(path, mode);
	}
};

using Ifstream = Cfstream::Ifstream;
using Ofstream = Cfstream::Ofstream;

} // namespace growth
