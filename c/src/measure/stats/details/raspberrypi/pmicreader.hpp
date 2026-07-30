#ifndef STATS_DETAILS_RASPBERRYPI_PMICREADER_HPP
#define STATS_DETAILS_RASPBERRYPI_PMICREADER_HPP

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

namespace tirex {
	/**
	 * @brief Reads the Raspberry Pi 5 PMIC rails and integrates power into energy.
	 * @details The Raspberry Pi exposes per-rail current and voltage through its PMIC. This reader
	 * obtains them by sending the firmware "gencmd" `pmic_read_adc` directly through the VideoCore
	 * mailbox property interface on `/dev/vcio` (the same mechanism `vcgencmd` uses, so no external
	 * binary is required). It computes the instantaneous power per rail (P = U * I) and integrates
	 * it over time (trapezoidal rule) into energy in joules. EnergyStats uses it on platforms where
	 * no RAPL energy is available (i.e., ARM / Raspberry Pi). On any non-Raspberry-Pi platform
	 * PmicReader::available() returns false and all methods are no-ops.
	 *
	 * The rails are mapped onto the same categories that RAPL reports:
	 *   - VDD_CORE              -> CPU energy
	 *   - DDR_VDD2 + DDR_VDDQ   -> RAM energy
	 */
	class PmicReader final {
	public:
		/**
		 * @brief Whether the PMIC can be read on this system.
		 * @details Probes `vcgencmd pmic_read_adc` once and caches the result. Returns true only if the
		 * expected rails could be read (i.e., we are running on a Raspberry Pi with PMIC access).
		 */
		static bool available();

		/** @brief Resets the accumulators and takes the first (baseline) sample. */
		void start();
		/** @brief Takes a sample and accumulates the energy since the previous sample. */
		void step();
		/** @brief Takes the final sample and accumulates the last segment. */
		void stop();

		/** @brief Accumulated energy of the VDD_CORE rail (mapped to "cpu") in joules. */
		double coreJoules() const noexcept { return coreJ; }
		/** @brief Accumulated energy of the DDR rails (mapped to "ram") in joules. */
		double ramJoules() const noexcept { return ramJ; }

	private:
		struct Sample {
			double coreW; /**< Instantaneous power of the CPU rail in watt. */
			double ramW;  /**< Instantaneous power of the RAM rails in watt. */
		};

		/** @brief Reads the PMIC once via the VideoCore mailbox. Empty if the rails can't be read. */
		static std::optional<Sample> readOnce();

		/**
		 * @brief Parses the text output of `pmic_read_adc` into a power sample.
		 * @details Pure (no I/O); separated from readOnce() for testability. Returns empty if no
		 * rails could be parsed.
		 */
		static std::optional<Sample> parseAdc(std::string_view output);

		bool hasPrev = false;
		Sample prev{};
		std::chrono::steady_clock::time_point prevTime{};
		double coreJ = 0.0;
		double ramJ = 0.0;
	};
} // namespace tirex

#endif
