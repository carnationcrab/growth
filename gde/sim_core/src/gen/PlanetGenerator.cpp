// World-gen form submit pipeline (3/4): sample blueprint (star, orbit, bulk, rotation, atmos, hydro, materials) and apply habitability constraints.
#include "gen/PlanetGenerator.hpp"
#include "gen/PlanetBlueprint.hpp"
#include "gen/Planet.hpp"
#include "util/Random.hpp"
#include "base/gateway/Cmath.hpp"
#include "base/gateway/Calgorithm.hpp"

namespace growth {

namespace {

	constexpr double pi = 3.141592653589793;
	constexpr double S_earth = 1361.0;  // W/m^2

	double lerp(double a, double b, double t) {
		return a + (b - a) * t;
	}

	double clamp_d(double v, double lo, double hi) {
		return (v < lo) ? lo : (v > hi) ? hi : v;
	}

	uint64_t mix_stream(uint64_t seed, uint32_t stream_id) {
		return seed ^ (static_cast<uint64_t>(stream_id) * 0x9e3779b97f4a7c15ULL);
	}

	double sample_dist(const DistSpec &spec, random::RNG &rng) {
		double u = rng.next_double();
		switch (spec.type) {
		case DistType::Constant:
			return spec.a;
		case DistType::Uniform: {
			double v = spec.a + (spec.b - spec.a) * u;
			return spec.log_space ? Cmath::exp(v) : v;
		}
		case DistType::LogUniform: {
			double log_a = Cmath::log(Calgorithm::max(spec.a, 1e-300));
			double log_b = Cmath::log(Calgorithm::max(spec.b, 1e-300));
			return Cmath::exp(log_a + (log_b - log_a) * u);
		}
		case DistType::Normal: {
			double u2 = rng.next_double();
			double z = Cmath::sqrt(-2.0 * Cmath::log(Calgorithm::max(u, 1e-300))) * Cmath::cos(2.0 * pi * u2);
			double v = spec.a + spec.b * z;
			if (spec.c <= spec.d)
				v = clamp_d(v, spec.c, spec.d);
			return v;
		}
		case DistType::Beta: {
			int alpha = static_cast<int>(spec.a);
			int beta = static_cast<int>(spec.b);
			if (alpha < 1) alpha = 1;
			if (beta < 1) beta = 1;
			double ga = 0.0, gb = 0.0;
			for (int i = 0; i < alpha; ++i) ga -= Cmath::log(Calgorithm::max(rng.next_double(), 1e-300));
			for (int i = 0; i < beta; ++i)  gb -= Cmath::log(Calgorithm::max(rng.next_double(), 1e-300));
			double b = ga / (ga + gb);
			return spec.c + (spec.d - spec.c) * b;
		}
		}
		return spec.a;
	}

