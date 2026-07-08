#ifndef STATS_IOREPORTSTATS_HPP
#define STATS_IOREPORTSTATS_HPP

#include <memory>

#include "provider.hpp"

namespace tirex {
	/**
	 * @brief Collects energy statistics on macOS via IOReport (Apple Silicon).
	 * @details Uses libIOReport.dylib to subscribe to the "Energy Model" channels and computes the energy consumed (in
	 * Joules) for CPU, GPU, and DRAM over the tracked interval. Only available on macOS; on other platforms
	 * IOReportStats::providedMeasures() returns an empty set.
	 */
	class IOReportStats final : public StatsProvider {
	private:
#ifdef __APPLE__
		// PImpl Idiom for system-specific includes.
		struct State;
		std::unique_ptr<State> state;
#endif

	public:
		IOReportStats();
		~IOReportStats();

		std::set<tirexMeasure> providedMeasures() noexcept override;
		void start() override;
		void stop() override;
		Stats getStats() override;

		static constexpr const char* description =
				"Collects energy consumption metrics via IOReport (macOS / Apple Silicon only).";
		static const char* version;
		static const std::set<tirexMeasure> measures;
	};
} // namespace tirex

#endif
