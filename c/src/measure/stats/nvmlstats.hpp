#ifndef STATS_NVMLStatsSTATS_HPP
#define STATS_NVMLStatsSTATS_HPP

#include "../timeseries.hpp"
#include "provider.hpp"

using nvmlDevice_t = struct nvmlDevice_st*;

namespace tirex {
	using namespace std::chrono_literals;

	class NVMLStats final : public StatsProvider {
	private:
		struct {
			const bool supported;
			std::vector<nvmlDevice_t> devices;
			TimeSeries<unsigned> vramUsageTotal =
					ts::store<unsigned>() | ts::Limit(300, TIREX_AGG_MAX) |
					ts::Batched(100ms, TIREX_AGG_MAX, 300); /** \todo make agg configurable */
			TimeSeries<unsigned> vramUsageProcess =
					ts::store<unsigned>() | ts::Limit(300, TIREX_AGG_MAX) |
					ts::Batched(100ms, TIREX_AGG_MAX, 300); /** \todo make agg configurable */
			TimeSeries<unsigned> utilizationTotal =
					ts::store<unsigned>() | ts::Limit(300, TIREX_AGG_MAX) |
					ts::Batched(100ms, TIREX_AGG_MAX, 300); /** \todo make agg configurable */
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