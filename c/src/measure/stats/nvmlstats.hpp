#ifndef STATS_NVMLStatsSTATS_HPP
#define STATS_NVMLStatsSTATS_HPP

#include "../timeseries.hpp"
#include "provider.hpp"

#if defined(_WINDOWS) || defined(_WIN32) || defined(WIN32)
#include <windows.h>
#undef ERROR //  Make problems with logging.h otherwise
#endif

using nvmlDevice_t = struct nvmlDevice_st*;

namespace tirex {
	using namespace std::chrono_literals;

	class NVMLStats final : public StatsProvider {
	private:
		struct {
			const bool supported;
			std::vector<nvmlDevice_t> devices;
			unsigned long long processUtilTimestamp = 0;
			TimeSeries<unsigned> vramUsageTotal =
					ts::store<unsigned>() | ts::Limit(300, TIREX_AGG_MAX) |
					ts::Batched(100ms, TIREX_AGG_MAX, 300); /** \todo make agg configurable */
			TimeSeries<unsigned> vramUsageProcess =
					ts::store<unsigned>() | ts::Limit(300, TIREX_AGG_MAX) |
					ts::Batched(100ms, TIREX_AGG_MAX, 300); /** \todo make agg configurable */
			TimeSeries<unsigned> utilizationTotal =
					ts::store<unsigned>() | ts::Limit(300, TIREX_AGG_MAX) |
					ts::Batched(100ms, TIREX_AGG_MAX, 300); /** \todo make agg configurable */
			TimeSeries<unsigned> utilizationProcess =
					ts::store<unsigned>() | ts::Limit(300, TIREX_AGG_MAX) |
					ts::Batched(100ms, TIREX_AGG_MAX, 300); /** \todo make agg configurable */
		} nvml;

#if defined(_WINDOWS) || defined(_WIN32) || defined(WIN32)
		HANDLE pid; /**< The process identifier of the tracked process. */
#elif __APPLE__ || __linux__
		pid_t pid; /**< The process identifier of the tracked process. */
#else
#error "Unsupported OS"
#endif

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