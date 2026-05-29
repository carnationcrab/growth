#pragma once

#include "util/CsrGraph.hpp"
#include "world/SphereTopology.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

/// Laplacian neighbour average; softens piecewise-linear dual-mesh faceting after erosion.
void smooth_region_elevation_laplacian(const CsrGraph &region_neighbours, Vector<float> &region_elevation, int passes = 1);

/// Smooth region and triangle elevations on the dual mesh (region centres + circumcenters).
/// Reduces needle-like spikes when the terrain mesh folds region heights against triangle averages.
void smooth_dual_elevation_laplacian(const SphereTopology &topology,
                                     const CsrGraph &region_triangle_rings,
                                     Vector<float> &region_elevation,
                                     Vector<float> &triangle_elevation,
                                     int passes = 1);

/// Final elevation pass before dual-folded terrain mesh: slope limit + dual Laplacian smooth.
void prepare_elevation_for_terrain_mesh(const SphereTopology &topology,
                                        const CsrGraph &region_neighbours,
                                        const CsrGraph &region_triangle_rings,
                                        Vector<float> &region_elevation,
                                        Vector<float> &triangle_elevation);

} // namespace growth
