#pragma once

#include "Diff.hpp"
#include "base/gateway/Cdeque.hpp"
#include "base/gateway/Cstddef.hpp"

namespace growth {

class DiffQueue {
public:
	void push(Diff d);
	bool pop(Diff &out);
	size_t size() const;
	void clear();

private:
	Deque<Diff> queue_;
};

} // namespace growth
