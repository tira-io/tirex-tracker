#ifndef STATS_ENERGYSTATS_HPP
#define STATS_ENERGYSTATS_HPP

#include "provider.hpp"

#ifndef __APPLE__
#include <cppJoules.hpp>
#else
class EnergyTracker {
public:
	void start() {}
	void stop() {}
};
#endif

namespace tirex {
	class EnergyStats final : public StatsProvider {
	private:
		EnergyTracker tracker;

	public:
		EnergyStats();

		void start() override;
		void stop() override;
		Stats getStats() override;

		static constexpr const char* description = "Collects the energy consumption of various components.";
		static const char* version;
		static const std::set<tirexMeasure> measures;
	};
} // namespace tirex

#endif