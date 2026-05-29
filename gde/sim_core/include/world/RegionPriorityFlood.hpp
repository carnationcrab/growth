#pragma once

#include "util/CsrGraph.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

/// Barnes-style priority-flood on the region graph: ocean seeds, fill depressions, carve near-saddle rims.
void priority_flood_region_elevation(const CsrGraph &region_neighbours, Vector<float> &region_elevation);

} // namespace growth
