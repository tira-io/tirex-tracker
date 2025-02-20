#ifndef MEASURE_MEASURE_HPP
#define MEASURE_MEASURE_HPP

#include <chrono>
#include <utility>
#include <vector>

namespace msr {
	template <typename T>
	struct TimeSeries final {
	private:
		T max;
		std::vector<std::pair<std::chrono::high_resolution_clock::time_point, T>> values;

	public:
		void addValue(const T& value) noexcept {
			values.emplace_back(std::pair<std::chrono::high_resolution_clock::time_point, T>{std::chrono::high_resolution_clock::now(), value
			});
			max = std::max(max, value);
		}
		void reset() { values.clear(); }

		const T& maxValue() const noexcept { return max; }
	};
}; // namespace msr

#endif