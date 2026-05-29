#pragma once

// Sole include of <string> in sim_core.
#include <string>

namespace growth {

struct Cstring final {
	Cstring() = delete;

	using String = std::string;

	static double stod(const String &s) { return std::stod(s); }

	template <typename T>
	static String to_string(T value) {
		return std::to_string(value);
	}

	static constexpr std::string::size_type npos = std::string::npos;
};

using String = Cstring::String;

} // namespace growth
