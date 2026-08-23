#ifndef MEASURE_TIMESERIES_HPP
#define MEASURE_TIMESERIES_HPP

#include "../abort.hpp"

#include <tirex_tracker.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <list>
#include <optional>
#include <queue>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace tirex {

	/**
	 * @brief The details namespace contains implementation details and is not part of the public API.
	 */
	namespace ts::details {
		/**
		 * @brief Tracks the running max/min/mean over every value it is fed, independent of how (or whether) those
		 * values themselves are later compressed/stored. E.g., if the values 2.010 and 2.015 are observed, then this
		 * could be stored as 2.013 but for the max, we still remember 2.015 exactly.
		 * @details Feeds directly into TimeSeries::maxValue()/minValue()/avgValue(); reports a default-constructed T if
		 * never updated (i.e. on an empty time series). Uses Welford's algorithm to compute the running mean.
		 */
		template <typename T>
		struct RunningAggregates {
			T max{};		 /**< @brief Maximum value encountered so far **/
			T min{};		 /**< @brief Minimum value encountered so far **/
			T avg{};		 /**< @brief Average value encountered so far **/
			double mean = 0; /**< @brief Running mean, used to compute the average **/
			size_t numAdded = 0;

			void update(const T& value) noexcept {
				if (numAdded == 0)
					max = min = value;
				else {
					max = std::max(max, value);
					min = std::min(min, value);
				}
				numAdded += 1;
				mean += (static_cast<double>(value) - mean) / static_cast<double>(numAdded);
				avg = static_cast<T>(mean);
			}
		};

		/**
		 * @brief One piece of a compressed time series: either a lossless run of identical raw values, or (once
		 * capacity forces lossy compression) a merged group of adjacent segments. Represents whatever value(s) were
		 * held during the time span `(timestamp - duration, timestamp]`.
		 * @details Carries enough sufficient statistics (duration-weighted sum/sum-of-squares, min/max with their own
		 * timestamps) to be merged again with a neighbor without ever revisiting the original raw samples.
		 * `sum`/`sumSq`/`duration` are wide (double) accumulators rather than T: T is unsigned at every real call
		 * site, and value*duration_ms (let alone its square) overflows 32 bits well within a long-running experiment.
		 */
		template <typename T>
		struct Segment {
			std::chrono::milliseconds timestamp{}; /**< @brief End of the time span this segment represents. */
			double sum = 0;		 /**< @brief Sum of value*duration over every raw sample folded into this segment. */
			double sumSq = 0;	 /**< @brief Sum of value^2*duration; together with `sum` gives the segment's SSE. */
			double duration = 0; /**< @brief Total duration (ms) folded into this segment so far. */
			T min{};
			T max{};
			std::chrono::milliseconds minAt{};
			std::chrono::milliseconds maxAt{};
			size_t count = 1;
			size_t id = 0; /**< @brief Permanent identity, used to detect a since-merged-away neighbor safely. */
			/** @brief Bumped on every mutation, to detect an outdated (but still-adjacent) cost. */
			size_t version = 0;

			/** @brief True if every raw sample folded into this segment so far had exactly the same value. */
			[[nodiscard]] bool isConstant() const noexcept { return min == max; }

			[[nodiscard]] T value(tirexAggregateFn agg) const noexcept {
				switch (agg) {
				case TIREX_AGG_MAX:
					return max;
				case TIREX_AGG_MIN:
					return min;
				case TIREX_AGG_MEAN:
					return duration > 0 ? static_cast<T>(sum / duration) : min;
				case TIREX_AGG_NO:
				default:
					assert((count == 1) && (min == max)); // No aggregation means we only have single segments.
					return min;
				}
			}

			/**
			 * @brief Creates a new timeseries segment that consists of a single measurement.
			 * @details Since a single value does not have a duration, the duration until this segment can be provided
			 * through \p gap . It is assumed that the gap ms until this measurement all had value \p value .
			 *
			 * @param value The value to be stored in the timeseries segment.
			 * @param timestamp The timestamp this value occurred at.
			 * @param id A unique identifier for this segment. Is used to identify whether a segment was since merged.
			 *           Default is 0.
			 * @param gap The time since the previous measurement. Used to track granularity of the measurement.
			 *            Default is 0.
			 * @return A new timeseries segment that consists of the single provided measurement.
			 */
			[[nodiscard]] static Segment<T>
			single(const T& value, std::chrono::milliseconds timestamp, size_t id = 0, double gap = 0) noexcept {
				return Segment{
						.timestamp = timestamp,
						.sum = static_cast<double>(value) * gap,
						.sumSq = static_cast<double>(value) * static_cast<double>(value) * gap,
						.duration = gap,
						.min = value,
						.max = value,
						.minAt = timestamp,
						.maxAt = timestamp,
						.count = 1,
						.id = id,
						.version = 0,
				};
			}

			/** @brief Combines two adjacent segments' sufficient statistics; `a` must chronologically precede `b`. */
			[[nodiscard]] static Segment<T> merge(const Segment<T>& a, const Segment<T>& b, size_t id) noexcept {
				Segment<T> out{
						.timestamp = b.timestamp,
						.sum = a.sum + b.sum,
						.sumSq = a.sumSq + b.sumSq,
						.duration = a.duration + b.duration,
						.count = a.count + b.count,
						.id = id,
				};
				if (a.min <= b.min) {
					out.min = a.min;
					out.minAt = a.minAt;
				} else {
					out.min = b.min;
					out.minAt = b.minAt;
				}
				if (a.max >= b.max) {
					out.max = a.max;
					out.maxAt = a.maxAt;
				} else {
					out.max = b.max;
					out.maxAt = b.maxAt;
				}
				return out;
			}
		};

		/**
		 * @brief Cost of merging two adjacent segments: the additional sum-of-squared-error the merge would introduce.
		 * Greedily merging the lowest-cost adjacent pair on overflow is minimum-information-loss merging, which keeps
		 * high resolution in noisy/spiky regions and low resolution in flat ones, with no dependence on recency. This
		 * is equivalent to V-optimal histogram bucket merging and to Ward's minimum-variance linkage.
		 * @see Keogh, Chu, Hart, Pazzani, "An Online Algorithm for Segmenting Time Series", ICDM 2001.
		 * @see Jagadish, Koudas, Muthukrishnan, Poosala, Sevcik, Suel, "Optimal Histograms with Quality Guarantees",
		 *      VLDB 1998.
		 * @see Ioannidis, Poosala, "Balancing Histogram Optimality and Practicality for Query Result Size Estimation",
		 *      SIGMOD 1995.
		 * @see Ward, "Hierarchical Grouping to Optimize an Objective Function", JASA 1963.
		 */
		template <typename T>
		[[nodiscard]] double mergeCost(const Segment<T>& a, const Segment<T>& b) noexcept {
			/* The sum-of-squared-error a segment's own duration-weighted mean leaves unexplained. Zero for a segment
			 * that has not yet accumulated any duration (a single, still-open raw sample: nothing to approximate yet,
			 * so no error). */
			const auto sse = [](const Segment<T>& s) {
				return s.duration <= 0 ? 0.0 : s.sumSq - (s.sum * s.sum) / s.duration;
			};
			return sse(Segment<T>::merge(a, b, -1)) - sse(a) - sse(b); // id is irrelevant; this segment is never stored
		}
	} // namespace ts::details

	/**
	 * @brief A bounded-memory, timestamped sequence of datapoints.
	 * @details Below \p maxSegments distinct values, every sample are kept exactly (and identical consecutive values
	 * are merged losslessly for free). Once capacity is exceeded, the adjacent pair of segments whose merger would
	 * introduce the least approximation error is merged (see ts::details::mergeCost). This adapts resolution to the
	 * data (flat stretches compress hard, noisy/spiky stretches stay fine-grained) evenly across the entire run; spikes
	 * near the start have the same visibility as those near the end.
	 *
	 * @tparam T The datatype of each entry of the timeseries.
	 */
	template <typename T>
	class TimeSeries final {
	private:
		using clock = std::chrono::high_resolution_clock;
		using Segment = ts::details::Segment<T>;
		using Iter = typename std::list<Segment>::iterator;

		struct PairCost {
			double cost;
			decltype(Segment::id) leftId, rightId;
			decltype(Segment::version) leftVersion, rightVersion;
			bool operator>(const PairCost& o) const noexcept { return cost > o.cost; }
		};

		clock::time_point starttime;
		size_t maxSegments;
		tirexAggregateFn agg;
		std::list<Segment> segments;
		std::unordered_map<size_t, Iter> liveSegments; // id -> iterator, only for currently-live segments
		size_t nextId = 0; /**< @brief A counter which unique ID should be assigned to the next created segment. */
		std::priority_queue<PairCost, std::vector<PairCost>, std::greater<>> pairCosts;
		ts::details::RunningAggregates<T> running;
		std::optional<T> lastRawValue;
		std::optional<std::chrono::milliseconds> lastRawTimestamp;

		static tirexAggregateFn validate(tirexAggregateFn agg) {
			switch (agg) {
			case TIREX_AGG_NO:
			case TIREX_AGG_MAX:
			case TIREX_AGG_MIN:
			case TIREX_AGG_MEAN:
				return agg;
			default:
				tirex::abort(tirexLogLevel::CRITICAL, "TimeSeries was given an invalid tirexAggregateFn value");
				return TIREX_AGG_NO;
			}
		}

		void pushPairCost(Iter right) {
			if (right == segments.begin())
				return;
			Iter left = std::prev(right);
			// Not pairCosts.emplace(...): PairCost is a plain aggregate, and parenthesized aggregate init (P0960)
			// isn't supported by libc++'s construct_at on all supported compilers (e.g. Apple Clang 15).
			pairCosts.push(
					PairCost{ts::details::mergeCost(*left, *right), left->id, right->id, left->version, right->version}
			);
		}

		/**
		 * @brief Rebuilds pairCosts from the currently-live segments, discarding every stale entry at once.
		 * @details pushPairCost() runs on every addValue() call, including when a run only *extends* the last segment
		 * (which does not change segments.size()); but entries are only ever popped inside rebalanceIfNeeded()'s loop,
		 * which only runs when the capacity is exceeded. Without this, a constant timeseries keeps segments.size()
		 * comfortably under maxSegments while still pushing one stale entry per call, forever, since nothing ever
		 * triggers a pop: unbounded growth of pairCosts despite the class being intended to be bounded-memory.
		 */
		void compactPairCostsIfNeeded() {
			const size_t numAdjPairs = segments.empty() ? 0 : (segments.size() - 1);
			/* Rebuilding costs O(numAdjPairs) (one pushPairCost() per live pair), so triggering it only once pairCosts
			 * has grown to a multiple of numAdjPairs keeps the *amortized* cost O(1) per addValue(): between two
			 * rebuilds, ~3*numAdjPairs pushes happen first, so the rebuild's cost is spread thin. The flat "+16" floor
			 * exists for when numAdjPairs itself is tiny (0-3 segments): without it, e.g. numAdjPairs=0 would give a
			 * threshold of 0 and force a (pointless) rebuild on every single call. */
			if (pairCosts.size() <= 4 * numAdjPairs + 16)
				return;
			pairCosts = decltype(pairCosts){};
			for (auto it = std::next(segments.begin()); it != segments.end(); ++it)
				pushPairCost(it);
		}

		/**
		 * @brief Lazy-deletion min-heap pop: an entry is only acted on if both segments are still alive, still
		 * adjacent, and unchanged since the entry was pushed (checked via Segment::id, safe even if the node was erased
		 * and its iterator invalidated, and Segment::version, which catches a still-adjacent pair whose cost is merely
		 * outdated). Otherwise it's discarded and the next-best entry is tried.
		 */
		void rebalanceIfNeeded() {
			while (segments.size() > maxSegments && !pairCosts.empty()) {
				auto top = pairCosts.top();
				pairCosts.pop();
				auto leftLookup = liveSegments.find(top.leftId);
				if (leftLookup == liveSegments.end())
					continue;
				Iter left = leftLookup->second;
				Iter right = std::next(left);
				if (right == segments.end() || right->id != top.rightId || left->version != top.leftVersion ||
					right->version != top.rightVersion)
					continue;

				Segment merged = Segment::merge(*left, *right, nextId++);
				liveSegments.erase(left->id);
				liveSegments.erase(right->id);
				Iter mergedIt = segments.insert(left, std::move(merged));
				segments.erase(left);
				segments.erase(right);
				liveSegments[mergedIt->id] = mergedIt;

				if (mergedIt != segments.begin())
					pushPairCost(mergedIt);
				auto after = std::next(mergedIt);
				if (after != segments.end())
					pushPairCost(after);
			}
		}

	public:
		explicit TimeSeries(size_t maxSegments, tirexAggregateFn agg)
				: starttime(clock::now()), maxSegments(std::max(size_t(1), maxSegments)), agg(validate(agg)) {}
		TimeSeries(const TimeSeries&) = delete;
		TimeSeries(TimeSeries&&) = default;
		TimeSeries& operator=(const TimeSeries&) = delete;
		TimeSeries& operator=(TimeSeries&&) = default;

		void addValue(const T& value) noexcept { addValue(value, currentTimestamp()); }
		void addValue(const T& value, const std::chrono::milliseconds& timestamp) noexcept {
			running.update(value);

			if (agg == TIREX_AGG_NO) { // No aggregation requested: keep every sample, unbounded, never merge.
				segments.push_back(Segment::single(value, timestamp));
				return;
			}

			/* Credit each sample with the gap since the *previous* one (backward attribution; also matches how this
			 * codebase's own CPU%-style measures are themselves "utilization since the last poll") instead of
			 * retroactively crediting the previous sample once this one arrives. A weight-0 segment contributes nothing
			 * to a weighted-sum merge, so that merge would look artificially free (cost 0) no matter how different the
			 * values are, silently defeating the greedy minimum-information-loss selection. That's not just a
			 * first-sample problem (nothing precedes it) — any sample tying or going backwards in time relative to its
			 * predecessor hits it too, so the gap is floored at a nominal 1 instead of 0. */
			double gap = lastRawTimestamp.has_value()
								 ? std::max(1.0, static_cast<double>((timestamp - *lastRawTimestamp).count()))
								 : 1.0;
			auto valueAsDouble = static_cast<double>(value);

			bool extendsRun = lastRawValue == value && !segments.empty() && segments.back().isConstant();
			if (extendsRun) {
				auto& back = segments.back();
				back.timestamp = timestamp;
				back.sum += valueAsDouble * gap;
				back.sumSq += valueAsDouble * valueAsDouble * gap;
				back.duration += gap;
				back.count += 1;
				++back.version;
				pushPairCost(std::prev(segments.end()));
			} else {
				auto& fresh = segments.emplace_back(Segment::single(value, timestamp, nextId++, gap));
				auto it = std::prev(segments.end());
				liveSegments[fresh.id] = it;
				pushPairCost(it);
			}

			lastRawValue = value;
			lastRawTimestamp = timestamp;
			rebalanceIfNeeded();
			compactPairCostsIfNeeded();
		}

		[[nodiscard]] std::chrono::milliseconds currentTimestamp() const noexcept {
			return std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - starttime);
		}

		/**
		 * @brief The largest value encountered in the time series.
		 * @details It is undefined behavior to call this on an empty time series.
		 * 
		 * @return The largest value encountered in the time series.
		 */
		[[nodiscard]] const T& maxValue() const noexcept { return running.max; }
		/**
		 * @brief The smallest value encountered in the time series.
		 * @details It is undefined behavior to call this on an empty time series.
		 * 
		 * @return The smallest value encountered in the time series.
		 */
		[[nodiscard]] const T& minValue() const noexcept { return running.min; }
		/**
		 * @brief The (potentially approximated) average over all values encountered in the time series.
		 * @details It is undefined behavior to call this on an empty time series.
		 * 
		 * @return The (potentially approximated) average over all values encountered in the time series.
		 */
		[[nodiscard]] const T& avgValue() const noexcept { return running.avg; }

		/**
		 * @brief A pair of timepoint and data vectors. Where the i-th data entry denotes the value of the time series
		 * at the i-th timepoint.
		 */
		[[nodiscard]] std::tuple<std::vector<std::chrono::milliseconds>, std::vector<T>> timeseries() const noexcept {
			std::vector<std::chrono::milliseconds> timepoints;
			std::vector<T> values;
			timepoints.reserve(segments.size());
			values.reserve(segments.size());
			for (const auto& s : segments) {
				timepoints.emplace_back(s.timestamp);
				values.emplace_back(s.value(agg));
			}
			return {std::move(timepoints), std::move(values)};
		}
		[[nodiscard]] size_t size() const noexcept { return segments.size(); }
	};
} // namespace tirex

#endif
