#ifndef STATS_DEVCONTAINERSTATS_HPP
#define STATS_DEVCONTAINERSTATS_HPP

#include "provider.hpp"

#include <filesystem>

namespace tirex {
	class DevContainerStats final : public StatsProvider {
	private:
		std::filesystem::path targetWorkingDir;

	public:
		explicit DevContainerStats(const TrackingTarget& target);

		std::set<tirexMeasure> providedMeasures() noexcept override;
		Stats getInfo() override;

		static constexpr const char* description = "Reads metainformation from the devcontainer.json";
		static const char* version;
		static const std::set<tirexMeasure> measures;
	};
} // namespace tirex

#endif