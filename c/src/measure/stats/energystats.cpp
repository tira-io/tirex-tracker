#include "energystats.hpp"

using namespace std::string_literals;

using tirex::EnergyStats;
using tirex::Stats;

const char* EnergyStats::version = nullptr;
const std::set<tirexMeasure> EnergyStats::measures{
		TIREX_CPU_ENERGY_SYSTEM_JOULES, TIREX_RAM_ENERGY_SYSTEM_JOULES, TIREX_GPU_ENERGY_SYSTEM_JOULES
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
	return {{TIREX_CPU_ENERGY_SYSTEM_JOULES, "TODO"s},
			{TIREX_RAM_ENERGY_SYSTEM_JOULES, "TODO"s},
			{TIREX_GPU_ENERGY_SYSTEM_JOULES, "TODO"s}};
}