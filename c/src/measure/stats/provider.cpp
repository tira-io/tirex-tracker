#include "provider.hpp"

#include "../../logging.hpp"

#include "energystats.hpp"
#include "gitstats.hpp"
#include "gpustats.hpp"
#include "systemstats.hpp"

#include <algorithm>

using tirex::EnergyStats;
using tirex::GitStats;
using tirex::GPUStats;
using tirex::StatsProvider;
using tirex::SystemStats;

const std::map<std::string, tirex::ProviderEntry> tirex::providers{
		{"system",
		 {std::make_unique<SystemStats>, SystemStats::measures, SystemStats::version, SystemStats::description}},
		{"energy",
		 {std::make_unique<EnergyStats>, EnergyStats::measures, EnergyStats::version, EnergyStats::description}},
		{"git", {std::make_unique<GitStats>, GitStats::measures, GitStats::version, GitStats::description}},
		{"gpu", {std::make_unique<GPUStats>, GPUStats::measures, GPUStats::version, GPUStats::description}}
};

std::set<tirexMeasure>
tirex::initProviders(std::set<tirexMeasure> measures, std::vector<std::unique_ptr<StatsProvider>>& providers) {
	for (auto& [_, info] : tirex::providers) {
		std::set<tirexMeasure> diff;
		std::set_difference(
				measures.begin(), measures.end(), info.measures.begin(), info.measures.end(),
				std::inserter(diff, diff.begin())
		);
		if (diff.size() != measures.size()) { // The provider is responsible for some of the requested measures
			providers.emplace_back(info.constructor());
		}
		measures = std::move(diff);
	}
	return measures;
}