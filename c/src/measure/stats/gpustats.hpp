#ifndef STATS_GPUSTATS_HPP
#define STATS_GPUSTATS_HPP

#include "../measure.hpp"
#include "provider.hpp"

using nvmlDevice_t = struct nvmlDevice_st*;

namespace msr {
	class GPUStats final : public StatsProvider {
	private:
		struct {
			const bool supported;
			std::vector<nvmlDevice_t> devices;
			TimeSeries<unsigned> vramUsageTotal;
			TimeSeries<unsigned> vramUsageProcess;
			TimeSeries<unsigned> utilizationTotal;
		} nvml;

	public:
		GPUStats();

		void start() override;
		void stop() override;
		void step() override;
		Stats getStats() override;

		static constexpr const char* description = "Collects gpu related metrics.";
		static const char* version;
		static const std::set<msrMeasure> measures;
	};
} // namespace msr

#endif