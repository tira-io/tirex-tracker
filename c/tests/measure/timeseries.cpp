#include <measure/timeseries.hpp>

#include <tirex_tracker.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

using Catch::Matchers::RangeEquals;

using namespace tirex::ts;
using namespace std::chrono_literals;

#include <iostream>

TEST_CASE("Timeseries", "[Batched]") {
	tirexSetLogCallback(+[](tirexLogLevel lvl, const char* component, const char* msg) {
		std::cout << "[" << lvl << "][" << component << "] " << msg << std::endl;
	});
	{
		auto timeseries = store<unsigned>() | Batched(10ms, TIREX_AGG_MEAN, 2);
		CHECK(timeseries.size() == 0);

		timeseries.addValue(0, 2ms);
		CHECK(timeseries.size() == 1);
		CHECK_THAT(std::get<0>(timeseries.timeseries()), RangeEquals(std::vector{2ms}));
		CHECK_THAT(std::get<1>(timeseries.timeseries()), RangeEquals(std::vector{0}));

		timeseries.addValue(1, 5ms); // Not aggregated because batching starts only after size==1
		CHECK(timeseries.size() == 2);
		CHECK_THAT(std::get<0>(timeseries.timeseries()), RangeEquals(std::vector{2ms, 5ms}));
		CHECK_THAT(std::get<1>(timeseries.timeseries()), RangeEquals(std::vector{0, 1}));

		timeseries.addValue(3, 14ms); // Aggregated because within the batch size
		CHECK(timeseries.size() == 2);
		CHECK_THAT(std::get<0>(timeseries.timeseries()), RangeEquals(std::vector{2ms, 5ms}));
		CHECK_THAT(std::get<1>(timeseries.timeseries()), RangeEquals(std::vector{0, (1 + 3) / 2}));

		timeseries.addValue(3, 16ms);
		CHECK(timeseries.size() == 3);
		CHECK_THAT(std::get<0>(timeseries.timeseries()), RangeEquals(std::vector{2ms, 5ms, 16ms}));
		CHECK_THAT(std::get<1>(timeseries.timeseries()), RangeEquals(std::vector{0, 2, 3}));

		timeseries.addValue(4, 27ms);
		CHECK(timeseries.size() == 4);
		CHECK_THAT(std::get<0>(timeseries.timeseries()), RangeEquals(std::vector{2ms, 5ms, 16ms, 27ms}));
		CHECK_THAT(std::get<1>(timeseries.timeseries()), RangeEquals(std::vector{0, 2, 3, 4}));
	}
}

TEST_CASE("Timeseries", "[Limit]") {
	{
		auto timeseries = store<unsigned>() | Limit(4, TIREX_AGG_MAX);
		REQUIRE(timeseries.size() == 0);
		timeseries.addValue(0);
		REQUIRE(timeseries.size() == 1);
		CHECK_THAT(std::get<1>(timeseries.timeseries()), RangeEquals(std::vector{0}));
		timeseries.addValue(1);
		REQUIRE(timeseries.size() == 2);
		CHECK_THAT(std::get<1>(timeseries.timeseries()), RangeEquals(std::vector{0, 1}));
		timeseries.addValue(2);
		REQUIRE(timeseries.size() == 3);
		CHECK_THAT(std::get<1>(timeseries.timeseries()), RangeEquals(std::vector{0, 1, 2}));
		timeseries.addValue(3);
		REQUIRE(timeseries.size() == 4);
		CHECK_THAT(std::get<1>(timeseries.timeseries()), RangeEquals(std::vector{0, 1, 2, 3}));
		timeseries.addValue(4);
		REQUIRE(timeseries.size() == 3);
		CHECK_THAT(std::get<1>(timeseries.timeseries()), RangeEquals(std::vector{1, 3, 4}));
		timeseries.addValue(5);
		REQUIRE(timeseries.size() == 4);
		CHECK_THAT(std::get<1>(timeseries.timeseries()), RangeEquals(std::vector{1, 3, 4, 5}));
		timeseries.addValue(6);
		REQUIRE(timeseries.size() == 3);
		CHECK_THAT(std::get<1>(timeseries.timeseries()), RangeEquals(std::vector{3, 5, 6}));
	}
}