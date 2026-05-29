#pragma once

#include "math/Vec3.hpp"
#include "base/gateway/Cstddef.hpp"
#include "base/gateway/Cstdint.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

/// Radial height-field scale for overworld shells: radius = |site_unit| * (1 + k * elevation).
/// Erosion and rivers mutate elevation scalars only; mesh export derives positions once.
constexpr float k_planet_elevation_scale = 0.08f;

struct PlanetGlobe;

/// Render-ready planet terrain mesh produced from a PlanetGlobe.
struct PlanetTerrainMesh {
	Vector<Vec3> vertices;
	Vector<Vec3> normals;
	Vector<uint32_t> indices;
	/// Per output triangle (indices.size() / 3): river-flow strength used for shading.
	Vector<float> river_strength;
	/// Per vertex (same length as vertices): map-layer elevation / moisture samples.
	Vector<float> vertex_elevation;
	Vector<float> vertex_moisture;
};

enum class PlanetTerrainMeshMethod {
	VoronoiTriangles,
	QuadMesh
};

/// Voronoi-triangle method (legacy): each Voronoi cell filled with a triangle fan.
/// Reads globe.topology + region_triangle_rings + mesh.t_xyz (legacy Voronoi cell fan).
void generate_planet_terrain_mesh_voronoi_triangles(const PlanetGlobe &globe, PlanetTerrainMesh &out);

/// Canonical preview/game shell: fixed icosphere connectivity; one vertex per region.
/// Positions are derived (not advected): pos = normalise(site) * (1 + k_planet_elevation_scale * elev).
/// Runs once after the scalar pipeline (elevation, flood, rivers, erosion); no per-stage mesh rebuild.
void generate_planet_terrain_mesh_quad(const PlanetGlobe &globe, PlanetTerrainMesh &out);

/// Dual half-edge mesh (RBG): region + triangle-centroid vertices. Emits one triangle per
/// directed half-edge (canonical s < twin only). Use quad mesh for the solid icosphere shell;
/// dual remains for experiments / folded coast variants.
void generate_planet_terrain_mesh_dual_folded(const PlanetGlobe &globe, PlanetTerrainMesh &out);

/// Per-triangle: swap winding when geometric normal points toward the origin (inward shell).
void ensure_planet_terrain_mesh_outward_winding(PlanetTerrainMesh &mesh);

/// Floating-islands mesh: each region emits its triangle-ring as a fan (using
/// globe.region_triangle_rings + globe.mesh.t_xyz), with elevations forced positive so the
/// mesh "floats" rather than dipping under sea level. Optionally fills triangle_region_out
/// with the source region id for each emitted triangle (size = indices.size() / 3).
void generate_planet_terrain_mesh_floating_islands(const PlanetGlobe &globe,
                                                   PlanetTerrainMesh &out,
                                                   Vector<uint32_t> *triangle_region_out = nullptr);

} // namespace growth
