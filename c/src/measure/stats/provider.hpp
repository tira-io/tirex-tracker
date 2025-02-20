#ifndef STATS_PROVIDER_HPP
#define STATS_PROVIDER_HPP

#include <measure.h>

#include "../measure.hpp"

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>

namespace msr {
	using StatVal = std::variant<std::string, msr::TimeSeries<unsigned>>;
	using Stats = std::map<msrMeasure, StatVal>;

	msrResult_st* createMsrResultFromStats(Stats&& stats);

	class StatsProvider {
	public:
		virtual ~StatsProvider() = default;

		/**
		 * @brief Start is called once at the very beginning of collecting statistics and shortly before the command is
		 * run.
		 */
		virtual void start() {}
		/**
		 * @brief Stop is called once at the end of collecting statistics and shortly after the command is run.
		 */
		virtual void stop() {}
		/**
		 * @brief Step may be called multiple times during the execution of the command at some configured intervall (or
		 * even not at all).
		 */
		virtual void step() {}
		/**
		 * @brief Returns the statistics collected during the last measurement. This should be called once after
		 * StatsProvider::stop().
		 * 
		 * @returns the statistics that were measured
		 */
		virtual Stats getStats() { return {}; }

		/**
		 * @brief Returns the information collected by this provider.
		 * 
		 * @return the system information that is collected by this provider. 
		 */
		virtual Stats getInfo() { return {}; }
	};

	using ProviderConstructor = std::function<std::unique_ptr<StatsProvider>(void)>;
	struct ProviderEntry final {
		ProviderConstructor constructor;
		const std::set<msrMeasure>& measures; /**< The set of measures that the provider is responsible for */
		const char* version;
		const char* description;
	};
	extern const std::map<std::string, ProviderEntry> providers;

	std::set<msrMeasure>
	initProviders(std::set<msrMeasure> measures, std::vector<std::unique_ptr<StatsProvider>>& providers);
} // namespace msr

#endif