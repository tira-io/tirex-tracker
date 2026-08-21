#include <tirex_tracker.h>

#include <catch2/catch_test_macros.hpp>

#if __linux__
#include <nlohmann/json.hpp>

#include <unistd.h>

#include <chrono>
#include <csignal>
#include <cstring>
#include <filesystem>
#include <sys/mman.h>
#include <sys/wait.h>
#include <thread>

namespace {
	/** @brief Ensures a forked child is killed and reaped even if a REQUIRE/CHECK below throws mid-test. */
	struct ChildGuard {
		pid_t pid;
		explicit ChildGuard(pid_t pid) : pid(pid) {}
		ChildGuard(const ChildGuard&) = delete;
		ChildGuard& operator=(const ChildGuard&) = delete;
		/** @brief Call after already reaping the child manually, so the destructor doesn't wait on it again. */
		void detach() { pid = -1; }
		~ChildGuard() {
			if (pid > 0) {
				kill(pid, SIGKILL);
				int status;
				waitpid(pid, &status, 0);
			}
		}
	};

	/** @brief Busy-loops (as opposed to sleeping) for \p duration, to register as real CPU time. */
	void burnCpu(std::chrono::milliseconds duration) {
		auto deadline = std::chrono::steady_clock::now() + duration;
		volatile double sink = 0;
		while (std::chrono::steady_clock::now() < deadline)
			sink += 1.0;
	}

	/** @brief Extracts the "max" field of a dynamic (JSON) measure from a tirexResult, failing the test if it's
	 * absent. */
	double extractMax(const tirexResult* result, tirexMeasure measure) {
		size_t n = 0;
		REQUIRE(tirexResultEntryNum(result, &n) == tirexError::TIREX_SUCCESS);
		for (size_t i = 0; i < n; ++i) {
			tirexResultEntry entry;
			REQUIRE(tirexResultEntryGetByIndex(result, i, &entry) == tirexError::TIREX_SUCCESS);
			if (entry.source != measure)
				continue;
			auto json = nlohmann::json::parse(static_cast<const char*>(entry.value));
			REQUIRE(!json.at("max").is_null());
			return json.at("max").get<double>();
		}
		FAIL("Measure " << static_cast<int>(measure) << " not found in result");
		return -1;
	}
} // namespace

TEST_CASE("tirexStartTracking targets an explicit pid, not the caller", "[TrackingTarget]") {
	pid_t child = fork();
	REQUIRE(child >= 0);
	if (child == 0) {
		burnCpu(std::chrono::milliseconds(600));
		_exit(0);
	}
	ChildGuard guard{child};

	tirexMeasureConf measures[] = {{TIREX_CPU_USED_PROCESS_PERCENT, TIREX_AGG_MAX}, tirexNullConf};
	tirexTrackingConf conf{.measures = measures, .pid = child, .trackSubprocesses = false, .pollIntervalMs = 50};
	tirexMeasureHandle* handle;
	REQUIRE(tirexStartTracking(conf, &handle) == tirexError::TIREX_SUCCESS);

	int status;
	waitpid(child, &status, 0);
	guard.detach();

	tirexResult* result;
	REQUIRE(tirexStopTracking(handle, &result) == tirexError::TIREX_SUCCESS);
	auto maxCpuPercent = extractMax(result, TIREX_CPU_USED_PROCESS_PERCENT);
	tirexResultFree(result);

	INFO("max CPU% reported for the targeted child: " << maxCpuPercent);
	CHECK(maxCpuPercent > 50.0);
}

