#pragma once

#include "SphereHalfEdgeMesh.hpp"
#include <cstddef>
#include <vector>

namespace growth {

/// River flow data (mapgen4-style): per-triangle downflow side, per-edge flow strength.
/// assign_downflow: each triangle finds the neighbour with lower elevation; t_downflow_s stores the half-edge water flows through.
/// assign_flow: process triangles high to low; water accumulates; s_flow stores river strength per edge.
struct RiverFlow {
	/// Per triangle: half-edge index through which water flows to the lower neighbour, or k_invalid if local minimum.
	std::vector<size_t> t_downflow_s;
	/// Per half-edge: accumulated flow strength (river strength); large values become rivers.
	std::vector<float> s_flow;

	static constexpr size_t k_invalid = SphereHalfEdgeMesh::k_invalid;
};

/// Step 1 — Determine flow direction. Each triangle finds the neighbour with lower elevation.
void assign_downflow(
	const SphereHalfEdgeMesh &mesh,
	const std::vector<float> &triangle_elevation,
	RiverFlow &out
);

/// Step 2 — Flow accumulation. Process triangles from high to low; flow[downstream] += flow[current].
/// Edges with large flow become rivers. triangle_elevation used for sort order (high to low).
void assign_flow(
	const SphereHalfEdgeMesh &mesh,
	const RiverFlow &downflow,
	const std::vector<float> &triangle_elevation,
	RiverFlow &out
);

} // namespace growth
