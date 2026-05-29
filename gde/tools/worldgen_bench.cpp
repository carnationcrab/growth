// Headless planet globe pipeline benchmark (no Godot). Build: scons bin/worldgen_bench
#include "world/PlanetGlobeGenerator.hpp"
#include "world/PlanetTerrainMesh.hpp"
#include "world/SphereDual.hpp"
#include "world/SphereHalfEdgeMesh.hpp"
#include "util/CsrGraph.hpp"
#include "gen/PlanetGenome.hpp"
#include "gen/WorldSeed.hpp"
#include "util/Parallel.hpp"
#include "util/PerformanceLogger.hpp"
#include "Types.hpp"
#include "base/gateway/Calgorithm.hpp"
#include "base/gateway/Cmath.hpp"
#include "base/gateway/Ccstdlib.hpp"
#include "base/gateway/Ccstring.hpp"
#include "base/gateway/Ciomanip.hpp"
#include "base/gateway/Ciostream.hpp"
#include "base/gateway/Csstream.hpp"
#include "base/gateway/Cstring.hpp"
#include "base/gateway/Cvector.hpp"

using namespace growth;

namespace {

U64 fnv1a(const void *data, Size len) {
	const unsigned char *p = static_cast<const unsigned char *>(data);
	U64 h = 0xcbf29ce484222325ull;
	for (Size i = 0; i < len; ++i) {
		h ^= p[i];
		h *= 0x100000001b3ull;
	}
	return h;
}

template <typename T>
U64 hash_vec(const Vector<T> &v) {
	return fnv1a(v.data(), v.size() * sizeof(T));
}

String hex64(U64 h) {
	static const char k_hex[] = "0123456789abcdef";
	char buf[17];
	for (int i = 15; i >= 0; --i) {
		buf[i] = k_hex[h & 0xf];
		h >>= 4;
	}
	buf[16] = '\0';
	return String(buf);
}

} // namespace

