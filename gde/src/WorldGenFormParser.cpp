// World-gen form submit pipeline (1/4): parse Godot form dict into ParsedWorldGenForm (seed_form, preset, temperature, precipitation).
#include "WorldGenFormParser.hpp"
#include "gen/SeedGenerator.hpp"
#include "util/Random.hpp"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot {

namespace growth_sim {

void parse_world_gen_form(const Dictionary &form_dict, growth::ParsedWorldGenForm &out) {
	out.seed_form.clear();
	out.planet_preset = "Earthlike";
	out.temperature = 100.0;
	out.precipitation = 100.0;
	out.voronoi_sites = 256;
	out.jitter = 0.0;
	out.num_plate_regions = 25;
	if (form_dict.size() == 0) return;

	Array keys = form_dict.keys();
	for (int i = 0; i < keys.size(); ++i) {
		Variant k = keys[i];
		if (k.get_type() != Variant::STRING) continue;
		String key_str = k;
		std::string key(key_str.utf8().get_data());
		Variant v = form_dict.get(k, Variant());

		if (key == "planet_preset" && v.get_type() == Variant::STRING) {
			String s = v.operator String();
			if (!s.is_empty())
				out.planet_preset = std::string(s.utf8().get_data());
			continue;
		}
		if (key == "temperature" && (v.get_type() == Variant::INT || v.get_type() == Variant::FLOAT)) {
			out.temperature = v.get_type() == Variant::FLOAT ? v.operator double() : static_cast<double>(v.operator int64_t());
			continue;
		}
		if (key == "precipitation" && (v.get_type() == Variant::INT || v.get_type() == Variant::FLOAT)) {
			out.precipitation = v.get_type() == Variant::FLOAT ? v.operator double() : static_cast<double>(v.operator int64_t());
			continue;
		}
		if (key == "voronoi_sites" && (v.get_type() == Variant::INT || v.get_type() == Variant::FLOAT)) {
			int64_t n = v.get_type() == Variant::FLOAT ? static_cast<int64_t>(v.operator double()) : v.operator int64_t();
			if (n < 32) n = 32;
			if (n > 10000) n = 10000;
			out.voronoi_sites = static_cast<size_t>(n);
			continue;
		}
		if (key == "jitter" && (v.get_type() == Variant::INT || v.get_type() == Variant::FLOAT)) {
			double j = v.get_type() == Variant::FLOAT ? v.operator double() : static_cast<double>(v.operator int64_t());
			if (j < 0.0) j = 0.0;
			if (j > 100.0) j = 100.0;
			out.jitter = j;
			continue;
		}
		if (key == "num_plate_regions" && (v.get_type() == Variant::INT || v.get_type() == Variant::FLOAT)) {
			int64_t n = v.get_type() == Variant::FLOAT ? static_cast<int64_t>(v.operator double()) : v.operator int64_t();
			if (n < 10) n = 10;
			if (n > 50) n = 50;
			out.num_plate_regions = static_cast<size_t>(n);
			continue;
		}

		int64_t val = 0;
		if (v.get_type() == Variant::INT)
			val = (int64_t)v.operator int64_t();
		else if (v.get_type() == Variant::FLOAT)
			val = (int64_t)v.operator double();
		else if (v.get_type() == Variant::STRING) {
			String s = v.operator String();
			if (key == "seed" && s.is_empty())
				val = static_cast<int64_t>(growth::random::random_seed());
			else if (!s.is_empty())
				val = s.to_int();
		}
		out.seed_form[key] = val;
	}
}

} // namespace growth_sim

} // namespace godot
