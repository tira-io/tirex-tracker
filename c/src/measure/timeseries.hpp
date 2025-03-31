#ifndef MEASURE_TIMESERIES_HPP
#define MEASURE_TIMESERIES_HPP

#include <tirex_tracker.h>

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace tirex {

	/**
	 * @brief The details namespace contains implementation details and is not part of the public API.
	 */
	namespace details {
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
		[[nodiscard]] static inline std::unique_ptr<AggFn<T>> enumToAggFn(tirexAggregateFn fn) {
			switch (fn) {
			case TIREX_AGG_NO:
				return nullptr;
			case TIREX_AGG_MAX:
				return std::make_unique<details::MaxAggFn<T>>();
			case TIREX_AGG_MIN:
				return std::make_unique<details::MinAggFn<T>>();
			case TIREX_AGG_MEAN:
				return std::make_unique<details::AvgAggFn<T>>();
			default:
				/** \todo handle more gracefully (e.g., return nullptr) **/
				abort(); /** invalid value **/
			};
		}
	} // namespace details

	template <typename T>
	class Strategy {
	public:
		Strategy() = default;
		Strategy(const Strategy& other) = default;
		Strategy(Strategy&& other) = default;
		virtual ~Strategy() = default;
		Strategy& operator=(const Strategy& other) = default;
		Strategy& operator=(Strategy&& other) = default;

		virtual void
		update(std::vector<std::chrono::milliseconds>& timepoints, std::vector<T>& values,
			   const std::chrono::milliseconds& deltatime, const T& value) noexcept = 0;
	};

	template <typename T>
	class NoStore final : public Strategy<T> {
	public:
		void
		update(std::vector<std::chrono::milliseconds>& timepoints, std::vector<T>& values,
			   const std::chrono::milliseconds& deltatime, const T& value) noexcept override {}
	};

	template <typename T>
	class BatchedStore final : public Strategy<T> {
	private:
		std::chrono::milliseconds batchinverall;
		std::unique_ptr<details::AggFn<T>> aggfn;

	public:
		BatchedStore(std::chrono::milliseconds batchinverall, tirexAggregateFn batchagg) noexcept
				: batchinverall(batchinverall), aggfn(details::enumToAggFn<T>(batchagg)) {}

		void
		update(std::vector<std::chrono::milliseconds>& timepoints, std::vector<T>& values,
			   const std::chrono::milliseconds& deltatime, const T& value) noexcept override {
			if (!timepoints.empty() && (deltatime - timepoints.back()) < batchinverall) { // Add to previous batch
				values.back() = aggfn->update(value);
			} else { // Start New Batch
				aggfn->reset();
				timepoints.emplace_back(deltatime);
				values.emplace_back(value);
			}
		}
	};
	template <typename T>
	class MaxDataPoints final : public Strategy<T> {
	private:
		using AggFn = T (*)(const T& a, const T& b);
		size_t maxDataPoints;
		AggFn aggfn;

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

	public:
		MaxDataPoints(size_t maxDataPoints, tirexAggregateFn batchagg) noexcept
				: maxDataPoints(maxDataPoints), aggfn(enumToAggFn(batchagg)) {}

		void
		update(std::vector<std::chrono::milliseconds>& timepoints, std::vector<T>& values,
			   const std::chrono::milliseconds& deltatime, const T& value) noexcept override {
			[[assume(timepoints.size() == values.size())]];
			if (timepoints.size() >= maxDataPoints) { // Aggregate every pair of elements to half the number of points
				for (size_t i = 0; i < timepoints.size(); i += 2) {
					timepoints[i / 2] = timepoints[i + 1];
					values[i / 2] = aggfn(values[i], values[i + 1]);
				}
				/** \fixme does not yet work for uneven maxDataPoints */
				timepoints.resize(timepoints.size() / 2);
				values.resize(timepoints.size());
			}
			timepoints.emplace_back(deltatime);
			values.emplace_back(value);
		}
	};

	/**
	 * @brief The timeseries datatype represents a compressed timestamped sequence of datapoints.
	 * 
	 * @tparam T The datatype of each entry of the timeseries.
	 */
	template <typename T>
	struct TimeSeries final {
		using clock = std::chrono::high_resolution_clock;

	private:
		clock::time_point starttime;
		T max; /**< @brief Minimum value encountered in the time series **/
		T min; /**< @brief Maximum value encountered in the time series **/
		T avg; /**< @brief Average value encountered in the time series **/
		std::vector<std::chrono::milliseconds> timepoints;
		std::vector<T> values;
		std::unique_ptr<Strategy<T>> strategy;

		static T maxFn(const T& a, const T& b) { return std::max(a, b); }
		static T minFn(const T& a, const T& b) { return std::min(a, b); }

	public:
		TimeSeries(std::unique_ptr<Strategy<T>>&& strategy, clock::time_point starttime = clock::now())
				: starttime(starttime), max(), min(), avg(), timepoints(), values(), strategy(std::move(strategy)) {}
		TimeSeries(TimeSeries&& other) = default;

		TimeSeries& operator=(TimeSeries&& other) = default;

		void addValue(const T& value) noexcept {
			auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - starttime);
			strategy->update(timepoints, values, delta, value);
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

		template <typename S, typename... Args>
		static TimeSeries create(Args&&... args) {
			return TimeSeries(std::make_unique<S>(std::forward<Args>(args)...));
		}
	};
}; // namespace tirex

#endif