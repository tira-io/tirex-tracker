#ifndef STATS_DEVCONTAINERSTATS_HPP
#define STATS_DEVCONTAINERSTATS_HPP

#include "provider.hpp"

namespace tirex {
	class DevContainerStats final : public StatsProvider {
	private:
	public:
		DevContainerStats();

		std::set<tirexMeasure> providedMeasures() noexcept override;
		void start() override;
		void stop() override;
		Stats getStats() override;

		static constexpr const char* description = "Reads metainformation from the devcontainer.json";
		static const char* version;
		static const std::set<tirexMeasure> measures;
	};
} // namespace tirex

#endif