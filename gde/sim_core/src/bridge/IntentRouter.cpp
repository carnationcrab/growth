#include "bridge/IntentRouter.hpp"
#include "bridge/SimBridge.hpp"
#include "base/gateway/Cutility.hpp"

namespace growth {

void IntentRouter::register_handler(const String &action_type, Handler handler) {
	handlers_[action_type] = Cutility::move(handler);
}

bool IntentRouter::apply(SimBridge &bridge, const DefDatabase &defs, const IntentRequest &request) {
	String action = request.action_type;
	if (!request.interaction_id.empty()) {
		const InteractionDef *interaction = defs.interaction(request.interaction_id);
		if (interaction != nullptr)
			action = interaction->action_type;
	}
	if (action.empty())
		return false;

	const auto it = handlers_.find(action);
	if (it == handlers_.end() || !it->second)
		return false;

	it->second(bridge, request);
	return true;
}

} // namespace growth
