#pragma once

#include "PlanetGenome.hpp"
#include "base/gateway/Cstdint.hpp"

namespace growth {

struct PlanetGenBlueprint;
struct PlanetGuardrails;

/// Guardrails for habitable terrestrial planet generation (earth-adjacent ranges).
struct PlanetGuardrails {
	double L_star_min = 0.5 * 3.828e26;   // 0.5 Sun
	double L_star_max = 1.5 * 3.828e26;  // 1.5 Sun
	double a_min = 1.0e11;                // m
	double a_max = 2.5e11;                // m
	double e_max = 0.25;
	double M_p_min = 0.5 * 5.972e24;     // Earth
	double M_p_max = 2.0 * 5.972e24;
	double R_p_min = 0.9 * 6.371e6;      // m
	double R_p_max = 1.2 * 6.371e6;
	double P_rot_min = 43200.0;           // 12 h (s)
	double P_rot_max = 129600.0;         // 36 h
	double obliquity_max = 0.52;         // ~30°
	double p0_min = 0.5e5;               // Pa
	double p0_max = 1.5e5;
	double albedo_min = 0.2;
	double albedo_max = 0.4;
	double greenhouse_min = 0.9;
	double greenhouse_max = 1.2;
	double water_min = 0.5;
	double water_max = 0.85;
};

/// Produces a PlanetGenome from a seed and optional guardrails or full blueprint. Deterministic.
class PlanetGenerator {
public:
	/// Generate a genome within default (earth-adjacent) guardrails.
	PlanetGenome generate(uint64_t seed) const;
	/// Generate a genome within the given guardrails.
	PlanetGenome generate(uint64_t seed, const PlanetGuardrails &guardrails) const;
	/// Generate a genome by sampling the blueprint and applying constraints. Uses blueprint.seed and stream_ids.
	PlanetGenome generate_from_blueprint(const PlanetGenBlueprint &blueprint) const;
};

} // namespace growth
