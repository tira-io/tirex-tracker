#include <tirex_tracker.h>

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <set>
#include <thread>

static std::set<tirexMeasure> collectSources(const tirexResult* result) {
	std::set<tirexMeasure> sources;
	size_t num = 0;
	REQUIRE(tirexResultEntryNum(result, &num) == TIREX_SUCCESS);
	for (size_t i = 0; i < num; ++i) {
		tirexResultEntry entry{};
		REQUIRE(tirexResultEntryGetByIndex(result, i, &entry) == TIREX_SUCCESS);
		sources.insert(entry.source);
	}
	return sources;
}

TEST_CASE("tirexPeekResult returns TIREX_INVALID_ARGUMENT for null handle", "[peek]") {
	tirexResult* result = nullptr;
	CHECK(tirexPeekResult(nullptr, &result) == TIREX_INVALID_ARGUMENT);
}

TEST_CASE("tirexPeekResult returns TIREX_INVALID_ARGUMENT for null result pointer", "[peek]") {
	tirexMeasureConf conf[]{{TIREX_CPU_USED_PROCESS_PERCENT, TIREX_AGG_NO}, tirexNullConf};
	tirexMeasureHandle* handle = nullptr;
	REQUIRE(tirexStartTracking(conf, 50, &handle) == TIREX_SUCCESS);
	REQUIRE(handle != nullptr);

	CHECK(tirexPeekResult(handle, nullptr) == TIREX_INVALID_ARGUMENT);

	tirexResult* result = nullptr;
	REQUIRE(tirexStopTracking(handle, &result) == TIREX_SUCCESS);
	tirexResultFree(result);
}

TEST_CASE("tirexPeekResult returns a non-empty result while tracking is ongoing", "[peek]") {
	tirexMeasureConf conf[]{
			{TIREX_CPU_USED_PROCESS_PERCENT, TIREX_AGG_NO}, {TIREX_RAM_USED_PROCESS_KB, TIREX_AGG_NO}, tirexNullConf
	};
	tirexMeasureHandle* handle = nullptr;
	REQUIRE(tirexStartTracking(conf, 50, &handle) == TIREX_SUCCESS);

	// Give the monitor thread at least one poll cycle
	std::this_thread::sleep_for(std::chrono::milliseconds(200));

	tirexResult* snapshot = nullptr;
	REQUIRE(tirexPeekResult(handle, &snapshot) == TIREX_SUCCESS);
	REQUIRE(snapshot != nullptr);

	size_t num = 0;
	REQUIRE(tirexResultEntryNum(snapshot, &num) == TIREX_SUCCESS);
	CHECK(num > 0);

	tirexResultFree(snapshot);

	tirexResult* finalResult = nullptr;
	REQUIRE(tirexStopTracking(handle, &finalResult) == TIREX_SUCCESS);
	tirexResultFree(finalResult);
}

TEST_CASE("tirexPeekResult snapshot contains requested time-series measures", "[peek]") {
	tirexMeasureConf conf[]{
			{TIREX_CPU_USED_PROCESS_PERCENT, TIREX_AGG_NO}, {TIREX_RAM_USED_PROCESS_KB, TIREX_AGG_NO}, tirexNullConf
	};
	tirexMeasureHandle* handle = nullptr;
	REQUIRE(tirexStartTracking(conf, 50, &handle) == TIREX_SUCCESS);

	std::this_thread::sleep_for(std::chrono::milliseconds(200));

	tirexResult* snapshot = nullptr;
	REQUIRE(tirexPeekResult(handle, &snapshot) == TIREX_SUCCESS);

	auto sources = collectSources(snapshot);
	CHECK(sources.count(TIREX_CPU_USED_PROCESS_PERCENT) == 1);
	CHECK(sources.count(TIREX_RAM_USED_PROCESS_KB) == 1);

	tirexResultFree(snapshot);

	tirexResult* finalResult = nullptr;
	REQUIRE(tirexStopTracking(handle, &finalResult) == TIREX_SUCCESS);
	tirexResultFree(finalResult);
}

