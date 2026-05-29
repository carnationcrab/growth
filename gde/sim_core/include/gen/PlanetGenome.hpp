#include "base/gateway/Cstdint.hpp"
#pragma once


namespace growth {

/// Material tags as bitfield bits for planet surface/atmosphere character (no single composition_id).
enum class MaterialTag : uint32_t {
	None = 0,
	IronRich = 1u << 0,
	HighSilicate = 1u << 1,
	IcePresent = 1u << 2,
	Organic = 1u << 3,
	VolatileRich = 1u << 4,
	LowMetal = 1u << 5,
};

inline MaterialTag operator|(MaterialTag a, MaterialTag b) {
	return static_cast<MaterialTag>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline uint32_t operator&(MaterialTag a, MaterialTag b) {
	return static_cast<uint32_t>(a) & static_cast<uint32_t>(b);
}

/// Minimal stored parameters for a habitable terrestrial planet. All SI or dimensionless.
/// Phenotype (gravity, year length, temperature, etc.) is derived on demand via Planet.
struct PlanetGenome {
	uint64_t seed = 0;
	uint16_t schema_version = 1;  /// Generator/schema version for reproducibility.

	/// Star: luminosity (W). Insolation scales as L_star / a^2.
	double L_star = 3.828e26;  // Sun

	/// Orbit: semimajor axis (m), eccentricity [0, ~0.3].
	double a = 1.496e11;       // 1 AU
	double e = 0.0167;

	/// Effective insolation (Earth = 1.0). Stored so climate logic is stable if star/orbit models change.
	double S_eff = 1.0;

	/// Planet bulk: mass (kg), radius (m).
	double M_p = 5.972e24;     // Earth
	double R_p = 6.371e6;

	/// Rotation: period (s), obliquity (radians).
	double P_rot = 86400.0;    // 1 day
	double obliquity = 0.4091; // ~23.4°

	/// Atmosphere: surface pressure (Pa), mean molecular weight (kg/mol), greenhouse factor, Bond albedo [0,1].
	double p0 = 101325.0;
	double mean_molecular_weight = 0.02897;  // kg/mol (N2/O2 mix)
	double greenhouse_factor = 1.0;          // dimensionless knob
	double albedo = 0.3;

	/// Hydrosphere: ocean/water coverage [0, 1].
	double water_fraction = 0.71;

	/// Planet-level precipitation (e.g. relative 0..1). Cloudiness is handled by game weather.
	double precipitation = 0.5;

	/// Unique materials tags (bitfield). See MaterialTag.
	uint32_t material_tags = 0;
};

} // namespace growth
