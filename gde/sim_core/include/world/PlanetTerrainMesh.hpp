#pragma once

#include "math/Vec3.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace growth {

struct PlanetGlobe;

/// Planet terrain mesh produced from a PlanetGlobe (render-ready). Used for world generation and
/// streaming the world map based on player position. Two generation methods:
/// - Voronoi triangles: each Voronoi cell split into tris (triangle centre, neighbour triangle centre, region centre).
/// - Quad (displaced sphere): Delaunay triangulation of Voronoi sites; one vertex per site, displaced radially by elevation; guaranteed connected.
struct PlanetTerrainMesh {
	std::vector<Vec3> vertices;
	std::vector<Vec3> normals;
	std::vector<uint32_t> indices;
	/// Per triangle (indices.size() / 3): river flow strength; high = river valley, 0 = ridge. Used for shading.
	std::vector<float> river_strength;
};

enum class PlanetTerrainMeshMethod {
	VoronoiTriangles,
	QuadMesh
};

/// Build planet terrain mesh using Voronoi-triangle method: each cell filled with tris (region centre, circumcenter i, circumcenter i+1).
void generate_planet_terrain_mesh_voronoi_triangles(const PlanetGlobe &globe, PlanetTerrainMesh &out);

/// Build planet terrain mesh as displaced sphere: Delaunay mesh of sites, vertices pushed/pulled by region elevation; river_strength per triangle from s_flow.
void generate_planet_terrain_mesh_quad(const PlanetGlobe &globe, PlanetTerrainMesh &out);

} // namespace growth
