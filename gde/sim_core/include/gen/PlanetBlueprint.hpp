#pragma once

#include "PlanetGenome.hpp"
#include "util/Random.hpp"
#include "base/gateway/Cstdint.hpp"
#include "base/gateway/Cstring.hpp"

namespace growth {

/// Distribution type for a blueprint knob. Parameters (a,b,c,d) interpreted per type.
enum class DistType : uint8_t {
	Constant,    /// Single value a.
	Uniform,     /// min=a, max=b.
	LogUniform,  /// log_min=a, log_max=b (sample in log space).
	Normal,      /// mean=a, sigma=b, clamp to [c, d] if c<=d.
	Beta,        /// alpha=a, beta=b, scale to [c, d].
};

/// Distribution spec: type + four parameters. Reinterpret (a,b,c,d) by DistType.
struct DistSpec {
	DistType type = DistType::Uniform;
	double a = 0.0;
	double b = 1.0;
	double c = 0.0;
	double d = 1.0;
	bool log_space = false;  /// If true, sample in log space (for Uniform/LogUniform).
};

/// Habitability constraint ranges. Applied after sampling; reject or remap.
struct HabitabilityConstraintsSpec {
	double insolation_min = 0.8;   /// S_eff
	double insolation_max = 1.2;
	double surface_temp_min = 273.0;  // K
	double surface_temp_max = 330.0;
	double gravity_min = 8.0;    // m/s^2
	double gravity_max = 12.0;
	double pressure_min = 0.5e5; // Pa
	double pressure_max = 2.0e5;
	double eccentricity_max = 0.25;
	bool allow_tidal_lock = false;
	int max_attempts = 32;
};

/// Policy: what is stored vs derived in the genome.
enum class RadiusMode : uint8_t { StoredRadius, DerivedFromMassAndComposition };
enum class StarMode : uint8_t { StoreLuminosity, StoreMassAndUseModel };

/// Stream IDs for reproducible RNG per logical group (orbit, atmosphere, etc.).
struct PlanetStreamIds {
	uint32_t star = 0;
	uint32_t orbit = 1;
	uint32_t bulk = 2;
	uint32_t rotation = 3;
	uint32_t atmos = 4;
	uint32_t hydro = 5;
	uint32_t precipitation = 6;
	uint32_t materials = 7;
};

/// Full generation spec: all knobs, constraints, policies. All fields settable.
struct PlanetGenBlueprint {
	uint64_t seed = 0;
	PlanetStreamIds stream_ids;

	RadiusMode radius_mode = RadiusMode::StoredRadius;
	StarMode star_mode = StarMode::StoreLuminosity;

	DistSpec L_star;
	DistSpec a;
	DistSpec e;
	DistSpec M_p;
	DistSpec R_p;
	DistSpec P_rot;
	DistSpec obliquity;
	DistSpec p0;
	DistSpec albedo;
	DistSpec greenhouse_factor;
	DistSpec water_fraction;
	DistSpec precipitation;
	DistSpec S_eff_target;  /// Target insolation (Earth=1); can derive a from L_star and S_eff.

	/// Material tag weights: probability (0..1) per tag to set bit. Sampled per tag.
	double material_weight_iron_rich = 0.0;
	double material_weight_high_silicate = 0.5;
	double material_weight_ice = 0.0;
	double material_weight_organic = 0.2;
	double material_weight_volatile = 0.0;
	double material_weight_low_metal = 0.0;

	HabitabilityConstraintsSpec constraints;
};

/// Set blueprint to a named preset (Earthlike, OceanWorld, Dry). Preset data can be loaded from XML elsewhere.
void set_blueprint_to_preset(PlanetGenBlueprint &blueprint, const String &preset_name);

/// Set blueprint to random valid configuration (wider ranges, randomised weights). Uses rng for weights only.
void set_blueprint_to_random(PlanetGenBlueprint &blueprint, random::RNG &rng);

} // namespace growth
