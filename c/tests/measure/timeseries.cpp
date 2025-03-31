#include <measure/timeseries.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

using Catch::Matchers::RangeEquals;

using tirex::MaxDataPoints;
using tirex::TimeSeries;

TEST_CASE("Timeseries", "[MaxDataPoints]") {
	{
		auto timeseries = TimeSeries<unsigned>::create<MaxDataPoints<unsigned>>(4, TIREX_AGG_MAX);
		timeseries.addValue(0);
		REQUIRE(timeseries.timeseries().first.size() == 1);
		CHECK_THAT(timeseries.timeseries().second, RangeEquals(std::vector{0}));
		timeseries.addValue(1);
		REQUIRE(timeseries.timeseries().first.size() == 2);
		CHECK_THAT(timeseries.timeseries().second, RangeEquals(std::vector{0, 1}));
		timeseries.addValue(2);
		REQUIRE(timeseries.timeseries().first.size() == 3);
		CHECK_THAT(timeseries.timeseries().second, RangeEquals(std::vector{0, 1, 2}));
		timeseries.addValue(3);
		REQUIRE(timeseries.timeseries().first.size() == 4);
		CHECK_THAT(timeseries.timeseries().second, RangeEquals(std::vector{0, 1, 2, 3}));
		timeseries.addValue(4);
		REQUIRE(timeseries.timeseries().first.size() == 3);
		CHECK_THAT(timeseries.timeseries().second, RangeEquals(std::vector{1, 3, 4}));
		timeseries.addValue(5);
		REQUIRE(timeseries.timeseries().first.size() == 4);
		CHECK_THAT(timeseries.timeseries().second, RangeEquals(std::vector{1, 3, 4, 5}));
		timeseries.addValue(6);
		REQUIRE(timeseries.timeseries().first.size() == 3);
		CHECK_THAT(timeseries.timeseries().second, RangeEquals(std::vector{3, 5, 6}));
	}
}