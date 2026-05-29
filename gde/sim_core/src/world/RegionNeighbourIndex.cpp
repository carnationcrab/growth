#include "world/RegionNeighbourIndex.hpp"
#include "util/Parallel.hpp"

namespace growth {

namespace {

void walk_region_neighbours(const SphereHalfEdgeMesh &mesh,
                            const Vector<Size> &first_side,
                            Size r,
                            U32 *out_neighbours,
                            U32 capacity,
                            U32 &out_count) {
	const Size num_regions = mesh.num_regions();
	const Size num_sides   = mesh.num_sides();
	out_count              = 0;
	const Size start_s = first_side[r];
	if (start_s == SphereHalfEdgeMesh::k_invalid)
		return;
	const Size cap = num_sides + 1;
	Size steps     = 0;
	Size s         = start_s;
	do {
		if (++steps > cap)
			break;
		const Size end_r = mesh.s_end_r[s];
		if (end_r < num_regions && out_count < capacity)
			out_neighbours[out_count++] = static_cast<U32>(end_r);
		const Size next_s = SphereHalfEdgeMesh::next_side_around_region(mesh, s);
		if (next_s == SphereHalfEdgeMesh::k_invalid)
			break;
		s = next_s;
	} while (s != start_s);
}

} // namespace

void RegionNeighbourIndex::build(const SphereHalfEdgeMesh &mesh, CsrGraph &out) {
	const Size num_regions = mesh.num_regions();
	const Size num_sides   = mesh.num_sides();
	out.clear();
	if (num_regions == 0) {
		out.offsets.assign(1, 0u);
		return;
	}

	// One representative starting side per region (the first side encountered whose s_begin_r is r).
	// Serial: O(num_sides), trivially cheap vs the per-region walks.
	Vector<Size> first_side(num_regions, SphereHalfEdgeMesh::k_invalid);
	for (Size s = 0; s < num_sides; ++s) {
		const Size begin = mesh.s_begin_r[s];
		if (begin < num_regions && first_side[begin] == SphereHalfEdgeMesh::k_invalid)
			first_side[begin] = s;
	}

	// Pass 1: parallel count of degrees by walking each region's ring.
	Vector<U32> degrees(num_regions, 0u);
	parallel::for_range(0, num_regions, 256, [&](Size r) {
		const Size start_s = first_side[r];
		if (start_s == SphereHalfEdgeMesh::k_invalid)
			return;
		const Size cap = num_sides + 1;
		Size steps     = 0;
		Size s         = start_s;
		U32 d          = 0;
		do {
			if (++steps > cap)
				break;
			if (mesh.s_end_r[s] < num_regions)
				++d;
			const Size next_s = SphereHalfEdgeMesh::next_side_around_region(mesh, s);
			if (next_s == SphereHalfEdgeMesh::k_invalid)
				break;
			s = next_s;
		} while (s != start_s);
		degrees[r] = d;
	});

	csr_build::scan_offsets_exclusive(degrees, out.offsets);
	out.neighbours.resize(out.offsets[num_regions]);

	// Pass 2: parallel scatter using the precomputed offsets.
	parallel::for_range(0, num_regions, 256, [&](Size r) {
		const U32 off  = out.offsets[r];
		const U32 cap  = out.offsets[r + 1] - off;
		U32 count      = 0;
		walk_region_neighbours(mesh, first_side, r, out.neighbours.data() + off, cap, count);
		(void)count;
	});
}

} // namespace growth
