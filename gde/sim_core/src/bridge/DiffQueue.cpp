#include "bridge/DiffQueue.hpp"
#include "base/gateway/Cutility.hpp"

namespace growth {

void DiffQueue::push(Diff d) {
	queue_.push_back(Cutility::move(d));
}

bool DiffQueue::pop(Diff &out) {
	if (queue_.empty()) return false;
	out = Cutility::move(queue_.front());
	queue_.pop_front();
	return true;
}

size_t DiffQueue::size() const {
	return queue_.size();
}

void DiffQueue::clear() {
	queue_.clear();
}

} // namespace growth
