#include "WorldGenMarshal.hpp"


#include <godot_cpp/variant/array.hpp>

#include <godot_cpp/variant/packed_float64_array.hpp>

#include <godot_cpp/variant/packed_int32_array.hpp>

#include <godot_cpp/variant/packed_vector3_array.hpp>

#include <godot_cpp/variant/variant.hpp>

#include <godot_cpp/variant/vector3.hpp>

#include "world/RiverFlow.hpp"

#include "world/SphereHalfEdgeMesh.hpp"

#include "world/WorldGenPreviewExport.hpp"
#include "base/gateway/Carray.hpp"
#include "base/gateway/Ccstring.hpp"



namespace growth_sim {



namespace {



using namespace godot;
using growth::Vector;



PackedFloat64Array pack_doubles(const Vector<float> &values) {

	PackedFloat64Array arr;

	const int64_t n = static_cast<int64_t>(values.size());

	arr.resize(n);

	if (n > 0) {

		double *dst = arr.ptrw();

		for (int64_t i = 0; i < n; ++i)

			dst[i] = values[static_cast<size_t>(i)];

	}

	return arr;

}



PackedVector3Array pack_vec3(const Vector<growth::Vec3> &values) {

	PackedVector3Array arr;

	const int64_t n = static_cast<int64_t>(values.size());

	arr.resize(n);

	if (n > 0) {

		Vector3 *dst = arr.ptrw();

		for (int64_t i = 0; i < n; ++i) {

			const growth::Vec3 &v = values[static_cast<size_t>(i)];

			dst[i]                = Vector3(v.x, v.y, v.z);

		}

	}

	return arr;

}



/// Godot Array for preview dicts (GDScript / C# autoload forward reliably; Packed* can be dropped on the boundary).

Array pack_vec3_godot_array(const Vector<growth::Vec3> &values) {

	Array arr;

	const int64_t n = static_cast<int64_t>(values.size());

	arr.resize(n);

	for (int64_t i = 0; i < n; ++i) {

		const growth::Vec3 &v = values[static_cast<size_t>(i)];

		arr.set(i, Vector3(v.x, v.y, v.z));

	}

	return arr;

}



Array pack_u32_godot_array(const Vector<uint32_t> &values) {

	Array arr;

	const int64_t n = static_cast<int64_t>(values.size());

	arr.resize(n);

	for (int64_t i = 0; i < n; ++i)

		arr.set(i, static_cast<int32_t>(values[static_cast<size_t>(i)]));

	return arr;

}



PackedInt32Array pack_i32(const Vector<growth::I32> &values) {

	PackedInt32Array arr;

	const int64_t n = static_cast<int64_t>(values.size());

	arr.resize(n);

	if (n > 0) {

		int32_t *dst = arr.ptrw();

		for (int64_t i = 0; i < n; ++i)

			dst[i] = values[static_cast<size_t>(i)];

	}

	return arr;

}



bool preset_uses_floating_islands(const growth::ParsedWorldGenForm &form) {

	return form.planet_preset == "FloatingIslands";

}



void marshal_terrain_mesh(

	const growth::PlanetTerrainMesh &mesh,

	Dictionary &out,

	const godot::String &prefix,

	const Vector<growth::U32> *triangle_region = nullptr) {

	if (mesh.vertices.empty())

		return;

	out[godot::String(prefix) + "_vertices"] = pack_vec3_godot_array(mesh.vertices);

	out[godot::String(prefix) + "_normals"]  = pack_vec3_godot_array(mesh.normals);

	out[godot::String(prefix) + "_indices"]  = pack_u32_godot_array(mesh.indices);

	out[godot::String(prefix) + "_river_strength"] = pack_doubles(mesh.river_strength);

	if (!mesh.vertex_elevation.empty())
		out[godot::String(prefix) + "_vertex_elevation"] = pack_doubles(mesh.vertex_elevation);
	if (!mesh.vertex_moisture.empty())
		out[godot::String(prefix) + "_vertex_moisture"] = pack_doubles(mesh.vertex_moisture);

	if (triangle_region != nullptr && !triangle_region->empty()) {

		PackedInt32Array region_per_tri;

		region_per_tri.resize(static_cast<int64_t>(triangle_region->size()));

		int32_t *dst = region_per_tri.ptrw();

		for (size_t i = 0; i < triangle_region->size(); ++i)

			dst[static_cast<int64_t>(i)] = static_cast<int32_t>((*triangle_region)[i]);

		out[godot::String(prefix) + "_triangle_region"] = region_per_tri;

	}

}



void marshal_debug_half_edge(const growth::SphereHalfEdgeMesh &mesh, Dictionary &out) {

	const size_t num_sides         = mesh.num_sides();

	constexpr int32_t mesh_invalid = -1;

	PackedInt32Array s_begin_r_arr, s_end_r_arr, s_inner_t_arr, s_outer_t_arr, s_next_s_arr, s_twin_s_arr;

	s_begin_r_arr.resize(static_cast<int64_t>(num_sides));

	s_end_r_arr.resize(static_cast<int64_t>(num_sides));

	s_inner_t_arr.resize(static_cast<int64_t>(num_sides));

	s_outer_t_arr.resize(static_cast<int64_t>(num_sides));

	s_next_s_arr.resize(static_cast<int64_t>(num_sides));

	s_twin_s_arr.resize(static_cast<int64_t>(num_sides));

	for (size_t i = 0; i < num_sides; ++i) {

		s_begin_r_arr.set(static_cast<int64_t>(i), static_cast<int32_t>(mesh.s_begin_r[i]));

		s_end_r_arr.set(static_cast<int64_t>(i), static_cast<int32_t>(mesh.s_end_r[i]));

		s_inner_t_arr.set(static_cast<int64_t>(i), static_cast<int32_t>(mesh.s_inner_t[i]));

		s_outer_t_arr.set(static_cast<int64_t>(i), mesh.s_outer_t[i] == growth::SphereHalfEdgeMesh::k_invalid ? mesh_invalid : static_cast<int32_t>(mesh.s_outer_t[i]));

		s_next_s_arr.set(static_cast<int64_t>(i), static_cast<int32_t>(mesh.s_next_s[i]));

		s_twin_s_arr.set(static_cast<int64_t>(i), mesh.s_twin_s[i] == growth::SphereHalfEdgeMesh::k_invalid ? mesh_invalid : static_cast<int32_t>(mesh.s_twin_s[i]));

	}

	out["mesh_s_begin_r"] = s_begin_r_arr;

	out["mesh_s_end_r"]   = s_end_r_arr;

	out["mesh_s_inner_t"] = s_inner_t_arr;

	out["mesh_s_outer_t"] = s_outer_t_arr;

	out["mesh_s_next_s"]  = s_next_s_arr;

	out["mesh_s_twin_s"]  = s_twin_s_arr;

}



void marshal_river_debug(const growth::RiverFlow &river, Dictionary &out) {

	PackedInt32Array t_downflow_s_arr;

	t_downflow_s_arr.resize(static_cast<int64_t>(river.t_downflow_s.size()));

	for (size_t i = 0; i < river.t_downflow_s.size(); ++i)

		t_downflow_s_arr.set(static_cast<int64_t>(i), river.t_downflow_s[i] == growth::RiverFlow::k_invalid ? -1 : static_cast<int32_t>(river.t_downflow_s[i]));

	out["river_t_downflow_s"] = t_downflow_s_arr;

	out["river_s_flow"]       = pack_doubles(river.s_flow);

}



} // namespace



void marshal_world_gen_job_result(

	const growth::WorldGenJobResult &job_result,

	Dictionary &out,

	PackedInt32Array &last_world_gen_triangles_out) {

	if (!job_result.ok)

		return;



	const growth::ParsedWorldGenForm &parsed = job_result.parsed;

	const growth::PlanetGlobe &globe         = job_result.globe;

	const growth::PlanetTerrainMesh &planet_terrain_mesh = job_result.planet_terrain_mesh;

	const bool floating_islands              = preset_uses_floating_islands(parsed);



	const growth::SphereTopology &topology = globe.topology;

	const size_t num_tri                   = topology.triangles.size();

	const int64_t flat_size                = static_cast<int64_t>(num_tri * 3);

	const auto &sites                      = topology.sites;



	out["sites"] = pack_vec3(sites);



	last_world_gen_triangles_out.resize(flat_size);

	for (size_t i = 0; i < num_tri; ++i) {

		const auto &t = topology.triangles[i];

		last_world_gen_triangles_out.set(static_cast<int64_t>(i * 3 + 0), static_cast<int32_t>(t[0]));

		last_world_gen_triangles_out.set(static_cast<int64_t>(i * 3 + 1), static_cast<int32_t>(t[1]));

		last_world_gen_triangles_out.set(static_cast<int64_t>(i * 3 + 2), static_cast<int32_t>(t[2]));

	}



	const size_t num_regions = topology.sites.size();

	if (growth::WorldGenPreviewExport::include_cell_rings(num_regions)) {

		Array tri_arr;

		tri_arr.resize(flat_size);

		for (int64_t i = 0; i < flat_size; ++i)

			tri_arr[i] = last_world_gen_triangles_out[i];

		out["triangles"] = tri_arr;

	}



	out["circumcenters"] = pack_vec3(globe.mesh.t_xyz);



	out["preview_cell_rings_included"] = growth::WorldGenPreviewExport::include_cell_rings(num_regions);

	if (growth::WorldGenPreviewExport::include_cell_rings(num_regions)) {

		const growth::CsrGraph &region_rings = globe.region_triangle_rings;

		const size_t num_rings = region_rings.num_nodes();

		Array cells_arr;

		cells_arr.resize(static_cast<int64_t>(num_rings));

		for (size_t i = 0; i < num_rings; ++i) {

			PackedInt32Array cell;

			const growth::U32 *cb = region_rings.neighbours_begin(i);

			const growth::U32 *ce = region_rings.neighbours_end(i);

			const size_t deg = static_cast<size_t>(ce - cb);

			cell.resize(static_cast<int64_t>(deg));

			int32_t *dst = cell.ptrw();

			for (size_t j = 0; j < deg; ++j)

				dst[static_cast<int64_t>(j)] = static_cast<int32_t>(cb[j]);

			cells_arr[static_cast<int64_t>(i)] = cell;

		}

		out["cells"] = cells_arr;

	}



	const growth::TectonicPlates &tectonic_plates = globe.plates;

	out["plate_regions"] = pack_i32(tectonic_plates.region_to_plate);



	const Vector<float> &region_elevation = globe.region_elevation.region_elevation;

	const Vector<float> &region_moisture  = globe.region_moisture.region_moisture;

	out["region_elevation"] = pack_doubles(region_elevation);

	out["region_moisture"]  = pack_doubles(region_moisture);



	out["triangle_elevation"] = pack_doubles(globe.triangle_values.triangle_elevation);

	out["triangle_moisture"]  = pack_doubles(globe.triangle_values.triangle_moisture);



	const size_t num_plates = tectonic_plates.num_plates;

	Vector<float> plate_elevation(num_plates, 0.0f);

	Vector<float> plate_moisture(num_plates, 0.5f);

	Vector<float> plate_elev_sum(num_plates, 0.0f);

	Vector<float> plate_moist_sum(num_plates, 0.0f);

	Vector<size_t> plate_count(num_plates, 0u);

	for (size_t i = 0; i < tectonic_plates.region_to_plate.size(); ++i) {

		int32_t p = tectonic_plates.region_to_plate[i];

		if (p < 0 || static_cast<size_t>(p) >= num_plates) continue;

		if (i < region_elevation.size()) plate_elev_sum[static_cast<size_t>(p)] += region_elevation[i];

		if (i < region_moisture.size()) plate_moist_sum[static_cast<size_t>(p)] += region_moisture[i];

		plate_count[static_cast<size_t>(p)]++;

	}

	for (size_t p = 0; p < num_plates; ++p) {

		if (plate_count[p] > 0) {

			plate_elevation[p] = plate_elev_sum[p] / static_cast<float>(plate_count[p]);

			plate_moisture[p]  = plate_moist_sum[p] / static_cast<float>(plate_count[p]);

		}

	}

	out["plate_elevation"] = pack_doubles(plate_elevation);

	out["plate_moisture"]  = pack_doubles(plate_moisture);



	if (growth::WorldGenPreviewExport::include_debug_mesh(num_regions)) {

		marshal_debug_half_edge(globe.mesh, out);

		marshal_river_debug(globe.river_flow, out);

	}



	if (parsed.use_planet_terrain_mesh || floating_islands) {

		out["planet_terrain_mesh_num_regions"] = static_cast<int64_t>(num_regions);

		Array topo_tri_arr;
		topo_tri_arr.resize(flat_size);
		for (int64_t i = 0; i < flat_size; ++i)
			topo_tri_arr[i] = last_world_gen_triangles_out[i];
		out["topology_triangles"] = topo_tri_arr;

		if (!planet_terrain_mesh.vertices.empty()) {

			const Vector<growth::U32> *tri_regions =

				floating_islands && !job_result.planet_terrain_triangle_region.empty() ? &job_result.planet_terrain_triangle_region : nullptr;

			marshal_terrain_mesh(planet_terrain_mesh, out, "planet_terrain_mesh", tri_regions);

		}
		if (!job_result.planet_terrain_mesh_pre_erosion.vertices.empty()) {
			marshal_terrain_mesh(job_result.planet_terrain_mesh_pre_erosion, out, "planet_terrain_mesh_pre_erosion", nullptr);
		}

		Vector<growth::Vec3> river_lines;
		growth::WorldGenPreviewExport::river_lines_for_preview(globe, river_lines);
		if (!river_lines.empty())
			out["planet_terrain_mesh_river_lines"] = pack_vec3_godot_array(river_lines);

	}

	out["use_planet_terrain_mesh"] = parsed.use_planet_terrain_mesh;

	out["planet_preset"]           = String(parsed.planet_preset.c_str());
	out["world_gen_mode"]          = String(parsed.world_gen_mode.c_str());
	out["use_simd"]                = parsed.use_simd;

	out["coarse_sim_applied"]      = parsed.sim_voronoi_sites > 0 && parsed.voronoi_sites > parsed.sim_voronoi_sites;
	const int64_t topology_sites_bytes = static_cast<int64_t>(globe.topology.sites.size() * sizeof(growth::Vec3));
	const int64_t topology_tris_bytes  = static_cast<int64_t>(globe.topology.triangles.size() * 3u * sizeof(growth::Size));
	const int64_t mesh_side_arrays     = static_cast<int64_t>(globe.mesh.num_sides() * sizeof(growth::Size) * 6u);
	const int64_t river_bytes          = static_cast<int64_t>(globe.river_flow.s_flow.size() * sizeof(float)
	                                            + globe.river_flow.t_downflow_s.size() * sizeof(growth::Size));
	out["alloc_stats_topology_bytes"] = topology_sites_bytes + topology_tris_bytes;
	out["alloc_stats_mesh_bytes"]     = mesh_side_arrays;
	out["alloc_stats_river_bytes"]    = river_bytes;

}



} // namespace growth_sim
