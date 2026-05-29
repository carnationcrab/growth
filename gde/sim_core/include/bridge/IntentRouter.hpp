#pragma once

#include "../data/DefDatabase.hpp"
#include "../world/WorldCoord.hpp"
#include "base/gateway/Cfunctional.hpp"
#include "base/gateway/Cstring.hpp"
#include "base/gateway/Cunordered_map.hpp"

namespace growth {

class SimBridge;

struct IntentRequest {
	String action_type;
	String interaction_id;
	ChunkCoord chunk{};
	int local_x = 0;
	int local_z = 0;
};

/// Routes player intents to registered handlers using DefDatabase for interaction defs.
class IntentRouter {
public:
	using Handler = Function<void(SimBridge &, const IntentRequest &)>;

	void register_handler(const String &action_type, Handler handler);
	bool apply(SimBridge &bridge, const DefDatabase &defs, const IntentRequest &request);

private:
	UnorderedMap<String, Handler> handlers_;
};

} // namespace growth
