#include "energystats.hpp"

using namespace std::string_literals;

using msr::EnergyStats;
using msr::Stats;

const char* EnergyStats::version = nullptr;
const std::set<msrMeasure> EnergyStats::measures{
		MSR_CPU_ENERGY_SYSTEM_JOULES, MSR_RAM_ENERGY_SYSTEM_JOULES, MSR_GPU_ENERGY_SYSTEM_JOULES
};

EnergyStats::EnergyStats() : tracker() {}

void EnergyStats::start() { tracker.start(); }
void EnergyStats::stop() { tracker.stop(); }
Stats EnergyStats::getStats() {
	/** \todo: filter by requested metrics */
	/*auto results = tracker.calculate_energy().energy;
	Stats stats{};
	for (auto& [device, result] : results)
		stats.insertChild(device, {std::to_string(result)});

	return {{"energy", stats}};*/
	return {{MSR_CPU_ENERGY_SYSTEM_JOULES, "TODO"s},
			{MSR_RAM_ENERGY_SYSTEM_JOULES, "TODO"s},
			{MSR_GPU_ENERGY_SYSTEM_JOULES, "TODO"s}};
}