TEST_CASE("trackSubprocesses aggregates a child's CPU usage; disabled mode does not", "[TrackingTarget]") {
	SECTION("enabled: the busy child's CPU time is reflected in the tracked total") {
		pid_t child = fork();
		REQUIRE(child >= 0);
		if (child == 0) {
			burnCpu(std::chrono::milliseconds(600));
			_exit(0);
		}
		ChildGuard guard{child};

		tirexMeasureConf measures[] = {{TIREX_CPU_USED_PROCESS_PERCENT, TIREX_AGG_MAX}, tirexNullConf};
		tirexTrackingConf conf{.measures = measures, .trackSubprocesses = true, .pollIntervalMs = 50};
		tirexMeasureHandle* handle;
		REQUIRE(tirexStartTracking(conf, &handle) == tirexError::TIREX_SUCCESS);

		int status;
		waitpid(child, &status, 0);
		guard.detach();

		tirexResult* result;
		REQUIRE(tirexStopTracking(handle, &result) == tirexError::TIREX_SUCCESS);
		auto maxCpuPercent = extractMax(result, TIREX_CPU_USED_PROCESS_PERCENT);
		tirexResultFree(result);

		INFO("max CPU% reported with trackSubprocesses=true: " << maxCpuPercent);
		CHECK(maxCpuPercent > 50.0);
	}
	SECTION("disabled (default): the busy child's CPU time is not reflected") {
		pid_t child = fork();
		REQUIRE(child >= 0);
		if (child == 0) {
			burnCpu(std::chrono::milliseconds(600));
			_exit(0);
		}
		ChildGuard guard{child};

		tirexMeasureConf measures[] = {{TIREX_CPU_USED_PROCESS_PERCENT, TIREX_AGG_MAX}, tirexNullConf};
		// trackSubprocesses defaults to false; the tracked process (this test) just blocks in waitpid below.
		tirexTrackingConf conf{.measures = measures, .pollIntervalMs = 50};
		tirexMeasureHandle* handle;
		REQUIRE(tirexStartTracking(conf, &handle) == tirexError::TIREX_SUCCESS);

		int status;
		waitpid(child, &status, 0);
		guard.detach();

		tirexResult* result;
		REQUIRE(tirexStopTracking(handle, &result) == tirexError::TIREX_SUCCESS);
		auto maxCpuPercent = extractMax(result, TIREX_CPU_USED_PROCESS_PERCENT);
		tirexResultFree(result);

		INFO("max CPU% reported with trackSubprocesses=false: " << maxCpuPercent);
		CHECK(maxCpuPercent < 50.0);
	}
}

TEST_CASE(
		"trackSubprocesses' RAM aggregation does not double-count memory shared across the tree", "[TrackingTarget]"
) {
	if (!std::filesystem::exists("/proc/self/smaps_rollup") && !std::filesystem::exists("/proc/self/smaps")) {
		SUCCEED("Skipping: neither /proc/<pid>/smaps_rollup nor /proc/<pid>/smaps is available on this kernel");
		return;
	}

	constexpr size_t sharedSize = size_t{100} * 1024 * 1024; // 100 MB, shared across both children.
	constexpr size_t privateSize = size_t{10} * 1024 * 1024; // 10 MB, private to each child.

	void* shared = mmap(nullptr, sharedSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	REQUIRE(shared != MAP_FAILED);
	std::memset(shared, 0xAB, sharedSize); // Fault in all pages before forking so both children inherit them resident.

	auto spawnChild = [&]() -> pid_t {
		pid_t child = fork();
		REQUIRE(child >= 0);
		if (child == 0) {
			void* priv = mmap(nullptr, privateSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
			if (priv == MAP_FAILED)
				_exit(1);
			std::memset(priv, 0xCD, privateSize);
			std::this_thread::sleep_for(std::chrono::milliseconds(600));
			_exit(0);
		}
		return child;
	};

	pid_t child1 = spawnChild();
	ChildGuard guard1{child1};
	pid_t child2 = spawnChild();
	ChildGuard guard2{child2};

	tirexMeasureConf measures[] = {{TIREX_RAM_USED_PROCESS_KB, TIREX_AGG_MAX}, tirexNullConf};
	tirexTrackingConf conf{.measures = measures, .trackSubprocesses = true, .pollIntervalMs = 50};
	tirexMeasureHandle* handle;
	REQUIRE(tirexStartTracking(conf, &handle) == tirexError::TIREX_SUCCESS);

	std::this_thread::sleep_for(std::chrono::milliseconds(400)); // Let both children settle and get sampled.

	tirexResult* result;
	REQUIRE(tirexStopTracking(handle, &result) == tirexError::TIREX_SUCCESS);
	auto maxRamKB = extractMax(result, TIREX_RAM_USED_PROCESS_KB);
	tirexResultFree(result);

	munmap(shared, sharedSize);

	// Naive (double-counted) sum would be ~2*100MB + 2*10MB =~ 225280 KB. The true unique footprint is
	// ~100MB + 2*10MB =~ 122880 KB, plus this test process's own (comparatively small) baseline usage.
	INFO("max RAM reported (KB): " << maxRamKB);
	CHECK(maxRamKB > 60.0 * 1024);
	CHECK(maxRamKB < 200.0 * 1024);
}

#endif
