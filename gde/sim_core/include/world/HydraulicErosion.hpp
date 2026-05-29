#pragma once

#include "SphereHalfEdgeMesh.hpp"
#include "SphereTopology.hpp"
#include "RiverFlow.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

/// Red Blob Games / mapgen4-style hydraulic erosion on the dual mesh.
/// See https://github.com/redblobgames/1843-planet-generation (assignFlow) and mapgen4 river code.
/// Lowers downstream triangle elevation when flow moves from a higher tributary to a lower trunk.

/// For each triangle in reverse downflow order, if the trunk is higher than the tributary,
/// set trunk elevation to tributary elevation (valley cutting along drainage).
void apply_hydraulic_erosion(
	const SphereHalfEdgeMesh &mesh,
	const RiverFlow &flow,
	Vector<float> &triangle_elevation);

/// After triangle erosion, push heights back to region centres (mean of incident triangles).
void sync_region_elevation_from_triangles(
	const SphereTopology &topology,
	const Vector<float> &triangle_elevation,
	Vector<float> &region_elevation);

} // namespace growth
