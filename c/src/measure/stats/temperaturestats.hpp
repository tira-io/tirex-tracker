#ifndef STATS_TEMPERATURESTATS_HPP
#define STATS_TEMPERATURESTATS_HPP

#include "../timeseries.hpp"
#include "provider.hpp"

#include <optional>

namespace tirex {
	using namespace std::chrono_literals;

	/**
	 * @brief Tracks the CPU package temperature over the measured period as a time series.
	 * @details The temperature is read from the operating system's thermal interface. On Linux this is the sysfs
	 * thermal zone whose type identifies the CPU/SoC (e.g., \c x86_pkg_temp on Intel or \c cpu-thermal on the
	 * Raspberry Pi). On systems without a recognized sensor (and on Windows/macOS, where this is not implemented),
	 * the measure is not provided.
	 */
	class TemperatureStats final : public StatsProvider {
	private:
		tirex::TimeSeries<unsigned> temperature =
				ts::store<unsigned>() | ts::Limit(300, TIREX_AGG_MEAN) |
				ts::Batched(100ms, TIREX_AGG_MEAN, 300); /** \todo make agg configurable */

		/**
		 * @brief Reads the current CPU temperature in degree Celsius.
		 * @details Implemented per platform; returns empty if the system exposes no suitable temperature sensor.
		 */
		static std::optional<unsigned> readTemperature();

	public:
		std::set<tirexMeasure> providedMeasures() noexcept override;
		void start() override;
		void stop() override;
		void step() override;
		Stats getStats() override;

		static constexpr const char* description = "Tracks the temperature of various components.";
		static const char* version;
		static const std::set<tirexMeasure> measures;
	};
} // namespace tirex

#endif
