#ifndef MEASURE_MEASURE_HPP
#define MEASURE_MEASURE_HPP

#include <tirex_tracker.h>

#include <chrono>
#include <functional>
#include <utility>
#include <vector>

#include <iostream>

namespace tirex {
	template <typename T>
	struct TimeSeries final {
		using clock = std::chrono::high_resolution_clock;

	private:
		using Aggfn = std::function<T(const T&, const T&)>;
		const bool storeSeries;
		const std::chrono::milliseconds batchinverall;
		const clock::time_point starttime;
		T max;
		T min;
		T avg;
		std::vector<std::chrono::milliseconds> timepoints;
		std::vector<T> values;
		const Aggfn aggfn;

		static T maxFn(const T& a, const T& b) { return std::max(a, b); }
		static T minFn(const T& a, const T& b) { return std::min(a, b); }

	public:
		TimeSeries()
				: storeSeries(false), batchinverall(), starttime(clock::now()), max(), min(), avg(), timepoints(),
				  values(), aggfn() {}
		TimeSeries(std::chrono::milliseconds batchintervall, tirexAggregateFn batchagg)
				: storeSeries(true), batchinverall(batchintervall), starttime(clock::now()), max(), min(), avg(),
				  timepoints(), values(), aggfn(maxFn /** \todo implement */) {}

		void addValue(const T& value) noexcept {
			if (storeSeries) {
				auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - starttime);
				if (!timepoints.empty() && (delta - timepoints.back()) < batchinverall) { // Add to previous batch
					values.back() = aggfn(values.back(), value);
				} else { // Start New Batch
					timepoints.emplace_back(delta);
					values.emplace_back(value);
				}
			}
			max = maxFn(max, value);
			min = minFn(min, value);
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
}; // namespace tirex

#endif