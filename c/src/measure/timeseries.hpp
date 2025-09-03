#ifndef MEASURE_TIMESERIES_HPP
#define MEASURE_TIMESERIES_HPP

#include <tirex_tracker.h>

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <tuple>
#include <vector>

namespace tirex {

	/**
	 * @brief The details namespace contains implementation details and is not part of the public API.
	 */
	namespace ts::details {
		template <typename T>
		struct AggFn {
			AggFn() = default;
			AggFn(const AggFn&) = delete;
			AggFn(AggFn&&) = default;
			virtual ~AggFn() = default;
			AggFn& operator=(const AggFn&) = delete;
			AggFn& operator=(AggFn&&) = delete;

			virtual T update(const T& next) = 0;
			virtual void reset() = 0;
		};
		template <typename T>
		struct MaxAggFn final : public AggFn<T> {
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
		};
		template <typename T>
		struct MinAggFn final : public AggFn<T> {
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
		};
		template <typename T>
		struct AvgAggFn final : public AggFn<T> {
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
		};

		template <typename T>
		struct TimeSeriesImpl {
		public:
			TimeSeriesImpl() = default;
			TimeSeriesImpl(const TimeSeriesImpl<T>&) = delete;
			TimeSeriesImpl(TimeSeriesImpl<T>&&) = default;
			virtual ~TimeSeriesImpl() = default;
			TimeSeriesImpl<T>& operator=(const TimeSeriesImpl<T>&) = delete;
			TimeSeriesImpl<T>& operator=(TimeSeriesImpl<T>&&) = default;

			virtual void addValue(const T& value, const std::chrono::milliseconds& timestamp) noexcept = 0;
			virtual std::chrono::milliseconds currentTimestamp() const noexcept = 0;
			virtual const T& maxValue() const noexcept = 0;
			virtual const T& minValue() const noexcept = 0;
			virtual const T& avgValue() const noexcept = 0;
			virtual std::tuple<std::vector<std::chrono::milliseconds>&, std::vector<T>&> timeseries() noexcept = 0;
			virtual std::tuple<const std::vector<std::chrono::milliseconds>&, const std::vector<T>&>
			timeseries() const noexcept = 0;

			virtual size_t size() const noexcept = 0;
		};
	} // namespace ts::details

	template <typename T>
	struct TimeSeries final {
	private:
		/** \todo propagate const **/
		std::unique_ptr<ts::details::TimeSeriesImpl<T>> impl;

	public:
		TimeSeries(const TimeSeries<T>&) = delete;
		TimeSeries(TimeSeries<T>&&) = default;
		explicit TimeSeries(std::unique_ptr<ts::details::TimeSeriesImpl<T>>&& impl) : impl(std::move(impl)) {}
		TimeSeries<T>& operator=(const TimeSeries<T>&) = delete;

		void addValue(const T& value) noexcept { return addValue(value, currentTimestamp()); }
		void addValue(const T& value, const std::chrono::milliseconds& timestamp) noexcept {
			impl->addValue(value, timestamp);
		}
		std::chrono::milliseconds currentTimestamp() const noexcept { return impl->currentTimestamp(); }

		/**
		 * @brief The largest value encountered in the time series.
		 * @details It is undefined behavior to call this on an empty time series.
		 * 
		 * @return The largest value encountered in the time series.
		 */
		const T& maxValue() const noexcept { return impl->maxValue(); }
		/**
		 * @brief The smallest value encountered in the time series.
		 * @details It is undefined behavior to call this on an empty time series.
		 * 
		 * @return The smallest value encountered in the time series.
		 */
		const T& minValue() const noexcept { return impl->minValue(); }
		/**
		 * @brief The (potentially approximated) average over all values encountered in the time series.
		 * @details It is undefined behavior to call this on an empty time series.
		 * 
		 * @return The (potentially approximated) average over all values encountered in the time series.
		 */
		const T& avgValue() const noexcept { return impl->avgValue(); }
		/**
		 * @brief A pair of timepoint and data vectors. Where the i-th data entry denotes the value of the time series
		 * at the i-th timepoint.
		 * 
		 * @return A pair of timepoint and data vectors.
		 */
		std::tuple<std::vector<std::chrono::milliseconds>&, std::vector<T>&> timeseries() noexcept {
			return impl->timeseries();
		}
		/**
		 * @brief A pair of timepoint and data vectors. Where the i-th data entry denotes the value of the time series
		 * at the i-th timepoint.
		 * 
		 * @return A pair of timepoint and data vectors.
		 */
		std::tuple<const std::vector<std::chrono::milliseconds>&, const std::vector<T>&> timeseries() const noexcept {
			return impl->timeseries();
		}
		size_t size() const noexcept { return impl->size(); }
	};

