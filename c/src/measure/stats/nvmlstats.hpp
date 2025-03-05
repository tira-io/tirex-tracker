#ifndef STATS_NVMLStatsSTATS_HPP
#define STATS_NVMLStatsSTATS_HPP

#include "../measure.hpp"
#include "provider.hpp"

using nvmlDevice_t = struct nvmlDevice_st*;

namespace tirex {
	class NVMLStats final : public StatsProvider {
	private:
		struct {
			const bool supported;
			std::vector<nvmlDevice_t> devices;
			TimeSeries<unsigned> vramUsageTotal{true};
			TimeSeries<unsigned> vramUsageProcess{true};
			TimeSeries<unsigned> utilizationTotal{true};
		} nvml;

	public:
		NVMLStats();

		void step() override;
		Stats getStats() override;
		Stats getInfo() override;

		static constexpr const char* description = "Collects GPU related metrics for NVIDIA GPUs.";
		static const char* version;
		static const std::set<tirexMeasure> measures;
	};
} // namespace tirex

#endif