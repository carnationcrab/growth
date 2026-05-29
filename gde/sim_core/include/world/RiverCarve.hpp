#pragma once

#include "PlanetGlobe.hpp"

namespace growth {

/// Lowers region elevation along high-flow sides and refreshes triangle min-elevations.
void carve_rivers_from_flow(PlanetGlobe &globe);

} // namespace growth
