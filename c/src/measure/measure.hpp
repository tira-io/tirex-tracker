#ifndef MEASURE_MEASURE_HPP
#define MEASURE_MEASURE_HPP

#include <tirex_tracker.h>

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace tirex {

	template <typename T>
	struct TimeSeries final {
		using clock = std::chrono::high_resolution_clock;

	private:
		struct AggFn {
			virtual T update(const T& next) = 0;
			virtual void reset() = 0;
			virtual std::unique_ptr<AggFn> copy() = 0;
		};
		struct MaxAggFn : public AggFn {
			std::optional<T> val;
			MaxAggFn() { reset(); }
			T update(const T& next) override {
				if (!val.has_value())
					val = next;
				else
					val = std::max(val.value(), next);
				return val.value();
			}
			void reset() override { val = std::nullopt; }
			std::unique_ptr<AggFn> copy() override { return std::make_unique<MaxAggFn>(*this); }
		};
		struct MinAggFn : public AggFn {
			std::optional<T> val;
			MinAggFn() { reset(); }
			T update(const T& next) override {
				if (!val.has_value())
					val = next;
				else
					val = std::min(val.value(), next);
				return val.value();
			}
			void reset() override { val = std::nullopt; }
			std::unique_ptr<AggFn> copy() override { return std::make_unique<MinAggFn>(*this); }
		};
		struct AvgAggFn : public AggFn {
			T sum;
			size_t num = 0;
			AvgAggFn() { reset(); }
			T update(const T& next) override {
				num += 1;
				sum += next;
				return sum / num;
			}
			void reset() override {
				sum = {};
				num = 0;
			}
			std::unique_ptr<AggFn> copy() override { return std::make_unique<AvgAggFn>(*this); }
		};
		/**
		 * @brief True iff timeseries should be stored in TimeSeries::timepoints and TimeSeries::values.
		 */
		const bool storeSeries;
		const std::chrono::milliseconds batchinverall;
		const clock::time_point starttime;
		T max; /**< @brief Minimum value encountered in the time series **/
		T min; /**< @brief Maximum value encountered in the time series **/
		T avg; /**< @brief Average value encountered in the time series **/
		std::vector<std::chrono::milliseconds> timepoints;
		std::vector<T> values;
		const std::unique_ptr<AggFn> aggfn;

		static T maxFn(const T& a, const T& b) { return std::max(a, b); }
		static T minFn(const T& a, const T& b) { return std::min(a, b); }

		static inline std::unique_ptr<AggFn> enumToAggFn(tirexAggregateFn fn) {
			switch (fn) {
			case TIREX_AGG_NO:
				return nullptr;
			case TIREX_AGG_MAX:
				return std::make_unique<MaxAggFn>();
			case TIREX_AGG_MIN:
				return std::make_unique<MinAggFn>();
			case TIREX_AGG_MEAN:
				return std::make_unique<AvgAggFn>();
			default:
				abort(); /** invalid value **/
			};
		}

	public:
		TimeSeries()
				: storeSeries(false), batchinverall(), starttime(clock::now()), max(), min(), avg(), timepoints(),
				  values(), aggfn() {}
		TimeSeries(std::chrono::milliseconds batchintervall, tirexAggregateFn batchagg)
				: storeSeries(true), batchinverall(batchintervall), starttime(clock::now()), max(), min(), avg(),
				  timepoints(), values(), aggfn(enumToAggFn(batchagg)) {}
		TimeSeries(const TimeSeries& other)
				: storeSeries(other.storeSeries), batchinverall(other.batchinverall), starttime(other.starttime),
				  max(other.max), min(other.min), avg(other.avg), timepoints(other.timepoints), values(other.values),
				  aggfn(other.aggfn->copy()) {}
		TimeSeries(TimeSeries&& other) = default;

		TimeSeries& operator=(TimeSeries&& other) = default;

		void addValue(const T& value) noexcept {
			if (storeSeries) {
				auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - starttime);
				if (!timepoints.empty() && (delta - timepoints.back()) < batchinverall) { // Add to previous batch
					values.back() = aggfn->update(value);
				} else { // Start New Batch
					aggfn->reset();
					timepoints.emplace_back(delta);
					values.emplace_back(value);
				}
			}
			max = maxFn(max, value);
			min = minFn(min, value);
			/** \todo update average **/
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