	bool meets_constraints(const PlanetGenome &g, const HabitabilityConstraintsSpec &c) {
		Planet p(g);
		double S = p.S_eff();
		if (S < c.insolation_min || S > c.insolation_max) return false;
		double T = p.T_surf_estimate();
		if (T < c.surface_temp_min || T > c.surface_temp_max) return false;
		double grav = p.g();
		if (grav < c.gravity_min || grav > c.gravity_max) return false;
		if (g.p0 < c.pressure_min || g.p0 > c.pressure_max) return false;
		if (g.e > c.eccentricity_max) return false;
		return true;
	}
}

PlanetGenome PlanetGenerator::generate(uint64_t seed) const {
	return generate(seed, PlanetGuardrails{});
}

PlanetGenome PlanetGenerator::generate(uint64_t seed, const PlanetGuardrails &g) const {
	random::RNG rng(seed);
	PlanetGenome out;
	out.seed = seed;
	out.schema_version = 1;

	out.L_star = lerp(g.L_star_min, g.L_star_max, static_cast<double>(rng.next_float()));
	out.a = lerp(g.a_min, g.a_max, static_cast<double>(rng.next_float()));
	out.e = Calgorithm::min(g.e_max, static_cast<double>(rng.next_float()) * g.e_max);
	out.M_p = lerp(g.M_p_min, g.M_p_max, static_cast<double>(rng.next_float()));
	out.R_p = lerp(g.R_p_min, g.R_p_max, static_cast<double>(rng.next_float()));
	out.P_rot = lerp(g.P_rot_min, g.P_rot_max, static_cast<double>(rng.next_float()));
	out.obliquity = static_cast<double>(rng.next_float()) * g.obliquity_max;
	out.p0 = lerp(g.p0_min, g.p0_max, static_cast<double>(rng.next_float()));
	out.albedo = lerp(g.albedo_min, g.albedo_max, static_cast<double>(rng.next_float()));
	out.greenhouse_factor = lerp(g.greenhouse_min, g.greenhouse_max, static_cast<double>(rng.next_float()));
	out.water_fraction = lerp(g.water_min, g.water_max, static_cast<double>(rng.next_float()));

	out.mean_molecular_weight = 0.02897;
	out.precipitation = 0.5;
	out.material_tags = 0;
	out.S_eff = out.L_star / (4.0 * pi * out.a * out.a * S_earth * Cmath::sqrt(1.0 - out.e * out.e));
	if (out.S_eff <= 0.0) out.S_eff = 1.0;

	return out;
}

PlanetGenome PlanetGenerator::generate_from_blueprint(const PlanetGenBlueprint &bp) const {
	constexpr double pi_val = 3.141592653589793;
	PlanetGenome out;
	out.seed = bp.seed;
	out.schema_version = 1;
	out.mean_molecular_weight = 0.02897;

	for (int attempt = 0; attempt < bp.constraints.max_attempts; ++attempt) {
		random::RNG rng(mix_stream(bp.seed, bp.stream_ids.star));
		out.L_star = sample_dist(bp.L_star, rng);

		rng.set_seed(mix_stream(bp.seed, bp.stream_ids.orbit));
		double S_eff = sample_dist(bp.S_eff_target, rng);
		S_eff = clamp_d(S_eff, bp.constraints.insolation_min, bp.constraints.insolation_max);
		out.a = Cmath::sqrt(out.L_star / (4.0 * pi_val * S_earth * S_eff));
		out.e = sample_dist(bp.e, rng);
		out.e = Calgorithm::min(out.e, bp.constraints.eccentricity_max);

		rng.set_seed(mix_stream(bp.seed, bp.stream_ids.bulk));
		out.M_p = sample_dist(bp.M_p, rng);
		out.R_p = sample_dist(bp.R_p, rng);

		rng.set_seed(mix_stream(bp.seed, bp.stream_ids.rotation));
		out.P_rot = sample_dist(bp.P_rot, rng);
		out.obliquity = sample_dist(bp.obliquity, rng);

		rng.set_seed(mix_stream(bp.seed, bp.stream_ids.atmos));
		out.p0 = sample_dist(bp.p0, rng);
		out.albedo = sample_dist(bp.albedo, rng);
		out.greenhouse_factor = sample_dist(bp.greenhouse_factor, rng);

		rng.set_seed(mix_stream(bp.seed, bp.stream_ids.hydro));
		out.water_fraction = sample_dist(bp.water_fraction, rng);

		rng.set_seed(mix_stream(bp.seed, bp.stream_ids.precipitation));
		out.precipitation = sample_dist(bp.precipitation, rng);

		out.S_eff = S_eff;

		out.material_tags = 0;
		rng.set_seed(mix_stream(bp.seed, bp.stream_ids.materials));
		if (rng.next_double() < bp.material_weight_iron_rich) out.material_tags |= static_cast<uint32_t>(MaterialTag::IronRich);
		if (rng.next_double() < bp.material_weight_high_silicate) out.material_tags |= static_cast<uint32_t>(MaterialTag::HighSilicate);
		if (rng.next_double() < bp.material_weight_ice) out.material_tags |= static_cast<uint32_t>(MaterialTag::IcePresent);
		if (rng.next_double() < bp.material_weight_organic) out.material_tags |= static_cast<uint32_t>(MaterialTag::Organic);
		if (rng.next_double() < bp.material_weight_volatile) out.material_tags |= static_cast<uint32_t>(MaterialTag::VolatileRich);
		if (rng.next_double() < bp.material_weight_low_metal) out.material_tags |= static_cast<uint32_t>(MaterialTag::LowMetal);

		if (meets_constraints(out, bp.constraints))
			return out;
	}

	return out;
}

} // namespace growth