namespace {

void validate_region_rings(const PlanetGlobe &globe) {
	const auto &tris = globe.topology.triangles;
	Vector<Vector<Size>> rings;
	SphereDual::region_triangle_rings_for_globe(globe, rings);

	Vector<Size> tri_use(tris.size(), 0);
	Size bad_adjacency = 0;
	Size incomplete    = 0;

	auto shares_edge = [&](Size t0, Size t1) {
		if (t0 >= tris.size() || t1 >= tris.size() || t0 == t1)
			return false;
		const auto &a = tris[t0];
		const auto &b = tris[t1];
		int shared      = 0;
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				if (a[static_cast<Size>(i)] == b[static_cast<Size>(j)])
					++shared;
		return shared == 2;
	};

	for (const Vector<Size> &ring : rings) {
		if (ring.size() < 2) {
			++incomplete;
			continue;
		}
		for (Size t : ring)
			if (t < tris.size())
				++tri_use[t];
		for (Size j = 0; j < ring.size(); ++j) {
			const Size t0 = ring[j];
			const Size t1 = ring[(j + 1) % ring.size()];
			if (!shares_edge(t0, t1))
				++bad_adjacency;
		}
	}

	Size wrong_tri_count = 0;
	for (Size use : tri_use)
		if (use != 3)
			++wrong_tri_count;

	Ciostream::cout() << "  [validate] region_rings incomplete=" << incomplete
	                  << " non_adjacent_steps=" << bad_adjacency
	                  << " tris_not_in_3_rings=" << wrong_tri_count << '\n';
	if (bad_adjacency > 0 || wrong_tri_count > 0)
		Ciostream::cerr() << "  [validate] WARN: region triangle rings are not a closed dual mesh\n";
}

void analyze_elevation_mesh(const PlanetGlobe &globe, const PlanetTerrainMesh *terrain) {
	const Vector<float> &elev = globe.region_elevation.region_elevation;
	const Size num_regions    = elev.size();
	if (num_regions == 0)
		return;

	float max_step = 0.0f;
	Size steep_edges = 0;
	const CsrGraph &nb = globe.region_neighbours;
	for (Size r = 0; r < num_regions; ++r) {
		const float er = elev[r];
		const U32 *b   = nb.neighbours_begin(r);
		const U32 *e   = nb.neighbours_end(r);
		for (const U32 *it = b; it != e; ++it) {
			const Size n = static_cast<Size>(*it);
			if (n >= num_regions || n <= r)
				continue;
			const float step = Cmath::abs(er - elev[n]);
			if (step > max_step)
				max_step = step;
			if (step > 0.35f)
				++steep_edges;
		}
	}

	float max_tri_fold = 0.0f;
	if (terrain != nullptr && terrain->indices.size() >= 3) {
		const Size nV = terrain->vertices.size();
		for (Size t = 0; t < terrain->indices.size() / 3; ++t) {
			const Size i0 = terrain->indices[t * 3 + 0];
			const Size i1 = terrain->indices[t * 3 + 1];
			const Size i2 = terrain->indices[t * 3 + 2];
			if (i0 >= nV || i1 >= nV || i2 >= nV)
				continue;
			const float e0 = terrain->vertices[i0].length();
			const float e1 = terrain->vertices[i1].length();
			const float e2 = terrain->vertices[i2].length();
			const float spread = Calgorithm::max(Cmath::abs(e0 - e1),
				Calgorithm::max(Cmath::abs(e1 - e2), Cmath::abs(e0 - e2)));
			if (spread > max_tri_fold)
				max_tri_fold = spread;
		}
	}

	Ciostream::cout() << "  [elev] plates=" << globe.plates.num_plates
	                  << " max_region_step=" << max_step
	                  << " steep_region_edges=" << steep_edges
	                  << " max_tri_radius_spread=" << max_tri_fold << '\n';
	if (max_step > 0.5f || steep_edges > num_regions / 4)
		Ciostream::cerr() << "  [elev] WARN: many steep region neighbours (check mountain seed scatter)\n";
	if (max_tri_fold > 0.08f)
		Ciostream::cerr() << "  [elev] WARN: dual-mesh triangles fold sharply (Delaunay x per-region height)\n";
}

void analyze_region_drainage(const PlanetGlobe &globe) {
	const Vector<float> &elev = globe.region_elevation.region_elevation;
	const Size num_regions    = elev.size();
	if (num_regions == 0)
		return;

	const CsrGraph &nb = globe.region_neighbours;
	Size land_local_minima = 0;
	Size land_regions      = 0;
	for (Size r = 0; r < num_regions; ++r) {
		if (elev[r] < 0.0f)
			continue;
		++land_regions;
		bool has_lower = false;
		for (const U32 n : nb.neighbours_of(r)) {
			if (static_cast<Size>(n) >= num_regions)
				continue;
			if (elev[n] < elev[r]) {
				has_lower = true;
				break;
			}
		}
		if (!has_lower)
			++land_local_minima;
	}

	const Vector<float> &tri_elev = globe.triangle_values.triangle_elevation;
	const Vector<size_t> &downflow = globe.river_flow.t_downflow_s;
	const Size num_tri            = tri_elev.size();
	Size land_triangles           = 0;
	Size land_missing_downflow    = 0;
	for (Size t = 0; t < num_tri; ++t) {
		const float e_t = (t < tri_elev.size()) ? tri_elev[t] : 0.0f;
		if (e_t < 0.0f)
			continue;
		++land_triangles;
		const bool missing = t >= downflow.size() || downflow[t] == SphereHalfEdgeMesh::k_invalid;
		if (missing)
			++land_missing_downflow;
	}

	Ciostream::cout() << "  [drain] land_regions=" << land_regions
	                  << " land_local_minima=" << land_local_minima
	                  << " land_triangles=" << land_triangles
	                  << " land_missing_downflow=" << land_missing_downflow << '\n';
	if (land_local_minima > 0)
		Ciostream::cerr() << "  [drain] WARN: land regions with no lower neighbour (trapped basins on region graph)\n";
	if (land_triangles > 0 && land_missing_downflow > land_triangles / 20)
		Ciostream::cerr() << "  [drain] WARN: many land triangles lack river downflow\n";
}

constexpr double k_max_inward_tri_pct = 5.0;

/// Fraction of triangles whose geometric normal points toward the origin (0–100).
double terrain_mesh_inward_tri_percent(const PlanetTerrainMesh &mesh) {
	const Size mesh_tris  = mesh.indices.size() / 3;
	const Size mesh_verts = mesh.vertices.size();
	if (mesh_tris == 0)
		return 0.0;

	Size inverted = 0;
	for (Size t = 0; t < mesh_tris; ++t) {
		const Size i0 = mesh.indices[t * 3 + 0];
		const Size i1 = mesh.indices[t * 3 + 1];
		const Size i2 = mesh.indices[t * 3 + 2];
		if (i0 >= mesh_verts || i1 >= mesh_verts || i2 >= mesh_verts)
			continue;
		const Vec3 &a = mesh.vertices[i0];
		const Vec3 &b = mesh.vertices[i1];
		const Vec3 &c = mesh.vertices[i2];
		const Vec3 centre = (a + b + c) * (1.0 / 3.0);
		const Vec3 n      = (b - a).cross(c - a);
		if (n.dot(centre) < 0.0)
			++inverted;
	}
	return 100.0 * static_cast<double>(inverted) / static_cast<double>(mesh_tris);
}

bool validate_terrain_mesh(Size regions, Size triangles, const PlanetTerrainMesh &mesh, bool gate) {
	const Size mesh_tris  = mesh.indices.size() / 3;
	const Size mesh_verts = mesh.vertices.size();
	const Size expected_verts = regions + triangles;
	bool failed               = false;

	Ciostream::cout() << "  [validate] expected_verts=" << expected_verts
	                  << " actual_verts=" << mesh_verts
	                  << " mesh_tris=" << mesh_tris << '\n';

	if (mesh_tris > 0 && mesh_verts == mesh_tris * 3) {
		Ciostream::cerr() << "  [validate] WARN: verts == 3 * tris (duplicated per-triangle pattern; rebuild GDExtension)\n";
	}

	if (mesh_verts != expected_verts) {
		Ciostream::cerr() << "  [validate] WARN: verts != regions + triangles (shared dual-cell fan expects equality)\n";
	}

	const double pct = terrain_mesh_inward_tri_percent(mesh);
	if (mesh_tris > 0) {
		Ciostream::cout() << "  [validate] inward_tris="
		                  << static_cast<Size>(pct * static_cast<double>(mesh_tris) / 100.0 + 0.5)
		                  << " (" << pct << "%)\n";
		if (pct > k_max_inward_tri_pct) {
			Ciostream::cerr() << "  [validate] " << (gate ? "FAIL" : "WARN")
			                  << ": >" << k_max_inward_tri_pct << "% triangles face inward\n";
			if (gate)
				failed = true;
		}
	}
	return !failed;
}

bool run_case(Size target_regions, Size sim_regions, double jitter_percent, bool with_terrain_mesh,
              bool gate_inward) {
	WorldSeed world_seed;
	world_seed.value = 0xdeadbeef42ULL;

	PlanetGenome planet_genome;
	planet_genome.seed = world_seed.value;

	PlanetGlobe globe;
	PlanetTerrainMesh terrain_out;
	PlanetGlobeGenerator generator;
	perf::PerformanceLogger perf;

	generator.run(world_seed, planet_genome, target_regions, jitter_percent, 16,
	              0.5, 0.5, globe, with_terrain_mesh, with_terrain_mesh ? &terrain_out : nullptr, nullptr,
	              "default_globe", &perf, sim_regions);

	const Size regions   = globe.topology.sites.size();
	const Size triangles = globe.topology.triangles.size();
	const Size sides     = globe.mesh.num_sides();

	Ciostream::cout() << Ciomanip::fixed() << Ciomanip::setprecision(2);
	Ciostream::cout() << "target_regions=" << target_regions;
	if (sim_regions > 0 && sim_regions < target_regions)
		Ciostream::cout() << " sim_regions=" << sim_regions;
	Ciostream::cout() << " actual_regions=" << regions
	                  << " triangles=" << triangles
	                  << " sides=" << sides
	                  << " elev_samples=" << globe.region_elevation.region_elevation.size();
	Ciostream::cout() << "\n  [hash] elev=" << hex64(hash_vec(globe.region_elevation.region_elevation))
	                  << " moist=" << hex64(hash_vec(globe.region_moisture.region_moisture))
	                  << " sflow=" << hex64(hash_vec(globe.river_flow.s_flow))
	                  << " r2p=" << hex64(hash_vec(globe.plates.region_to_plate));
	analyze_elevation_mesh(globe, with_terrain_mesh ? &terrain_out : nullptr);
	analyze_region_drainage(globe);
	if (with_terrain_mesh) {
		const Size mesh_tris = terrain_out.indices.size() / 3;
		Ciostream::cout() << " terrain_verts=" << terrain_out.vertices.size()
		                  << " terrain_tris=" << mesh_tris;
		Ciostream::cout() << '\n';
		if (!validate_terrain_mesh(regions, triangles, terrain_out, gate_inward))
			return false;
	} else {
		Ciostream::cout() << '\n';
	}
	validate_region_rings(globe);
	for (const String &line : perf.format_report())
		Ciostream::cout() << line << '\n';
	Ciostream::cout() << '\n';
	return true;
}

} // namespace

