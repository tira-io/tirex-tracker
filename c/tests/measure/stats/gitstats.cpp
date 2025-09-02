#include <measure/stats/gitstats.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

using Catch::Matchers::RangeEquals;

using namespace tirex::ts;
using namespace std::chrono_literals;

#include <iostream>

TEST_CASE("Git Stats", "[Archive]") {
	tirexSetLogCallback(+[](tirexLogLevel lvl, const char* component, const char* msg) {
		std::cout << "[" << lvl << "][" << component << "] " << msg << std::endl;
	});
	{
	}
}