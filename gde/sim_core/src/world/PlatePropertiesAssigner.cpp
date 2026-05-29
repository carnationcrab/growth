#include "world/PlatePropertiesAssigner.hpp"
#include "util/Random.hpp"
#include "base/gateway/Cmath.hpp"
#include "base/gateway/Cstddef.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

namespace {

constexpr uint64_t k_plate_props_stream = 0x8f5b3c1e9d6a4f2bULL;

/// For a unit-sphere centroid `c` and a desired tangential velocity `v` at that point (with
/// `v` perpendicular to `c`), return the Euler-pole angular velocity vector `omega` such that
/// `omega.cross(c) == v`. Identity used: `(c x v) x c = (c.c)v - (v.c)c = v` when |c|=1 and
/// `v . c = 0`. Magnitude of omega equals the magnitude of v_tangent, so a unit-length tangent
/// yields unit angular speed and preserves the existing tuning of k_collision_delta_time.
inline Vec3 omega_from_tangent(const Vec3 &c, const Vec3 &v_tangent) {
	return c.cross(v_tangent);
}

inline Vec3 project_to_tangent(const Vec3 &centroid_unit, const Vec3 &target) {
	const Vec3 delta   = target - centroid_unit;
	const float radial = delta.dot(centroid_unit);
	const Vec3 t       = delta - centroid_unit * radial;
	const float len    = t.length();
	if (len > 1e-6f)
		return t / len;
	const Vec3 ref = (Cmath::abs(centroid_unit.x) < 0.9f) ? Vec3(1.0f, 0.0f, 0.0f) : Vec3(0.0f, 1.0f, 0.0f);
	const Vec3 fb  = ref - centroid_unit * ref.dot(centroid_unit);
	const float fb_len = fb.length();
	return fb_len > 1e-6f ? fb / fb_len : Vec3(1.0f, 0.0f, 0.0f);
}

} // namespace

void PlatePropertiesAssigner::assign(const SphereTopology &topology,
                                     const CsrGraph &region_neighbours,
                                     const TectonicPlates &tectonic_plates,
                                     const WorldSeed &world_seed,
                                     PlateProperties &out) const {
	const size_t num_regions = topology.sites.size();
	const size_t num_plates  = tectonic_plates.num_plates;
	out.num_plates = num_plates;
	out.plate_omega.clear();
	out.plate_is_ocean.clear();
	if (num_plates == 0 || num_regions == 0)
		return;

	const Vec3 *sites                  = topology.sites.data();
	const Vector<int32_t> &r2p         = tectonic_plates.region_to_plate;

	out.plate_omega.resize(num_plates);
	out.plate_is_ocean.resize(num_plates);

	random::RNG rng(world_seed.value ^ k_plate_props_stream);

	// First serial pass: accumulate centroids and pick a representative boundary-neighbour per
	// plate. Single sweep over regions; cheap and deterministic.
	Vector<Vec3> plate_sum(num_plates, Vec3(0.0f, 0.0f, 0.0f));
	Vector<size_t> plate_count(num_plates, 0);
	Vector<size_t> plate_boundary_r(num_plates, static_cast<size_t>(-1));

	for (size_t r = 0; r < num_regions; ++r) {
		const int32_t plate_id = r2p[r];
		if (plate_id < 0 || static_cast<size_t>(plate_id) >= num_plates)
			continue;
		const size_t p = static_cast<size_t>(plate_id);
		plate_sum[p].x += sites[r].x;
		plate_sum[p].y += sites[r].y;
		plate_sum[p].z += sites[r].z;
		++plate_count[p];

		if (plate_boundary_r[p] != static_cast<size_t>(-1))
			continue;
		const U32 *nb_b = region_neighbours.neighbours_begin(r);
		const U32 *nb_e = region_neighbours.neighbours_end(r);
		for (const U32 *it = nb_b; it != nb_e; ++it) {
			const size_t n = static_cast<size_t>(*it);
			if (n >= num_regions)
				continue;
			if (r2p[n] != plate_id) {
				plate_boundary_r[p] = n;
				break;
			}
		}
	}

	for (size_t plate_id = 0; plate_id < num_plates; ++plate_id) {
		if (plate_count[plate_id] == 0) {
			out.plate_omega[plate_id]    = Vec3(0.0f, 0.0f, 1.0f);
			out.plate_is_ocean[plate_id] = false;
			continue;
		}
		Vec3 centroid    = plate_sum[plate_id];
		const float inv  = 1.0f / static_cast<float>(plate_count[plate_id]);
		centroid.x *= inv;
		centroid.y *= inv;
		centroid.z *= inv;
		const float clen = centroid.length();
		if (clen <= 1e-9f) {
			out.plate_omega[plate_id]    = Vec3(0.0f, 0.0f, 1.0f);
			out.plate_is_ocean[plate_id] = (rng.next_int(0, 9) < 5);
			continue;
		}
		centroid = centroid / clen;

		// Representative tangent direction: from centroid toward the first cross-plate
		// neighbour (already cached above). Then build the Euler pole that produces that
		// tangent velocity at the centroid. McKenzie & Parker (1967): velocity = omega x r.
		Vec3 v_tangent;
		const size_t br = plate_boundary_r[plate_id];
		if (br == static_cast<size_t>(-1))
			v_tangent = project_to_tangent(centroid, Vec3(1.0f, 0.0f, 0.0f) + centroid);
		else
			v_tangent = project_to_tangent(centroid, sites[br]);

		out.plate_omega[plate_id]    = omega_from_tangent(centroid, v_tangent);
		out.plate_is_ocean[plate_id] = (rng.next_int(0, 9) < 5);
	}
}

} // namespace growth