	namespace ts::details {
		/**
	 * @brief The timeseries datatype represents a compressed timestamped sequence of datapoints.
	 * 
	 * @tparam T The datatype of each entry of the timeseries.
	 */
		template <typename T>
		struct StoreImpl final : public TimeSeriesImpl<T> {
			using clock = std::chrono::high_resolution_clock;

		private:
			clock::time_point starttime;
			T max; /**< @brief Minimum value encountered in the time series **/
			T min; /**< @brief Maximum value encountered in the time series **/
			T avg; /**< @brief Average value encountered in the time series **/
			std::vector<std::chrono::milliseconds> timepoints;
			std::vector<T> values;

			static T maxFn(const T& a, const T& b) { return std::max(a, b); }
			static T minFn(const T& a, const T& b) { return std::min(a, b); }

		public:
			StoreImpl() : StoreImpl(clock::now()) {}
			explicit StoreImpl(clock::time_point starttime)
					: starttime(starttime), max(), min(), avg(), timepoints(), values() {}
			StoreImpl(StoreImpl&& other) = default;

			StoreImpl& operator=(StoreImpl&& other) = default;
			std::chrono::milliseconds currentTimestamp() const noexcept override {
				return std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - starttime);
				;
			}

			void addValue(const T& value, const std::chrono::milliseconds& timestamp) noexcept override {
				timepoints.emplace_back(timestamp);
				values.emplace_back(value);
				max = maxFn(max, value);
				min = minFn(min, value);
				/** \todo update average **/
			}

			const T& maxValue() const noexcept override { return max; }
			const T& minValue() const noexcept override { return min; }
			const T& avgValue() const noexcept override { return avg; }
			std::tuple<std::vector<std::chrono::milliseconds>&, std::vector<T>&> timeseries() noexcept override {
				return {timepoints, values};
			}
			std::tuple<const std::vector<std::chrono::milliseconds>&, const std::vector<T>&>
			timeseries() const noexcept override {
				return {timepoints, values};
			}

			size_t size() const noexcept override { return timepoints.size(); }
		};

		template <typename T>
		struct BatchedImpl final : public TimeSeriesImpl<T> {
			std::chrono::milliseconds batchintervall;
			std::unique_ptr<AggFn<T>> aggfn;
			size_t startBatchingAtSize;
			TimeSeries<T> ts;

			[[nodiscard]] static inline std::unique_ptr<AggFn<T>> enumToAggFn(tirexAggregateFn fn) {
				switch (fn) {
				case TIREX_AGG_NO:
					return nullptr;
				case TIREX_AGG_MAX:
					return std::make_unique<MaxAggFn<T>>();
				case TIREX_AGG_MIN:
					return std::make_unique<MinAggFn<T>>();
				case TIREX_AGG_MEAN:
					return std::make_unique<AvgAggFn<T>>();
				default:
					/** \todo handle more gracefully (e.g., return nullptr) **/
					abort(); /** invalid value **/
				};
			}

			BatchedImpl(
					std::chrono::milliseconds batchintervall, tirexAggregateFn agg, size_t startBatchingAtSize,
					TimeSeries<T>&& ts
			)
					: batchintervall(batchintervall), aggfn(enumToAggFn(agg)),
					  startBatchingAtSize(std::max(size_t(1), startBatchingAtSize)), ts(std::move(ts)) {}

			void addValue(const T& value, const std::chrono::milliseconds& timestamp) noexcept override {
				const auto& [timepoints, values] = timeseries();
				if (size() >= startBatchingAtSize &&
					(timestamp - timepoints.back()) < batchintervall) { // Add to previous batch
					values.back() = aggfn->update(value);
				} else { // Start New Batch
					aggfn->reset();
					ts.addValue(aggfn->update(value), timestamp);
				}
			}
			std::chrono::milliseconds currentTimestamp() const noexcept override { return ts.currentTimestamp(); }
			const T& maxValue() const noexcept override { return ts.maxValue(); }
			const T& minValue() const noexcept override { return ts.minValue(); }
			const T& avgValue() const noexcept override { return ts.avgValue(); }
			std::tuple<std::vector<std::chrono::milliseconds>&, std::vector<T>&> timeseries() noexcept override {
				return ts.timeseries();
			}
			std::tuple<const std::vector<std::chrono::milliseconds>&, const std::vector<T>&>
			timeseries() const noexcept override {
				return ts.timeseries();
			}

