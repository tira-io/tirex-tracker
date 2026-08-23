#include <measure/utils/osutils.hpp>

#include <tirex_tracker.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

#if defined(__linux__) || defined(__APPLE__)
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <csignal>
#include <sys/wait.h>
#include <thread>
#include <vector>

using Catch::Matchers::UnorderedRangeEquals;

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
} // namespace

TEST_CASE("discoverProcessTree resolves TIREX_PID_SELF to the calling process", "[TrackingTarget]") {
	auto tree = tirex::utils::discoverProcessTree(TIREX_PID_SELF);
	CHECK(std::find(tree.begin(), tree.end(), static_cast<int64_t>(getpid())) != tree.end());
}

TEST_CASE("discoverProcessTree includes direct children", "[TrackingTarget]") {
	pid_t child1 = fork();
	REQUIRE(child1 >= 0);
	if (child1 == 0) {
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		_exit(0);
	}
	ChildGuard guard1{child1};

	pid_t child2 = fork();
	REQUIRE(child2 >= 0);
	if (child2 == 0) {
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		_exit(0);
	}
	ChildGuard guard2{child2};

	// Give both children a moment to show up under /proc before scanning.
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	auto tree = tirex::utils::discoverProcessTree(getpid());
	CHECK_THAT(
			tree, UnorderedRangeEquals(std::vector<int64_t>{
						  static_cast<int64_t>(getpid()), static_cast<int64_t>(child1), static_cast<int64_t>(child2)
				  })
	);
}

TEST_CASE("discoverProcessTree includes transitively spawned grandchildren", "[TrackingTarget]") {
	int pipefds[2];
	REQUIRE(pipe(pipefds) == 0);

	pid_t child = fork();
	REQUIRE(child >= 0);
	if (child == 0) {
		close(pipefds[0]);
		pid_t grandchild = fork();
		if (grandchild == 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			_exit(0);
		}
		if (grandchild > 0) {
			auto written = write(pipefds[1], &grandchild, sizeof(grandchild));
			(void)written;
		}
		close(pipefds[1]);
		// Wait for our own child so its parent-child link stays valid for the whole test, rather than being
		// reparented away the moment we exit.
		int status;
		waitpid(grandchild, &status, 0);
		_exit(0);
	}
	close(pipefds[1]);
	ChildGuard childGuard{child};

	pid_t grandchild = -1;
	REQUIRE(read(pipefds[0], &grandchild, sizeof(grandchild)) == sizeof(grandchild));
	close(pipefds[0]);
	REQUIRE(grandchild > 0);

	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	auto tree = tirex::utils::discoverProcessTree(getpid());
	CHECK_THAT(
			tree, UnorderedRangeEquals(std::vector<int64_t>{
						  static_cast<int64_t>(getpid()), static_cast<int64_t>(child), static_cast<int64_t>(grandchild)
				  })
	);
}

TEST_CASE(
		"discoverProcessTree does not count a multi-threaded child's own threads as separate processes",
		"[TrackingTarget]"
) {
	pid_t child = fork();
	REQUIRE(child >= 0);
	if (child == 0) {
		// On Linux, every thread here gets its own top-level /proc/<tid> entry; discoverProcessTree must not mistake
		// any of them for a separate child process (see osutils.hpp's Tgid-filtering contract). On macOS,
		// proc_listchildpids only ever reports child processes, but this still guards against a regression there.
		std::vector<std::thread> threads;
		for (int i = 0; i < 3; ++i)
			threads.emplace_back([] { std::this_thread::sleep_for(std::chrono::milliseconds(500)); });
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		for (auto& t : threads)
			t.join();
		_exit(0);
	}
	ChildGuard guard{child};

	// Give the child's extra threads time to start (and thus show up under /proc) before scanning.
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	auto tree = tirex::utils::discoverProcessTree(getpid());
	CHECK_THAT(
			tree,
			UnorderedRangeEquals(std::vector<int64_t>{static_cast<int64_t>(getpid()), static_cast<int64_t>(child)})
	);
}

TEST_CASE("discoverProcessTree of a vanished pid returns just that pid", "[TrackingTarget]") {
	pid_t child = fork();
	REQUIRE(child >= 0);
	if (child == 0)
		_exit(0);
	int status;
	REQUIRE(waitpid(child, &status, 0) == child); // Fully reaped: `child` is now a PID no process holds.

	auto tree = tirex::utils::discoverProcessTree(child);
	CHECK_THAT(tree, UnorderedRangeEquals(std::vector<int64_t>{static_cast<int64_t>(child)}));
}

#endif
