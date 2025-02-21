#ifndef MEASURE_MEASURE_HPP
#define MEASURE_MEASURE_HPP

#include <chrono>
#include <utility>
#include <vector>

namespace msr {
	template <typename T>
	struct TimeSeries final {
		using clock = std::chrono::high_resolution_clock;

	private:
		const bool storeSeries;
		const clock::time_point starttime;
		T max;
		T min;
		T avg;
		std::vector<std::chrono::milliseconds> timepoints;
		std::vector<T> values;

	public:
		TimeSeries(bool storeSeries)
				: storeSeries(storeSeries), starttime(clock::now()), max(), min(), avg(), timepoints(), values() {}

		void addValue(const T& value) noexcept {
			if (storeSeries) {
				timepoints.emplace_back(std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - starttime)
				);
				values.emplace_back(value);
			}
			max = std::max(max, value);
			min = std::min(min, value);
			/** \todo update average **/
		}
		void reset() {
			timepoints.clear();
			values.clear();
		}

		const T& maxValue() const noexcept { return max; }
		const T& minValue() const noexcept { return min; }
		const T& avgValue() const noexcept { return avg; }
		const std::pair<const std::vector<std::chrono::milliseconds>&, const std::vector<T>&>
		timeseries() const noexcept {
			return {timepoints, values};
		}
	};
}; // namespace msr

#endif