			size_t size() const noexcept override { return ts.size(); }
		};

		template <typename T>
		struct LimitImpl final : public TimeSeriesImpl<T> {
			using AggFn = T (*)(const T& a, const T& b);
			size_t limit;
			AggFn aggfn;
			TimeSeries<T> ts;

			static AggFn enumToAggFn(tirexAggregateFn fn) {
				switch (fn) {
				case TIREX_AGG_NO:
					return nullptr;
				case TIREX_AGG_MAX:
					return +[](const T& a, const T& b) { return std::max(a, b); };
				case TIREX_AGG_MIN:
					return +[](const T& a, const T& b) { return std::min(a, b); };
				case TIREX_AGG_MEAN:
					return +[](const T& a, const T& b) { return (a + b) / 2; };
				default:
					/** \todo handle more gracefully (e.g., return nullptr) **/
					abort(); /** invalid value **/
				};
			}

			LimitImpl(size_t limit, tirexAggregateFn agg, TimeSeries<T>&& ts)
					: limit(limit), aggfn(enumToAggFn(agg)), ts(std::move(ts)) {}

			void addValue(const T& value, const std::chrono::milliseconds& timestamp) noexcept override {
				auto data = timeseries();
				auto& [timepoints, values] = data;
				if (size() >= limit) { // Aggregate every pair of elements to half the number of points
					for (size_t i = 0; i < timepoints.size(); i += 2) {
						timepoints[i / 2] = timepoints[i + 1];
						values[i / 2] = aggfn(values[i], values[i + 1]);
					}
					/** \fixme does not yet work for uneven limit */
					timepoints.resize(timepoints.size() / 2);
					values.resize(timepoints.size());
				}
				ts.addValue(value, timestamp);
			}
			std::chrono::milliseconds currentTimestamp() const noexcept override { return ts.currentTimestamp(); }
			const T& maxValue() const noexcept override { return ts.maxValue(); }
			const T& minValue() const noexcept override { return ts.minValue(); }
			const T& avgValue() const noexcept override { return ts.avgValue(); }
			std::tuple<std::vector<std::chrono::milliseconds>&, std::vector<T>&> timeseries() noexcept override {
				return ts.timeseries();
			}
			std::tuple<const std::vector<std::chrono::milliseconds>&, const std::vector<T>&>
			timeseries() const noexcept override {
				return ts.timeseries();
			}

			size_t size() const noexcept override { return ts.size(); }
		};
	} // namespace ts::details

	namespace ts {

		struct Batched final {
		private:
			std::chrono::milliseconds batchintervall;
			tirexAggregateFn agg;
			size_t startBatchingAtSize;

		public:
			Batched(std::chrono::milliseconds batchintervall, tirexAggregateFn agg, size_t startBatchingAtSize = 0)
					: batchintervall(batchintervall), agg(agg), startBatchingAtSize(startBatchingAtSize) {}
			template <typename T>
			friend TimeSeries<T> operator|(TimeSeries<T>&& ts, const Batched& self) {
				return TimeSeries<T>(std::make_unique<details::BatchedImpl<T>>(
						self.batchintervall, self.agg, self.startBatchingAtSize, std::move(ts)
				));
			}
		};

		struct Limit final {
		private:
			size_t limit;
			tirexAggregateFn agg;

		public:
			Limit(size_t limit, tirexAggregateFn agg) : limit(limit), agg(agg) {}
			template <typename T>
			friend TimeSeries<T> operator|(TimeSeries<T>&& ts, const Limit& self) {
				return TimeSeries<T>(std::make_unique<details::LimitImpl<T>>(self.limit, self.agg, std::move(ts)));
			}
		};

		template <typename T>
		static inline TimeSeries<T> store() {
			return TimeSeries<T>(std::make_unique<details::StoreImpl<T>>());
		}
	} // namespace ts

}; // namespace tirex

#endif