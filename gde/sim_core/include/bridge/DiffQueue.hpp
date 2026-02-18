#pragma once

#include "Diff.hpp"
#include <deque>
#include <cstddef>

namespace growth {

class DiffQueue {
public:
	void push(Diff d);
	bool pop(Diff &out);
	size_t size() const;
	void clear();

private:
	std::deque<Diff> queue_;
};

} // namespace growth
