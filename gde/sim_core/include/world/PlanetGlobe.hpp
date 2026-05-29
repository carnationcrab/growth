#pragma once

#include "PlateProperties.hpp"
#include "RegionElevation.hpp"
#include "RegionMoisture.hpp"
#include "RiverFlow.hpp"
#include "SphereHalfEdgeMesh.hpp"
#include "SphereTopology.hpp"
#include "TectonicPlates.hpp"
#include "TriangleValues.hpp"
#include "../util/CsrGraph.hpp"

namespace growth {

/// Full globe result that flows through every world-gen stage.
///
/// Height-field model: topology.sites are fixed directions (optional tangent jitter once);
/// elevation/moisture/plates are scalars on regions. Rivers use triangle_values on the dual mesh;
/// optional PlanetTerrainMesh derives 3D positions radially at export — not between stages.
///
/// Canonical fields (post-refactor):
///   topology              - unit-sphere sites + icosphere triangles
///   mesh                  - half-edge structure derived from topology
///   region_neighbours     - CSR adjacency over regions; built once, reused by every stage
///   region_triangle_rings - CSR mapping each region to its surrounding triangle indices
///                           (the "Voronoi cell" — replaces the old VoronoiSphere::cells)
///
struct PlanetGlobe {
	SphereTopology topology;
	SphereHalfEdgeMesh mesh;
	CsrGraph region_neighbours;
	CsrGraph region_triangle_rings;
	TectonicPlates plates;
	PlateProperties plate_properties;
	RegionElevation region_elevation;
	RegionElevation region_elevation_pre_erosion;
	RegionMoisture region_moisture;
	TriangleValues triangle_values;
	RiverFlow river_flow;
};

} // namespace growth
