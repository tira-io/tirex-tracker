#ifndef STATS_ENERGYSTATS_HPP
#define STATS_ENERGYSTATS_HPP

#include "provider.hpp"

#include <cppjoules/cppjoules.hpp>

namespace tirex {
	class EnergyStats final : public StatsProvider {
	private:
		cppjoules::EnergyTracker tracker;

	public:
		EnergyStats();

		std::set<tirexMeasure> providedMeasures() noexcept override;
		void start() override;
		void stop() override;
		Stats getStats() override;

		static constexpr const char* description = "Collects the energy consumption of various components.";
		static const char* version;
		static const std::set<tirexMeasure> measures;
	};
} // namespace tirex

#endif
