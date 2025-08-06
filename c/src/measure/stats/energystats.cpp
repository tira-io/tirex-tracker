#include "energystats.hpp"

#include "../../logging.hpp"

using namespace std::string_literals;

using tirex::EnergyStats;
using tirex::Stats;

template <typename C, typename K, typename V>
static V getOrDefault(const C& m, K const& key, const V& defval) {
	auto it = m.find(key);
	if (it == m.end())
		return defval;
	return it->second;
}

const char* EnergyStats::version = nullptr;
/** \todo check which are available and only report thos that are available to be supported **/
const std::set<tirexMeasure> EnergyStats::measures{
		TIREX_CPU_ENERGY_SYSTEM_JOULES, TIREX_RAM_ENERGY_SYSTEM_JOULES, TIREX_GPU_ENERGY_SYSTEM_JOULES
};

EnergyStats::EnergyStats() : tracker() {}

std::set<tirexMeasure> EnergyStats::providedMeasures() noexcept {
	std::set<tirexMeasure> measures{};
	auto cap = tracker.getCapabilities();
	if (cap & cppjoules::Capability::CPU_PROFILE)
		measures.insert(TIREX_CPU_ENERGY_SYSTEM_JOULES);
	if (cap & cppjoules::Capability::RAM_PROFILE)
		measures.insert(TIREX_RAM_ENERGY_SYSTEM_JOULES);
	if (cap & cppjoules::Capability::GPU_PROFILE)
		measures.insert(TIREX_GPU_ENERGY_SYSTEM_JOULES);
	return measures;
}

void EnergyStats::start() { tracker.start(); }
void EnergyStats::stop() { tracker.stop(); }
Stats EnergyStats::getStats() {
	auto results = tracker.calculate_energy().energy;
	for (auto& [device, result] : results)
		tirex::log::debug("cppjoules", "[{}] {}", device, result);
	// Divide by 1000'000 since CPPJoules reports micro Joule (uJ)
	return makeFilteredStats(
			enabled,
			std::pair{TIREX_CPU_ENERGY_SYSTEM_JOULES, std::to_string(getOrDefault(results, "core-0", 0) / 1000'000)},
			std::pair{TIREX_RAM_ENERGY_SYSTEM_JOULES, std::to_string(getOrDefault(results, "dram-0", 0) / 1000'000)},
			std::pair{TIREX_GPU_ENERGY_SYSTEM_JOULES, std::to_string(getOrDefault(results, "nvidia_gpu_0", 0) / 1000)}
	);
}