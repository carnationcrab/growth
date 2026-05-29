#include "world/SphereDual.hpp"
#include "world/PlanetGlobe.hpp"
#include "util/Parallel.hpp"

namespace growth {

Vec3 SphereDual::triangle_centroid(const SphereTopology &topology, const Array<size_t, 3> &triangle) {
	const Vec3 &a = topology.sites[triangle[0]];
	const Vec3 &b = topology.sites[triangle[1]];
	const Vec3 &c = topology.sites[triangle[2]];
	return Vec3((a.x + b.x + c.x) / 3.0, (a.y + b.y + c.y) / 3.0, (a.z + b.z + c.z) / 3.0).normalised();
}

void SphereDual::build_triangle_centroids(const SphereTopology &topology, SphereHalfEdgeMesh &mesh) {
	const size_t num_tri = topology.triangles.size();
	mesh.t_xyz.resize(num_tri);
	parallel::for_range(0, num_tri, 1024, [&](size_t t) {
		mesh.t_xyz[t] = triangle_centroid(topology, topology.triangles[t]);
	});
}

namespace {

U32 walk_ring(const SphereHalfEdgeMesh &mesh,
              size_t start_s,
              U32 *out,
              U32 capacity,
              bool write_out) {
	const size_t num_sides = mesh.num_sides();
	const size_t cap       = num_sides + 1;
	size_t steps           = 0;
	size_t s               = start_s;
	U32 count              = 0;
	do {
		if (++steps > cap)
			break;
		if (write_out && count < capacity)
			out[count] = static_cast<U32>(mesh.s_inner_t[s]);
		++count;
		const size_t next_s = SphereHalfEdgeMesh::next_side_around_region(mesh, s);
		if (next_s == SphereHalfEdgeMesh::k_invalid)
			break;
		s = next_s;
	} while (s != start_s);
	return count;
}

} // namespace

void SphereDual::build_region_triangle_rings(const SphereHalfEdgeMesh &mesh, CsrGraph &out) {
	const size_t num_regions = mesh.num_regions();
	const size_t num_sides   = mesh.num_sides();
	out.clear();
	if (num_regions == 0) {
		out.offsets.assign(1, 0u);
		return;
	}

	Vector<size_t> first_side(num_regions, SphereHalfEdgeMesh::k_invalid);
	for (size_t s = 0; s < num_sides; ++s) {
		const size_t begin = mesh.s_begin_r[s];
		if (begin < num_regions && first_side[begin] == SphereHalfEdgeMesh::k_invalid)
			first_side[begin] = s;
	}

	Vector<U32> degrees(num_regions, 0u);
	parallel::for_range(0, num_regions, 256, [&](size_t r) {
		const size_t start_s = first_side[r];
		if (start_s == SphereHalfEdgeMesh::k_invalid)
			return;
		degrees[r] = walk_ring(mesh, start_s, nullptr, 0u, false);
	});

	csr_build::scan_offsets_exclusive(degrees, out.offsets);
	out.neighbours.resize(out.offsets[num_regions]);

	parallel::for_range(0, num_regions, 256, [&](size_t r) {
		const size_t start_s = first_side[r];
		if (start_s == SphereHalfEdgeMesh::k_invalid)
			return;
		const U32 off = out.offsets[r];
		const U32 cap = out.offsets[r + 1] - off;
		walk_ring(mesh, start_s, out.neighbours.data() + off, cap, true);
	});
}

void SphereDual::region_triangle_rings_for_globe(const PlanetGlobe &globe, Vector<Vector<Size>> &out) {
	if (!globe.region_triangle_rings.empty() && globe.region_triangle_rings.num_nodes() == globe.mesh.num_regions()) {
		csr_to_vec_of_vec(globe.region_triangle_rings, out);
		return;
	}
	CsrGraph csr;
	build_region_triangle_rings(globe.mesh, csr);
	csr_to_vec_of_vec(csr, out);
}

} // namespace growth
