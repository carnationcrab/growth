#pragma once

#include "SphereHalfEdgeMesh.hpp"
#include "base/gateway/Cstddef.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

/// River flow data (mapgen4 / Red Blob Games 1843-planet-generation style).
struct RiverFlow {
	/// Per triangle: half-edge index water flows through toward lower elevation.
	Vector<size_t> t_downflow_s;
	/// Visit order from assign_downflow (ocean seeds, then elevation priority queue).
	Vector<size_t> order_t;
	/// Per half-edge: accumulated flow strength (river strength).
	Vector<float> s_flow;

	static constexpr size_t k_invalid = SphereHalfEdgeMesh::k_invalid;
};

/// Step 1 — Downflow and visit order (RBG assignDownflow): ocean triangles first, then land by elevation.
void assign_downflow(
	const SphereHalfEdgeMesh &mesh,
	const Vector<float> &triangle_elevation,
	RiverFlow &out);

/// Step 2 — Moisture-driven flow accumulation (RBG assignFlow, without elevation change).
/// Fills s_flow; use apply_hydraulic_erosion for the elevation carve pass.
void assign_flow(
	const SphereHalfEdgeMesh &mesh,
	const RiverFlow &downflow,
	const Vector<float> &triangle_elevation,
	const Vector<float> &triangle_moisture,
	RiverFlow &out);

/// 90th-percentile of positive side flows; used for river carve and preview line export.
float river_flow_visual_threshold(const Vector<float> &s_flow);

} // namespace growth
