#include "energystats.hpp"

#include "../../logging.hpp"

using namespace std::string_literals;

using tirex::EnergyStats;
using tirex::Stats;

template <typename C, typename K, typename V>
V getOrDefault(const C& m, K const& key, const V& defval) {
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

void EnergyStats::start() { tracker.start(); }
void EnergyStats::stop() { tracker.stop(); }
Stats EnergyStats::getStats() {
	/** \todo: filter by requested metrics */
	auto results = tracker.calculate_energy().energy;
	for (auto& [device, result] : results)
		tirex::log::debug("cppjoules", "[{}] {}", device, result);
	return {{TIREX_CPU_ENERGY_SYSTEM_JOULES, std::to_string(getOrDefault(results, "core-0", 0))},
			{TIREX_RAM_ENERGY_SYSTEM_JOULES, std::to_string(getOrDefault(results, "dram-0", 0))},
			{TIREX_GPU_ENERGY_SYSTEM_JOULES, std::to_string(getOrDefault(results, "nvidia_gpu_0", 0))}};
}