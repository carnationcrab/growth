#pragma once

#include "math/Vec3.hpp"
#include "base/gateway/Cstddef.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

/// Per-plate properties for the elevation stage.
///
/// We use the Euler-pole formulation from McKenzie & Parker (1967): each rigid plate's motion
/// on the sphere is a rotation about a single axis through the planet's centre, with a fixed
/// angular speed. The angular velocity is encoded as a single vector `omega` whose direction is
/// the rotation axis and whose magnitude is the angular speed. The instantaneous tangential
/// velocity at any point `p` on the unit sphere is then `omega.cross(p)`. This is correct
/// everywhere on the plate, not just near a representative point as the older "single drift
/// vector" simplification was. See:
///   https://blogs.egu.eu/divisions/ts/2020/09/30/ts-must-read-papers-mckenzie-and-parker-1967/
struct PlateProperties {
	/// Angular velocity (Euler vector) per plate. Direction = rotation axis through origin,
	/// magnitude = angular speed (consumed alongside `k_collision_delta_time`). Velocity at a
	/// point `p` on the unit sphere is `plate_omega[id].cross(p)`.
	Vector<Vec3> plate_omega;
	/// True if plate is oceanic (basin), false if continental.
	Vector<bool> plate_is_ocean;
	size_t num_plates = 0;
};

} // namespace growth
