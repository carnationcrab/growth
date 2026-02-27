// Blueprint presets (Earthlike, OceanWorld, Dry) and random blueprint; used by WorldGenRunner before PlanetGenerator.
#include "gen/PlanetBlueprint.hpp"
#include "util/Random.hpp"
#include <cmath>
#include <algorithm>

namespace growth {

namespace {

	void preset_earthlike(PlanetGenBlueprint &b) {
		b.L_star = { DistType::Uniform, 0.9 * 3.828e26, 1.1 * 3.828e26, 0, 0, false };
		b.S_eff_target = { DistType::Normal, 1.0, 0.15, 0.8, 1.2 };
		b.a = { DistType::Uniform, 1.2e11, 1.7e11, 0, 0, false };
		b.e = { DistType::Uniform, 0.0, 0.08, 0, 0, false };
		b.M_p = { DistType::Uniform, 0.8 * 5.972e24, 1.2 * 5.972e24, 0, 0, false };
		b.R_p = { DistType::Uniform, 0.95 * 6.371e6, 1.05 * 6.371e6, 0, 0, false };
		b.P_rot = { DistType::Uniform, 75600.0, 97200.0, 0, 0, false };  // 21–27 h
		b.obliquity = { DistType::Normal, 0.4091, 0.1, 0.2, 0.6 };
		b.p0 = { DistType::Uniform, 0.9e5, 1.1e5, 0, 0, false };
		b.albedo = { DistType::Uniform, 0.28, 0.35, 0, 0, false };
		b.greenhouse_factor = { DistType::Uniform, 0.95, 1.08, 0, 0, false };
		b.water_fraction = { DistType::Uniform, 0.65, 0.75, 0, 0, false };
		b.precipitation = { DistType::Uniform, 0.4, 0.7, 0, 0, false };
		b.material_weight_high_silicate = 0.6;
		b.material_weight_organic = 0.3;
		b.constraints = { 0.85, 1.15, 273.0, 310.0, 8.5, 11.5, 0.7e5, 1.4e5, 0.2, false, 32 };
	}

	void preset_ocean_world(PlanetGenBlueprint &b) {
		preset_earthlike(b);
		b.water_fraction = { DistType::Uniform, 0.85, 0.95, 0, 0, false };
		b.precipitation = { DistType::Uniform, 0.6, 0.95, 0, 0, false };
		b.p0 = { DistType::Uniform, 1.0e5, 1.5e5, 0, 0, false };
		b.constraints.surface_temp_max = 320.0;
	}

	void preset_dry(PlanetGenBlueprint &b) {
		preset_earthlike(b);
		b.water_fraction = { DistType::Uniform, 0.1, 0.35, 0, 0, false };
		b.precipitation = { DistType::Uniform, 0.05, 0.25, 0, 0, false };
		b.albedo = { DistType::Uniform, 0.32, 0.42, 0, 0, false };
	}
}

void set_blueprint_to_preset(PlanetGenBlueprint &blueprint, const std::string &preset_name) {
	if (preset_name == "Earthlike")
		preset_earthlike(blueprint);
	else if (preset_name == "OceanWorld")
		preset_ocean_world(blueprint);
	else if (preset_name == "Dry")
		preset_dry(blueprint);
	else
		preset_earthlike(blueprint);  // default
}

void set_blueprint_to_random(PlanetGenBlueprint &blueprint, random::RNG &rng) {
	double u = static_cast<double>(rng.next_float());
	if (u < 0.33)
		preset_earthlike(blueprint);
	else if (u < 0.66)
		preset_ocean_world(blueprint);
	else
		preset_dry(blueprint);
	// Widen ranges slightly and randomise material weights
	blueprint.material_weight_iron_rich = static_cast<double>(rng.next_float()) * 0.3;
	blueprint.material_weight_high_silicate = 0.3 + static_cast<double>(rng.next_float()) * 0.5;
	blueprint.material_weight_ice = static_cast<double>(rng.next_float()) * 0.2;
	blueprint.material_weight_organic = static_cast<double>(rng.next_float()) * 0.4;
	blueprint.material_weight_volatile = static_cast<double>(rng.next_float()) * 0.2;
	blueprint.material_weight_low_metal = static_cast<double>(rng.next_float()) * 0.2;
}

} // namespace growth
