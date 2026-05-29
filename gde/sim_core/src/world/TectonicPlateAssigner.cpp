#include "world/TectonicPlateAssigner.hpp"
#include "util/ParallelBfs.hpp"
#include "util/Random.hpp"
#include "base/gateway/Calgorithm.hpp"
#include "base/gateway/Cstddef.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

namespace {

constexpr uint64_t k_tectonic_stream = 0x7e4a9c2b1d5f8e3aULL;

} // namespace

void TectonicPlateAssigner::assign(const SphereTopology &topology,
                                   const CsrGraph &region_neighbours,
                                   const WorldSeed &world_seed,
                                   size_t num_plate_regions,
                                   TectonicPlates &out) const {
	const size_t num_regions = topology.sites.size();
	out.region_to_plate.clear();
	out.num_plates = 0;
	if (num_regions == 0)
		return;

	if (num_plate_regions < 1u)
		num_plate_regions = 1u;
	if (num_plate_regions > num_regions)
		num_plate_regions = num_regions;

	// Pick num_plate_regions distinct seed regions deterministically from the world seed.
	random::RNG rng(world_seed.value ^ k_tectonic_stream);
	Vector<U32> chosen;
	chosen.reserve(num_plate_regions);
	Vector<bool> picked(num_regions, false);
	while (chosen.size() < num_plate_regions) {
		const size_t r = static_cast<size_t>(rng.next_int(0, static_cast<int64_t>(num_regions) - 1));
		if (picked[r])
			continue;
		picked[r] = true;
		chosen.push_back(static_cast<U32>(r));
	}

	// Canonical seed order: sort by region id so the (sources, labels) pair fed to bfs_label is
	// independent of the order produced by the RNG. plate_id = position in the sorted list.
	Calgorithm::sort(chosen.begin(), chosen.end());

	Vector<U32> labels(chosen.size());
	for (size_t i = 0; i < chosen.size(); ++i)
		labels[i] = static_cast<U32>(i);

	Vector<U32> region_labels;
	parallel::bfs_label(region_neighbours, chosen, labels, region_labels);

	out.region_to_plate.assign(num_regions, -1);
	for (size_t r = 0; r < num_regions; ++r) {
		const U32 lab = (r < region_labels.size()) ? region_labels[r] : parallel::k_bfs_unlabeled;
		if (lab != parallel::k_bfs_unlabeled)
			out.region_to_plate[r] = static_cast<int32_t>(lab);
	}
	out.plate_seed_region = chosen;
	out.num_plates          = num_plate_regions;
}

} // namespace growth
