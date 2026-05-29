#include "world/WorldGenPreviewExport.hpp"
#include "world/RiverFlow.hpp"
#include "world/SphereDual.hpp"
#include "world/SphereHalfEdgeMesh.hpp"

namespace growth {

namespace {

constexpr float k_elevation_scale = 0.08f;

Vec3 displace_triangle(const SphereHalfEdgeMesh &mesh, const Vector<float> &tri_elev, size_t t) {
	if (t >= mesh.t_xyz.size())
		return Vec3(0.0f, 0.0f, 1.0f);
	Vec3 dir = mesh.t_xyz[t];
	const float len = dir.length();
	if (len > 1e-6f)
		dir = dir / len;
	const float e = (t < tri_elev.size()) ? tri_elev[t] : 0.0f;
	return dir * (1.0f + k_elevation_scale * e);
}

} // namespace

bool WorldGenPreviewExport::include_cell_rings(size_t num_regions) {
	return num_regions <= k_max_regions_with_cell_rings;
}

bool WorldGenPreviewExport::include_debug_mesh(size_t num_regions) {
	return num_regions <= k_max_regions_with_debug_mesh;
}

void WorldGenPreviewExport::triangle_centroids_for_preview(const PlanetGlobe &globe, Vector<Vec3> &out) {
	out = globe.mesh.t_xyz;
}

void WorldGenPreviewExport::region_cell_rings_for_preview(const PlanetGlobe &globe, Vector<Vector<size_t>> &out) {
	Vector<Vector<Size>> rings;
	SphereDual::region_triangle_rings_for_globe(globe, rings);
	out.assign(rings.begin(), rings.end());
}

void WorldGenPreviewExport::river_lines_for_preview(const PlanetGlobe &globe, Vector<Vec3> &out_segment_endpoints) {
	out_segment_endpoints.clear();

	const RiverFlow &river = globe.river_flow;
	const SphereHalfEdgeMesh &mesh = globe.mesh;
	const Vector<float> &tri_elev = globe.triangle_values.triangle_elevation;
	const size_t num_tri = mesh.num_triangles();
	const size_t num_sides = mesh.num_sides();
	if (num_tri == 0 || num_sides == 0 || river.s_flow.size() < num_sides
	    || river.t_downflow_s.size() < num_tri)
		return;

	const float threshold = river_flow_visual_threshold(river.s_flow);
	if (threshold >= 1e8f)
		return;

	out_segment_endpoints.reserve(num_tri);

	// RBG-style: one segment per land triangle along its downflow edge (tributary → trunk).
	for (size_t t = 0; t < num_tri; ++t) {
		const float e_t = (t < tri_elev.size()) ? tri_elev[t] : 0.0f;
		if (e_t < 0.0f)
			continue;

		const size_t flow_s = river.t_downflow_s[t];
		if (flow_s == RiverFlow::k_invalid || flow_s >= num_sides)
			continue;
		if (river.s_flow[flow_s] < threshold)
			continue;

		const size_t t_trunk = mesh.s_outer_t[flow_s];
		if (t_trunk == SphereHalfEdgeMesh::k_invalid || t_trunk >= num_tri)
			continue;

		out_segment_endpoints.push_back(displace_triangle(mesh, tri_elev, t));
		out_segment_endpoints.push_back(displace_triangle(mesh, tri_elev, t_trunk));
	}
}

} // namespace growth
