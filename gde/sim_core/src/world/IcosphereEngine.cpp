#include "world/IcosphereEngine.hpp"
#include "Types.hpp"
#include "base/gateway/Cunordered_map.hpp"
#include "base/gateway/Cutility.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

namespace {

constexpr double k_phi = 1.6180339887498948482;

U64 edge_key(Size a, Size b) {
	if (a > b) Cutility::swap(a, b);
	return (static_cast<U64>(a) << 32) | static_cast<U64>(b);
}

Vec3 normalised_vertex(double x, double y, double z) {
	Vec3 v(x, y, z);
	return v.normalised();
}

} // namespace

Size IcosphereEngine::vertex_count_for_subdivision(int subdivision) {
	if (subdivision < 0) subdivision = 0;
	const Size pow4 = static_cast<Size>(1) << (2 * subdivision);
	return 2 + 10 * pow4;
}

Size IcosphereEngine::triangle_count_for_subdivision(int subdivision) {
	if (subdivision < 0) subdivision = 0;
	const Size pow4 = static_cast<Size>(1) << (2 * subdivision);
	return 20 * pow4;
}

int IcosphereEngine::subdivision_for_target(Size target_vertices) {
	int level = 0;
	while (level < 9 && vertex_count_for_subdivision(level) < target_vertices)
		++level;
	return level;
}

bool IcosphereEngine::fine_embeds_coarse(Size fine_vertices, Size coarse_vertices) {
	if (coarse_vertices == 0 || fine_vertices < coarse_vertices)
		return false;
	const int fine_sub   = subdivision_for_target(fine_vertices);
	const int coarse_sub = subdivision_for_target(coarse_vertices);
	if (fine_sub < coarse_sub)
		return false;
	return vertex_count_for_subdivision(coarse_sub) <= fine_vertices;
}

void IcosphereEngine::build(int subdivision, SphereTopology &out) {
	out.sites.clear();
	out.triangles.clear();
	if (subdivision < 0) subdivision = 0;

	out.sites.reserve(vertex_count_for_subdivision(subdivision));
	out.triangles.reserve(triangle_count_for_subdivision(subdivision));

	const double t  = k_phi;
	const double t2 = 1.0 / t;
	out.sites.push_back(normalised_vertex(-1, t, 0));
	out.sites.push_back(normalised_vertex(1, t, 0));
	out.sites.push_back(normalised_vertex(-1, -t, 0));
	out.sites.push_back(normalised_vertex(1, -t, 0));
	out.sites.push_back(normalised_vertex(0, -1, t));
	out.sites.push_back(normalised_vertex(0, 1, t));
	out.sites.push_back(normalised_vertex(0, -1, -t));
	out.sites.push_back(normalised_vertex(0, 1, -t));
	out.sites.push_back(normalised_vertex(t, 0, -1));
	out.sites.push_back(normalised_vertex(t, 0, 1));
	out.sites.push_back(normalised_vertex(-t, 0, -1));
	out.sites.push_back(normalised_vertex(-t, 0, 1));

	Vector<Array<Size, 3>> faces = {
		{{0, 11, 5}}, {{0, 5, 1}}, {{0, 1, 7}}, {{0, 7, 10}}, {{0, 10, 11}},
		{{1, 5, 9}}, {{5, 11, 4}}, {{11, 10, 2}}, {{10, 7, 6}}, {{7, 1, 8}},
		{{3, 9, 4}}, {{3, 4, 2}}, {{3, 2, 6}}, {{3, 6, 8}}, {{3, 8, 9}},
		{{4, 9, 5}}, {{2, 4, 11}}, {{6, 2, 10}}, {{8, 6, 7}}, {{9, 8, 1}},
	};

	UnorderedMap<U64, Size> midpoint_cache;
	midpoint_cache.reserve(triangle_count_for_subdivision(subdivision));
	for (int level = 0; level < subdivision; ++level) {
		midpoint_cache.clear();
		midpoint_cache.reserve(faces.size() * 2);
		Vector<Array<Size, 3>> next_faces;
		next_faces.reserve(faces.size() * 4);

		auto midpoint = [&](Size a, Size b) -> Size {
			const U64 key = edge_key(a, b);
			const auto it      = midpoint_cache.find(key);
			if (it != midpoint_cache.end())
				return it->second;
			const Vec3 &va = out.sites[a];
			const Vec3 &vb = out.sites[b];
			const Size idx =
				out.sites.size();
			out.sites.push_back(normalised_vertex(va.x + vb.x, va.y + vb.y, va.z + vb.z));
			midpoint_cache.emplace(key, idx);
			return idx;
		};

		for (const auto &f : faces) {
			const Size a = f[0];
			const Size b = f[1];
			const Size c = f[2];
			const Size ab = midpoint(a, b);
			const Size bc = midpoint(b, c);
			const Size ca = midpoint(c, a);
			next_faces.push_back({{a, ab, ca}});
			next_faces.push_back({{b, bc, ab}});
			next_faces.push_back({{c, ca, bc}});
			next_faces.push_back({{ab, bc, ca}});
		}
		faces = Cutility::move(next_faces);
	}

	out.triangles = Cutility::move(faces);
}

} // namespace growth