int main(int argc, char **argv) {
	parallel::RuntimeConfig cfg;
	if (argc >= 2) {
		int w = Ccstdlib::atoi(argv[1]);
		if (w < 0) w = 0;
		cfg.worker_count_override = static_cast<U32>(w);
	}
	parallel::configure(cfg);
	Ciostream::cout() << "[worldgen_bench] worker_count_override=" << cfg.worker_count_override
	                  << " (0 = hardware default)\n";

	if (argc >= 3 && String(argv[2]) == "det") {
		const Size target = 16384;
		const U32 workers[] = { 1u, 0u };
		String hashes[2];
		for (int pass = 0; pass < 2; ++pass) {
			parallel::RuntimeConfig cfg;
			cfg.profile = parallel::ExecutionProfile::MaxPerformance;
			cfg.worker_count_override = workers[pass];
			parallel::configure(cfg);
			Ciostream::cerr() << "[det] pass=" << pass << " workers=" << workers[pass] << " target=" << target << '\n';
			WorldSeed world_seed;
			world_seed.value = 0xdeadbeef42ULL;
			PlanetGenome planet_genome;
			planet_genome.seed = world_seed.value;
			PlanetGlobe globe;
			perf::PerformanceLogger perf;
			PlanetGlobeGenerator generator;
			generator.run(world_seed, planet_genome, target, 0.0, 16, 0.5f, 0.5f, globe, false, nullptr, nullptr,
			              "default_globe", &perf, 0);
			OStringStream os;
			os << hex64(hash_vec(globe.region_elevation.region_elevation)) << ':'
			   << hex64(hash_vec(globe.region_moisture.region_moisture)) << ':'
			   << hex64(hash_vec(globe.river_flow.s_flow)) << ':'
			   << hex64(hash_vec(globe.plates.region_to_plate));
			hashes[pass] = os.str();
			Ciostream::cout() << "[det] hash=" << hashes[pass] << '\n';
		}
		if (hashes[0] != hashes[1]) {
			Ciostream::cerr() << "[det] FAIL: hashes differ between worker counts\n";
			return 1;
		}
		Ciostream::cout() << "[det] OK: bit-identical across worker counts\n";
		return 0;
	}

	if (argc >= 3 && (String(argv[2]) == "fast" || String(argv[2]) == "100k")) {
		Ciostream::cout() << "[worldgen_bench] RBG-scale path (100000 fine, 50000 coarse, 25% jitter, mesh)\n";
		return run_case(100000, 50000, 25.0, true, false) ? 0 : 1;
	}

	// SC-E2 / OW-E12 CI gate: coarse sim + field upsample reference preset only.
	if (argc >= 3 && String(argv[2]) == "gate") {
		Ciostream::cout() << "[worldgen_bench] SC-E2 gate: default_globe coarse upsample "
		                     "(target=100000 sim=50000 jitter=25% mesh, inward_tris <= "
		                  << k_max_inward_tri_pct << "%)\n";
		const bool ok = run_case(100000, 50000, 25.0, true, true);
		if (!ok) {
			Ciostream::cerr() << "[worldgen_bench] gate FAIL\n";
			return 1;
		}
		Ciostream::cout() << "[worldgen_bench] gate OK\n";
		return 0;
	}

	Ciostream::cout() << "[worldgen_bench] default_globe pipeline, double precision\n";
	const Size targets[] = { 4096, 16384, 100000, 262144, 524288 };
	for (Size n : targets) {
		Ciostream::cerr() << "[case] target=" << n << " no_mesh\n";
		run_case(n, 0, 0.0, false, false);
	}
	Ciostream::cout() << "[worldgen_bench] with dual_cell_fan terrain_mesh\n";
	{ Ciostream::cerr() << "[case] target=16384 mesh\n"; run_case(16384, 0, 0.0, true, false); }
	{ Ciostream::cerr() << "[case] target=100000 mesh\n"; run_case(100000, 0, 0.0, true, false); }
	{ Ciostream::cerr() << "[case] target=262144 mesh\n"; run_case(262144, 0, 0.0, true, false); }
	{ Ciostream::cerr() << "[case] target=524288 mesh\n"; run_case(524288, 0, 0.0, true, false); }
	Ciostream::cout() << "[worldgen_bench] coarse sim + upsample\n";
	{ Ciostream::cerr() << "[case] target=100000 sim=50000 mesh jitter=25%\n";
	  run_case(100000, 50000, 25.0, true, false); }
	{ Ciostream::cerr() << "[case] target=524288 sim=16384 mesh\n";
	  run_case(524288, 16384, 0.0, true, false); }
	return 0;
}
