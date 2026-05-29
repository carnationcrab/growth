#pragma once

#include "PlanetGenome.hpp"
#include "base/gateway/Cmath.hpp"

namespace growth {

namespace planet_constants {
	constexpr double pi = 3.141592653589793;
	constexpr double G = 6.67430e-11;           // m^3 kg^-1 s^-2
	constexpr double sigma_sb = 5.670374419e-8; // W m^-2 K^-4 (Stefan-Boltzmann)
	constexpr double R_gas = 8.314462618;        // J mol^-1 K^-1
	constexpr double two_pi = 2.0 * pi;
}

/// Wraps a genome and computes derived physical properties (phenotype). No stored state beyond the genome.
class Planet {
public:
	explicit Planet(const PlanetGenome &genome) : g_(genome) {}

	const PlanetGenome &genome() const { return g_; }

	/// Stored effective insolation (Earth = 1). Use for climate anchor.
	double S_eff() const { return g_.S_eff > 0.0 ? g_.S_eff : (insolation() / 1361.0); }
	/// Planet-level precipitation [0, 1] from genome.
	double precipitation() const { return g_.precipitation; }
	/// Material tags bitfield from genome.
	uint32_t material_tags() const { return g_.material_tags; }

	/// Surface gravity (m/s^2).
	double g() const {
		return planet_constants::G * g_.M_p / (g_.R_p * g_.R_p);
	}

	/// Bulk density (kg/m^3).
	double density() const {
		double vol = (4.0 / 3.0) * planet_constants::pi * g_.R_p * g_.R_p * g_.R_p;
		return g_.M_p / vol;
	}

	/// Escape velocity (m/s).
	double v_escape() const {
		return Cmath::sqrt(2.0 * planet_constants::G * g_.M_p / g_.R_p);
	}

	/// Orbital period (s). Uses Kepler: P = 2*pi*sqrt(a^3 / (G*M_star)) with M_p neglected.
	double P_orb() const {
		double mu = planet_constants::G * (mass_star_approx() + g_.M_p);
		return planet_constants::two_pi * Cmath::sqrt(g_.a * g_.a * g_.a / mu);
	}

	/// Approximate stellar mass (kg) from L_star via rough mass–luminosity (L ~ M^3.5 for main sequence).
	double mass_star_approx() const {
		constexpr double L_sun = 3.828e26;
		double x = g_.L_star / L_sun;
		return 1.9885e30 * Cmath::pow(x, 1.0 / 3.5);  // Sun mass
	}

	/// Time-averaged insolation (W/m^2) at top of atmosphere. (1-e^2)^(-1/2) for eccentricity.
	double insolation() const {
		double flux = g_.L_star / (4.0 * planet_constants::pi * g_.a * g_.a);
		return flux / Cmath::sqrt(1.0 - g_.e * g_.e);
	}

	/// Equilibrium temperature (K). Uses stored S_eff when set, else insolation from L_star/a.
	double T_eq() const {
		constexpr double S_earth = 1361.0;  // W/m^2
		double S = (g_.S_eff > 0.0) ? (g_.S_eff * S_earth) : insolation();
		return Cmath::pow(S * (1.0 - g_.albedo) / (4.0 * planet_constants::sigma_sb), 0.25);
	}

	/// Simple surface temperature estimate (K): T_eq scaled by greenhouse factor.
	double T_surf_estimate() const {
		return T_eq() * Cmath::sqrt(Cmath::sqrt(g_.greenhouse_factor));
	}

	/// Scale height (m): H = R*T / (mu*g).
	double scale_height() const {
		double T = T_surf_estimate();
		return (planet_constants::R_gas / 1000.0) * T / (g_.mean_molecular_weight * g());
	}

	/// Surface area (m^2).
	double surface_area() const {
		return 4.0 * planet_constants::pi * g_.R_p * g_.R_p;
	}

	/// Volume (m^3).
	double volume() const {
		return (4.0 / 3.0) * planet_constants::pi * g_.R_p * g_.R_p * g_.R_p;
	}

private:
	PlanetGenome g_;
};

} // namespace growth
