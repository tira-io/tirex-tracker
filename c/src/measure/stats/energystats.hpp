#ifndef STATS_ENERGYSTATS_HPP
#define STATS_ENERGYSTATS_HPP

#include "provider.hpp"

#ifndef __APPLE__
#include <cppjoules/cppjoules.hpp>
#else
// Remove me once CPPJoules supports apple
#include <chrono>
#include <map>

namespace cppjoules {
	struct TrackerResults {
		std::chrono::milliseconds time;
		std::map<std::string, long long> energy;
	};

	enum Capability { NONE = 0, CPU_PROFILE = 1 << 0, RAM_PROFILE = 1 << 1, GPU_PROFILE = 1 << 2 };

	class EnergyTracker {
	public:
		void start() {}
		void stop() {}
		TrackerResults calculate_energy() const noexcept { return TrackerResults{{}, {}}; }

		Capability getCapabilities() const { return Capability::NONE; }
	};
} // namespace cppjoules
#endif

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
