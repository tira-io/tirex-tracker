#ifndef STATS_NVMLStatsSTATS_HPP
#define STATS_NVMLStatsSTATS_HPP

#include "../timeseries.hpp"
#include "provider.hpp"

using nvmlDevice_t = struct nvmlDevice_st*;

namespace tirex {
	class NVMLStats final : public StatsProvider {
	private:
		struct {
			const bool supported;
			std::vector<nvmlDevice_t> devices;
			TimeSeries<unsigned> vramUsageTotal = TimeSeries<unsigned>::create<MaxDataPoints<unsigned>>(
					300, TIREX_AGG_MAX
			); /** \todo make agg configurable */
			TimeSeries<unsigned> vramUsageProcess = TimeSeries<unsigned>::create<MaxDataPoints<unsigned>>(
					300, TIREX_AGG_MAX
			); /** \todo make agg configurable */
			TimeSeries<unsigned> utilizationTotal = TimeSeries<unsigned>::create<MaxDataPoints<unsigned>>(
					300, TIREX_AGG_MAX
			); /** \todo make agg configurable */
		} nvml;

	public:
		NVMLStats();

		std::set<tirexMeasure> providedMeasures() noexcept override;
		void step() override;
		Stats getStats() override;
		Stats getInfo() override;

		static constexpr const char* description = "Collects GPU related metrics for NVIDIA GPUs.";
		static const char* version;
		static const std::set<tirexMeasure> measures;
	};
} // namespace tirex

#endif