TEST_CASE("tirexPeekResult snapshot contains elapsed wall-clock time", "[peek]") {
	tirexMeasureConf conf[]{{TIREX_TIME_ELAPSED_WALL_CLOCK_MS, TIREX_AGG_NO}, tirexNullConf};
	tirexMeasureHandle* handle = nullptr;
	REQUIRE(tirexStartTracking(conf, 50, &handle) == TIREX_SUCCESS);

	std::this_thread::sleep_for(std::chrono::milliseconds(150));

	tirexResult* snapshot = nullptr;
	REQUIRE(tirexPeekResult(handle, &snapshot) == TIREX_SUCCESS);

	auto sources = collectSources(snapshot);
	CHECK(sources.count(TIREX_TIME_ELAPSED_WALL_CLOCK_MS) == 1);

	// Elapsed time should be > 0 and less than 10 seconds
	size_t num = 0;
	tirexResultEntryNum(snapshot, &num);
	for (size_t i = 0; i < num; ++i) {
		tirexResultEntry entry{};
		tirexResultEntryGetByIndex(snapshot, i, &entry);
		if (entry.source == TIREX_TIME_ELAPSED_WALL_CLOCK_MS) {
			long long elapsed = std::atoll(static_cast<const char*>(entry.value));
			CHECK(elapsed > 0);
			CHECK(elapsed < 10000);
		}
	}

	tirexResultFree(snapshot);

	tirexResult* finalResult = nullptr;
	REQUIRE(tirexStopTracking(handle, &finalResult) == TIREX_SUCCESS);
	tirexResultFree(finalResult);
}

TEST_CASE("tirexPeekResult snapshot does NOT contain TIREX_TIME_STOP", "[peek]") {
	tirexMeasureConf conf[]{{TIREX_TIME_STOP, TIREX_AGG_NO}, tirexNullConf};
	tirexMeasureHandle* handle = nullptr;
	REQUIRE(tirexStartTracking(conf, 50, &handle) == TIREX_SUCCESS);

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	tirexResult* snapshot = nullptr;
	REQUIRE(tirexPeekResult(handle, &snapshot) == TIREX_SUCCESS);

	auto sources = collectSources(snapshot);
	CHECK(sources.count(TIREX_TIME_STOP) == 0);

	tirexResultFree(snapshot);

	tirexResult* finalResult = nullptr;
	REQUIRE(tirexStopTracking(handle, &finalResult) == TIREX_SUCCESS);
	tirexResultFree(finalResult);
}

TEST_CASE("tirexPeekResult can be called multiple times during a single tracking session", "[peek]") {
	tirexMeasureConf conf[]{{TIREX_CPU_USED_PROCESS_PERCENT, TIREX_AGG_NO}, tirexNullConf};
	tirexMeasureHandle* handle = nullptr;
	REQUIRE(tirexStartTracking(conf, 50, &handle) == TIREX_SUCCESS);

	for (int i = 0; i < 3; ++i) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		tirexResult* snapshot = nullptr;
		REQUIRE(tirexPeekResult(handle, &snapshot) == TIREX_SUCCESS);
		REQUIRE(snapshot != nullptr);
		tirexResultFree(snapshot);
	}

	tirexResult* finalResult = nullptr;
	REQUIRE(tirexStopTracking(handle, &finalResult) == TIREX_SUCCESS);
	tirexResultFree(finalResult);
}

TEST_CASE("tirexPeekResult result is independent of the final tirexStopTracking result", "[peek]") {
	tirexMeasureConf conf[]{{TIREX_CPU_USED_PROCESS_PERCENT, TIREX_AGG_NO}, tirexNullConf};
	tirexMeasureHandle* handle = nullptr;
	REQUIRE(tirexStartTracking(conf, 50, &handle) == TIREX_SUCCESS);

	std::this_thread::sleep_for(std::chrono::milliseconds(150));

	tirexResult* snapshot = nullptr;
	REQUIRE(tirexPeekResult(handle, &snapshot) == TIREX_SUCCESS);

	tirexResult* finalResult = nullptr;
	REQUIRE(tirexStopTracking(handle, &finalResult) == TIREX_SUCCESS);

	// Both results must be independently valid and freeable
	size_t snapNum = 0;
	size_t finalNum = 0;
	CHECK(tirexResultEntryNum(snapshot, &snapNum) == TIREX_SUCCESS);
	CHECK(tirexResultEntryNum(finalResult, &finalNum) == TIREX_SUCCESS);
	CHECK(snapNum > 0);
	CHECK(finalNum > 0);

	tirexResultFree(snapshot);
	tirexResultFree(finalResult);
}
