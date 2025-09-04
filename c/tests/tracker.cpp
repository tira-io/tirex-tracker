#include <tirex_tracker.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

#include <filesystem>

#include <iostream>

TEST_CASE("Tracker", "[Archive]") {
	tirexSetLogCallback(+[](tirexLogLevel lvl, const char* component, const char* msg) {
		std::cout << "[" << lvl << "][" << component << "] " << msg << std::endl;
	});
	{
		tirexMeasureConf conf[]{{TIREX_GIT_ARCHIVE_PATH, TIREX_AGG_NO}, tirexNullConf};
		tirexResult* result;
		REQUIRE(tirexFetchInfo(conf, &result) == tirexError::TIREX_SUCCESS);

		size_t entrynum;
		tirexResultEntry entry;
		REQUIRE(tirexResultEntryNum(result, &entrynum) == tirexError::TIREX_SUCCESS);
		REQUIRE(entrynum == 1);
		REQUIRE(tirexResultEntryGetByIndex(result, 0, &entry) == tirexError::TIREX_SUCCESS);
		REQUIRE(entry.source == TIREX_GIT_ARCHIVE_PATH);
		REQUIRE(entry.type == tirexResultType::TIREX_STRING);

		CHECK(std::filesystem::is_regular_file(static_cast<const char*>(entry.value)));

		tirexResultFree(result);
		CHECK_FALSE(std::filesystem::exists(static_cast<const char*>(entry.value)));
	}
}