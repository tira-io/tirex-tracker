#include <measure/timeseries.hpp>

#include <tirex_tracker.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

using Catch::Approx;
using Catch::Matchers::RangeEquals;

using tirex::TimeSeries;
using namespace std::chrono_literals;

TEST_CASE("Timeseries", "[MinMaxAvg]") {
	{
		TimeSeries<unsigned> timeseries{300, TIREX_AGG_MEAN};
		timeseries.addValue(20, 0ms);
		CHECK(timeseries.minValue() == 20);
		CHECK(timeseries.maxValue() == 20);
		CHECK(timeseries.avgValue() == 20);

		timeseries.addValue(60, 1ms);
		CHECK(timeseries.minValue() == 20);
		CHECK(timeseries.maxValue() == 60);
		CHECK(timeseries.avgValue() == 40);

		timeseries.addValue(10, 2ms);
		CHECK(timeseries.minValue() == 10);
		CHECK(timeseries.maxValue() == 60);
		CHECK(timeseries.avgValue() == 30);
	}
}

TEST_CASE("Timeseries", "[RunLengthEncoding]") {
	// Consecutive, exactly-equal raw values must collapse losslessly into a single segment, regardless of how
	// long the run lasts, and a differing value must open a new one.
	TimeSeries<unsigned> timeseries{100, TIREX_AGG_MEAN};
	timeseries.addValue(5, 0ms);
	CHECK(timeseries.size() == 1);
	timeseries.addValue(5, 10ms);
	CHECK(timeseries.size() == 1);
	timeseries.addValue(5, 20ms);
	CHECK(timeseries.size() == 1);

	timeseries.addValue(7, 30ms);
	CHECK(timeseries.size() == 2);
	auto [timepoints, values] = timeseries.timeseries();
	CHECK_THAT(values, RangeEquals(std::vector<unsigned>{5, 7}));
	(void)timepoints;
}

TEST_CASE("Timeseries", "[AdaptiveResolution]") {
	// A long flat stretch (0 held for 2000ms) followed by two close, noisy readings (100, then 105 1ms later).
	// With capacity for only 2 segments, the greedy minimum-information-loss merge must merge the two similar
	// noisy readings together (near-zero error introduced) rather than blending either of them into the
	// unrelated flat stretch (which would introduce a huge error) — the whole point of this algorithm over a
	// recency- or position-biased scheme.
	TimeSeries<unsigned> timeseries{2, TIREX_AGG_MEAN};
	timeseries.addValue(0, 0ms);
	timeseries.addValue(0, 1000ms);
	timeseries.addValue(0, 2000ms);
	timeseries.addValue(100, 2001ms);
	timeseries.addValue(105, 2002ms); // pushes size to 3, forcing exactly one merge

	REQUIRE(timeseries.size() == 2);
	auto [timepoints, values] = timeseries.timeseries();
	CHECK_THAT(values, RangeEquals(std::vector<unsigned>{0, 102})); // (100+105)/2, unsigned-truncated
	CHECK_THAT(timepoints, RangeEquals(std::vector{2000ms, 2002ms}));
}

TEST_CASE("Timeseries", "[TimestampReflectsRealDuration]") {
	// Regression: the exported timestamp must always be a segment's `end`, even in MAX/MIN mode -- never
	// maxAt/minAt. `end` is kept accurate on every extension, so consecutive points' timestamps always add up
	// to real elapsed time. maxAt/minAt freeze at a constant run's *first* occurrence and never move again, so
	// using them here silently erases however long that run actually lasted from the visible timeline. This was
	// found via a genuinely confusing 01_tracking chart: a multi-second flat RAM baseline (held constant while
	// an unrelated, long CPU-bound computation ran) collapsed to near-zero width, and its true ~3 real seconds
	// got visually reassigned to whatever value happened to follow it, making that next value look 3x longer
	// than it really lasted.
	TimeSeries<unsigned> timeseries{2, TIREX_AGG_MAX};
	timeseries.addValue(0, 0ms);
	timeseries.addValue(0, 1000ms);
	timeseries.addValue(0, 3000ms);	  // 0 truly holds for 3 real seconds
	timeseries.addValue(100, 3005ms); // the spike
	timeseries.addValue(10, 3006ms);  // pushes size to 3, forcing the spike's segment to merge with its neighbor

	REQUIRE(timeseries.size() == 2);
	auto [timepoints, values] = timeseries.timeseries();
	CHECK_THAT(values, RangeEquals(std::vector<unsigned>{0, 100}));
	// 3000ms (the flat run's real end), not 0ms (where the value first appeared) -- otherwise its entire
	// 3-second real span would vanish from the exported timeline.
	CHECK_THAT(timepoints, RangeEquals(std::vector{3000ms, 3006ms}));
}

