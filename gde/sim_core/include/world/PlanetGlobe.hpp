#pragma once

#include "PlateProperties.hpp"
#include "RegionElevation.hpp"
#include "RegionMoisture.hpp"
#include "RiverFlow.hpp"
#include "SphereHalfEdgeMesh.hpp"
#include "TectonicPlates.hpp"
#include "TriangleValues.hpp"
#include "VoronoiSphere.hpp"

namespace growth {

/// Full globe result: topology, half-edge mesh, plates, plate properties, per-region and per-triangle values, rivers.
/// Produced by PlanetGlobeGenerator.
struct PlanetGlobe {
	VoronoiSphere voronoi;
	SphereHalfEdgeMesh mesh;
	TectonicPlates plates;
	PlateProperties plate_properties;
	RegionElevation region_elevation;
	RegionMoisture region_moisture;
	TriangleValues triangle_values;
	RiverFlow river_flow;
};

} // namespace growth
