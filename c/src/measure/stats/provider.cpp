#include "provider.hpp"

#include "../../logging.hpp"

#include "energystats.hpp"
#include "gitstats.hpp"
#include "nvmlstats.hpp"
#include "systemstats.hpp"

#include <algorithm>

using tirex::EnergyStats;
using tirex::GitStats;
using tirex::NVMLStats;
using tirex::StatsProvider;
using tirex::SystemStats;

const std::map<std::string, tirex::ProviderEntry> tirex::providers{
		{"system",
		 {std::make_unique<SystemStats>, SystemStats::measures, SystemStats::version, SystemStats::description}},
		{"energy",
		 {std::make_unique<EnergyStats>, EnergyStats::measures, EnergyStats::version, EnergyStats::description}},
		{"git", {std::make_unique<GitStats>, GitStats::measures, GitStats::version, GitStats::description}},
		{"gpu", {std::make_unique<NVMLStats>, NVMLStats::measures, NVMLStats::version, NVMLStats::description}}
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
			auto& provider = providers.emplace_back(info.constructor());
			provider->requestMeasures(measures);
		}
		measures = std::move(diff);
	}
	return measures;
}

void StatsProvider::requestMeasures(const std::set<tirexMeasure>& measures) noexcept {
	auto supported = providedMeasures();
	enabled.clear();
	std::set_intersection(
			measures.cbegin(), measures.cend(), supported.begin(), supported.end(),
			std::inserter(enabled, enabled.begin())
	);
}