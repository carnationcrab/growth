#include "world/PlanetTerrainMesh.hpp"
#include "world/PlanetGlobe.hpp"
#include "util/Parallel.hpp"
#include "base/gateway/Cstddef.hpp"
#include "base/gateway/Cstdint.hpp"
#include "base/gateway/Cvector.hpp"
namespace growth {

namespace {

Vec3 normalised_vec3(const Vec3 &v) {
	const float l = v.length();
	return (l > 0.0f) ? (v / l) : Vec3(0.0f, 0.0f, 0.0f);
}

constexpr float k_elevation_scale = 0.08f;

/// River strength per Delaunay triangle: mean of the three half-edge flows. Used by both the
/// dual-folded and quad meshes.
float triangle_river_strength(const RiverFlow &river, size_t t) {
	float sum = 0.0f;
	for (int e = 0; e < 3; ++e) {
		const size_t s = t * 3u + static_cast<size_t>(e);
		if (s < river.s_flow.size())
			sum += river.s_flow[s];
	}
	return sum * (1.0f / 3.0f);
}

} // namespace

void generate_planet_terrain_mesh_voronoi_triangles(const PlanetGlobe &globe, PlanetTerrainMesh &out) {
	out.vertices.clear();
	out.normals.clear();
	out.indices.clear();
	out.river_strength.clear();

	const SphereTopology &topology = globe.topology;
	const CsrGraph &rings          = globe.region_triangle_rings;
	const Vector<Vec3> &t_xyz      = globe.mesh.t_xyz;
	const size_t num_regions       = topology.sites.size();
	const size_t num_tri           = t_xyz.size();
	if (num_regions == 0 || num_tri == 0 || rings.num_nodes() != num_regions)
		return;

	out.vertices.reserve(num_regions + num_tri);
	out.normals.reserve(num_regions + num_tri);
	for (size_t i = 0; i < num_regions; ++i) {
		const Vec3 p = topology.sites[i];
		out.vertices.push_back(p);
		out.normals.push_back(normalised_vec3(p));
	}
	for (size_t i = 0; i < num_tri; ++i) {
		const Vec3 p = t_xyz[i];
		out.vertices.push_back(p);
		out.normals.push_back(normalised_vec3(p));
	}

	for (size_t r = 0; r < num_regions; ++r) {
		const U32 *cb = rings.neighbours_begin(r);
		const U32 *ce = rings.neighbours_end(r);
		Vector<size_t> cell;
		for (const U32 *it = cb; it != ce; ++it)
			cell.push_back(static_cast<size_t>(*it));
		if (cell.size() < 2)
			continue;
		for (size_t j = 0; j < cell.size(); ++j) {
			const size_t c0 = cell[j];
			const size_t c1 = cell[(j + 1) % cell.size()];
			if (c0 >= num_tri || c1 >= num_tri)
				continue;
			out.indices.push_back(static_cast<uint32_t>(r));
			out.indices.push_back(static_cast<uint32_t>(num_regions + c0));
			out.indices.push_back(static_cast<uint32_t>(num_regions + c1));
			out.river_strength.push_back(0.0f);
		}
	}
}

void generate_planet_terrain_mesh_quad(const PlanetGlobe &globe, PlanetTerrainMesh &out) {
	out.vertices.clear();
	out.normals.clear();
	out.indices.clear();
	out.river_strength.clear();

	const SphereTopology &topology   = globe.topology;
	const Vector<float> &r_elevation = globe.region_elevation.region_elevation;
	const RiverFlow &river           = globe.river_flow;
	const size_t num_regions         = topology.sites.size();
	const size_t num_tri             = topology.triangles.size();
	if (num_regions == 0 || num_tri == 0)
		return;

	out.vertices.reserve(num_regions);
	out.normals.reserve(num_regions);
	for (size_t r = 0; r < num_regions; ++r) {
		const Vec3 dir   = normalised_vec3(topology.sites[r]);
		const float elev = (r < r_elevation.size()) ? r_elevation[r] : 0.0f;
		const Vec3 pos   = dir * (1.0f + k_elevation_scale * elev);
		out.vertices.push_back(pos);
		out.normals.push_back(normalised_vec3(pos));
	}

	out.indices.reserve(num_tri * 3);
	for (size_t t = 0; t < num_tri; ++t) {
		const auto &tri = topology.triangles[t];
		out.indices.push_back(static_cast<uint32_t>(tri[0]));
		out.indices.push_back(static_cast<uint32_t>(tri[1]));
		out.indices.push_back(static_cast<uint32_t>(tri[2]));
	}

	out.river_strength.resize(num_tri);
	parallel::for_range(0, num_tri, 4096, [&](size_t t) {
		out.river_strength[t] = triangle_river_strength(river, t);
	});
}

void generate_planet_terrain_mesh_dual_folded(const PlanetGlobe &globe, PlanetTerrainMesh &out) {
	out.vertices.clear();
	out.normals.clear();
	out.indices.clear();
	out.river_strength.clear();
	out.vertex_elevation.clear();
	out.vertex_moisture.clear();

	const SphereHalfEdgeMesh &mesh     = globe.mesh;
	const Vector<float> &r_elevation   = globe.region_elevation.region_elevation;
	const Vector<float> &r_moisture    = globe.region_moisture.region_moisture;
	const Vector<float> &tri_elev      = globe.triangle_values.triangle_elevation;
	const Vector<float> &tri_moist     = globe.triangle_values.triangle_moisture;
	const RiverFlow &river             = globe.river_flow;
	const size_t num_regions           = globe.topology.sites.size();
	const size_t num_tri               = globe.topology.triangles.size();
	const size_t num_sides             = mesh.num_sides();
	if (num_regions == 0 || num_tri == 0 || num_sides == 0)
		return;

	const size_t num_vertices = num_regions + num_tri;
	out.vertices.resize(num_vertices);
	out.normals.resize(num_vertices);
	out.vertex_elevation.resize(num_vertices);
	out.vertex_moisture.resize(num_vertices);

	parallel::for_range(0, num_regions, 4096, [&](size_t r) {
		const Vec3 dir   = normalised_vec3(globe.topology.sites[r]);
		const float elev = (r < r_elevation.size()) ? r_elevation[r] : 0.0f;
		const float moist = (r < r_moisture.size()) ? r_moisture[r] : 0.0f;
		const Vec3 pos   = dir * (1.0f + k_elevation_scale * elev);
		out.vertices[r]          = pos;
		out.normals[r]           = normalised_vec3(pos);
		out.vertex_elevation[r]  = elev;
		out.vertex_moisture[r]   = moist;
	});

	parallel::for_range(0, num_tri, 4096, [&](size_t t) {
		const size_t v = num_regions + t;
		const Vec3 dir   = normalised_vec3(mesh.t_xyz[t]);
		const float elev = (t < tri_elev.size()) ? tri_elev[t] : 0.0f;
		const float moist = (t < tri_moist.size()) ? tri_moist[t] : 0.0f;
		const Vec3 pos   = dir * (1.0f + k_elevation_scale * elev);
		out.vertices[v]          = pos;
		out.normals[v]           = normalised_vec3(pos);
		out.vertex_elevation[v]  = elev;
		out.vertex_moisture[v]   = moist;
	});

	out.indices.resize(num_sides * 3u);
	out.river_strength.resize(num_sides);

	for (size_t s = 0; s < num_sides; ++s) {
		const size_t r1   = mesh.s_begin_r[s];
		const size_t twin = mesh.s_twin_s[s];
		const size_t r2   = (twin != SphereHalfEdgeMesh::k_invalid) ? mesh.s_begin_r[twin] : r1;
		const size_t t1   = mesh.s_inner_t[s];

		// Continuous dual surface: one triangle per half-edge (r1, r2, inner triangle).
		// Coast/river ridge folding is disabled so the preview shell is watertight.
		out.indices[s * 3u + 0] = static_cast<uint32_t>(r1);
		out.indices[s * 3u + 1] = static_cast<uint32_t>(r2);
		out.indices[s * 3u + 2] = static_cast<uint32_t>(num_regions + t1);
		out.river_strength[s] = (s < river.s_flow.size()) ? river.s_flow[s] : 0.0f;
	}

	ensure_planet_terrain_mesh_outward_winding(out);
}

void ensure_planet_terrain_mesh_outward_winding(PlanetTerrainMesh &mesh) {
	const size_t mesh_tris  = mesh.indices.size() / 3u;
	const size_t mesh_verts = mesh.vertices.size();
	if (mesh_tris == 0 || mesh_verts == 0)
		return;

	for (size_t t = 0; t < mesh_tris; ++t) {
		const size_t i0 = mesh.indices[t * 3u + 0];
		const size_t i1 = mesh.indices[t * 3u + 1];
		const size_t i2 = mesh.indices[t * 3u + 2];
		if (i0 >= mesh_verts || i1 >= mesh_verts || i2 >= mesh_verts)
			continue;
		const Vec3 &a = mesh.vertices[i0];
		const Vec3 &b = mesh.vertices[i1];
		const Vec3 &c = mesh.vertices[i2];
		const Vec3 centre = (a + b + c) * (1.0f / 3.0f);
		const Vec3 n      = (b - a).cross(c - a);
		if (n.dot(centre) < 0.0f) {
			const uint32_t tmp              = mesh.indices[t * 3u + 1];
			mesh.indices[t * 3u + 1] = mesh.indices[t * 3u + 2];
			mesh.indices[t * 3u + 2] = tmp;
		}
	}
}

void generate_planet_terrain_mesh_floating_islands(const PlanetGlobe &globe,
                                                   PlanetTerrainMesh &out,
                                                   Vector<uint32_t> *triangle_region_out) {
	out.vertices.clear();
	out.normals.clear();
	out.indices.clear();
	out.river_strength.clear();
	out.vertex_elevation.clear();
	out.vertex_moisture.clear();
	if (triangle_region_out)
		triangle_region_out->clear();

	const SphereTopology &topology   = globe.topology;
	const CsrGraph &rings            = globe.region_triangle_rings;
	const Vector<Vec3> &t_xyz        = globe.mesh.t_xyz;
	const Vector<float> &r_elevation = globe.region_elevation.region_elevation;
	const RiverFlow &river           = globe.river_flow;
	const size_t num_regions         = topology.sites.size();
	const size_t num_tri_topology    = topology.triangles.size();
	if (num_regions == 0 || rings.num_nodes() != num_regions || t_xyz.size() != num_tri_topology)
		return;

	// Pass 1: count valid ring vertices and triangles per region (fan-triangulation needs >=3).
	Vector<uint32_t> vert_count(num_regions, 0u);
	Vector<uint32_t> tri_count(num_regions, 0u);
	parallel::for_range(0, num_regions, 1024, [&](size_t r) {
		const U32 *cb = rings.neighbours_begin(r);
		const U32 *ce = rings.neighbours_end(r);
		uint32_t valid = 0u;
		for (const U32 *it = cb; it != ce; ++it) {
			if (*it < num_tri_topology)
				++valid;
		}
		if (valid < 3u)
			return;
		// Each region emits: (valid ring vertices) + 1 centre vertex per region; fan has `valid` triangles.
		vert_count[r] = valid + 1u;
		tri_count[r]  = valid;
	});

	// Pass 2: exclusive scan for offsets.
	Vector<uint32_t> vert_offsets(num_regions + 1u, 0u);
	Vector<uint32_t> tri_offsets(num_regions + 1u, 0u);
	for (size_t r = 0; r < num_regions; ++r) {
		vert_offsets[r + 1] = vert_offsets[r] + vert_count[r];
		tri_offsets[r + 1]  = tri_offsets[r] + tri_count[r];
	}
	const size_t total_verts = vert_offsets[num_regions];
	const size_t total_tris  = tri_offsets[num_regions];

	out.vertices.resize(total_verts);
	out.normals.resize(total_verts);
	out.vertex_elevation.resize(total_verts);
	out.vertex_moisture.resize(total_verts);
	out.indices.resize(total_tris * 3u);
	out.river_strength.resize(total_tris);
	if (triangle_region_out)
		triangle_region_out->resize(total_tris);

	// Pass 3: parallel emit per region.
	parallel::for_range(0, num_regions, 256, [&](size_t r) {
		const uint32_t vbase = vert_offsets[r];
		const uint32_t tbase = tri_offsets[r];
		const uint32_t vcount = vert_count[r];
		const uint32_t tcount = tri_count[r];
		if (vcount < 4u || tcount == 0u)
			return;

		const Vec3 site_dir   = normalised_vec3(topology.sites[r]);
		const float elev      = (r < r_elevation.size()) ? r_elevation[r] : 0.0f;
		const float moist     = (r < globe.region_moisture.region_moisture.size()) ? globe.region_moisture.region_moisture[r] : 0.0f;
		// Floating islands: lift elevation above zero so the mesh hangs above the sea floor.
		const float lifted    = (elev < 0.0f) ? 0.0f : elev;
		const float radius    = 1.0f + k_elevation_scale * lifted;
		const Vec3 centre_pos = site_dir * radius;
		out.vertices[vbase + vcount - 1u] = centre_pos;
		out.normals[vbase + vcount - 1u]  = normalised_vec3(centre_pos);
		out.vertex_elevation[vbase + vcount - 1u] = lifted;
		out.vertex_moisture[vbase + vcount - 1u]  = moist;

		const U32 *cb     = rings.neighbours_begin(r);
		const U32 *ce     = rings.neighbours_end(r);
		uint32_t local_v  = 0u;
		Vector<uint32_t> ring_tris;
		ring_tris.reserve(vcount - 1u);
		for (const U32 *it = cb; it != ce; ++it) {
			const U32 t = *it;
			if (t >= num_tri_topology)
				continue;
			const Vec3 tdir = normalised_vec3(t_xyz[t]);
			const Vec3 pos  = tdir * radius;
			out.vertices[vbase + local_v] = pos;
			out.normals[vbase + local_v]  = normalised_vec3(pos);
			out.vertex_elevation[vbase + local_v] = lifted;
			out.vertex_moisture[vbase + local_v]  = moist;
			ring_tris.push_back(t);
			++local_v;
		}

		// Fan-triangulate: triangle i = (centre, ring[i], ring[(i+1) % ring_len]).
		const uint32_t ring_len = local_v;
		const uint32_t centre_idx = vbase + vcount - 1u;
		for (uint32_t i = 0; i < ring_len; ++i) {
			const uint32_t a = vbase + i;
			const uint32_t b = vbase + ((i + 1u) % ring_len);
			out.indices[(tbase + i) * 3u + 0] = centre_idx;
			out.indices[(tbase + i) * 3u + 1] = a;
			out.indices[(tbase + i) * 3u + 2] = b;
			const uint32_t tri_id = ring_tris[i];
			out.river_strength[tbase + i] = triangle_river_strength(river, tri_id);
			if (triangle_region_out)
				(*triangle_region_out)[tbase + i] = static_cast<uint32_t>(r);
		}
	});
}

} // namespace growth