TEST_CASE("Timeseries", "[NoAggregation]") {
	// Bugs.md #14: TIREX_AGG_NO must never crash, and must behave as fully unbounded (every sample kept, no
	// merging at all) regardless of how small maxSegments is.
	TimeSeries<unsigned> timeseries{2, TIREX_AGG_NO};
	for (unsigned i = 0; i < 10; ++i)
		timeseries.addValue(i, std::chrono::milliseconds(i));

	CHECK(timeseries.size() == 10);
	auto [timepoints, values] = timeseries.timeseries();
	CHECK_THAT(values, RangeEquals(std::vector<unsigned>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
	(void)timepoints;
}

TEST_CASE("Timeseries", "[TrendPreservation]") {
	// A larger, more realistic stress test: three consecutive trends (values hovering around 3, then 8, then 1,
	// each perturbed with a small repeating wobble so consecutive samples are never exactly equal and RLE alone
	// can't collapse a whole trend into one segment) forced through a capacity far below the raw sample count.
	// The greedy minimum-information-loss merge should preserve the overall *shape* of the run -- each trend's
	// window should still average close to the value it actually held -- rather than blurring everything into
	// noise or letting merges cross trend boundaries indiscriminately.
	//
	// The gap between trends (3 -> 8 -> 1) is deliberately much larger than the wobble amplitude (1), so a
	// boundary is unambiguously more informative to merge across than a within-trend step: an earlier version
	// of this test used gaps barely bigger than the internal wobble, which made a trend boundary land, by sheer
	// coincidence of the two trends' wobble phases, on a smaller jump than plenty of jumps *inside* a trend --
	// nothing in the raw numbers themselves distinguished "trend change" from "noise" at that point, so of
	// course a value-blind compressor couldn't preserve a distinction that wasn't actually there in the data.
	constexpr size_t phaseLength = 200;
	constexpr size_t maxSegments = 18;
	TimeSeries<unsigned> timeseries{maxSegments, TIREX_AGG_MEAN};

	auto addPhase = [&](unsigned base, std::chrono::milliseconds& t) {
		constexpr unsigned wobble[] = {0, 1}; // deterministic period-2 wobble, averaging 0.5
		for (size_t i = 0; i < phaseLength; ++i) {
			timeseries.addValue(base + wobble[i % 2], t);
			t += 1ms;
		}
	};

	std::chrono::milliseconds t{0};
	addPhase(3, t); // trend A: 3-4
	addPhase(8, t); // trend B: 8-9
	addPhase(1, t); // trend C: 1-2

	REQUIRE(timeseries.size() <= maxSegments);
	REQUIRE(timeseries.size() >= 6); // enough resolution left to tell the three trends apart at all

	auto [timepoints, values] = timeseries.timeseries();

	// Classify each exported point by which trend's time window its own timestamp falls into, rather than by
	// position in the output: the whole point of this algorithm is that it's free to allocate more or fewer
	// segments to each trend depending on how much internal variation it has, so a fixed by-position split
	// would be checking the wrong thing.
	double sumA = 0, sumB = 0, sumC = 0;
	size_t nA = 0, nB = 0, nC = 0;
	for (size_t i = 0; i < values.size(); ++i) {
		if (timepoints[i] < phaseLength * 1ms) {
			sumA += values[i];
			++nA;
		} else if (timepoints[i] < 2 * phaseLength * 1ms) {
			sumB += values[i];
			++nB;
		} else {
			sumC += values[i];
			++nC;
		}
	}
	REQUIRE(nA > 0);
	REQUIRE(nB > 0);
	REQUIRE(nC > 0);
	CHECK(sumA / static_cast<double>(nA) == Approx(3.5).margin(1.0));
	CHECK(sumB / static_cast<double>(nB) == Approx(8.5).margin(1.0));
	CHECK(sumC / static_cast<double>(nC) == Approx(1.5).margin(1.0));
}

TEST_CASE("Timeseries", "[OddCapacityRegression]") {
	// Bugs.md #15: the old implementation had a parity-dependent out-of-bounds read on an odd buffer size. This
	// design never indexes an array by parity (it always merges exactly one heap-selected adjacent pair via
	// linked-list node merge), so an odd maxSegments driven through many overlapping merges should simply work.
	TimeSeries<unsigned> timeseries{3, TIREX_AGG_MEAN};
	for (unsigned i = 0; i < 50; ++i)
		timeseries.addValue((i * 7) % 13, std::chrono::milliseconds(i));

	CHECK(timeseries.size() <= 3);
}
