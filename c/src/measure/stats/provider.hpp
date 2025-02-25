#ifndef STATS_PROVIDER_HPP
#define STATS_PROVIDER_HPP

#include <tirex_tracker.h>

#include "../measure.hpp"

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <variant>

namespace tirex {
	using StatVal = std::variant<std::string, tirex::TimeSeries<unsigned>>;
	using Stats = std::map<tirexMeasure, StatVal>;

	tirexResult_st* createMsrResultFromStats(Stats&& stats);

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
		const std::set<tirexMeasure>& measures; /**< The set of measures that the provider is responsible for */
		const char* version;
		const char* description;
	};
	extern const std::map<std::string, ProviderEntry> providers;

	std::set<tirexMeasure>
	initProviders(std::set<tirexMeasure> measures, std::vector<std::unique_ptr<StatsProvider>>& providers);
} // namespace tirex

#endif