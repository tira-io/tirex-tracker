#include <measure/stats/gitstats.hpp>

#include "../../test_utils/cwdscopeguard.hpp"
#include "../../test_utils/mockfolder.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include <zip.h>

#include <iostream>

#if defined(__linux__)
#include <unistd.h>

#include <csignal>
#include <sys/wait.h>
#endif

using Catch::Matchers::RangeEquals;

using namespace tirex;

TEST_CASE("Git Stats", "[Archive]") {
	tirexSetLogCallback(+[](tirexLogLevel lvl, const char* component, const char* msg) {
		std::cout << "[" << lvl << "][" << component << "] " << msg << std::endl;
	});
	{
		// Create Mock Files
		MockFolder folder{"tirex-tests"};
		folder.addFile({.relpath = "a.txt", .stageInGit = true, .commitToGit = true, .content = "file a"});
		folder.addFile({.relpath = "b.txt", .stageInGit = true, .commitToGit = true, .content = "file b"});
		folder.addFile({.relpath = "c.txt", .stageInGit = false, .commitToGit = false, .content = "file c"});
		folder.addFile({.relpath = "d.txt", .stageInGit = true, .commitToGit = true, .content = "file d"});
		folder.addFile({.relpath = "d.txt", .stageInGit = true, .commitToGit = false, .content = "modified d"});

		{
			CWDScopeGuard cwdguard{folder.path()};
			GitStats stats{TrackingTarget{}};
			stats.requestMeasures(
					{tirexMeasure::TIREX_GIT_UNCOMMITTED_CHANGES, tirexMeasure::TIREX_GIT_UNCHECKED_FILES,
					 tirexMeasure::TIREX_GIT_ARCHIVE_PATH, tirexMeasure::TIREX_GIT_ROOT}
			);
			auto info = stats.getInfo();

			{ // Check Git Root
				REQUIRE(info.find(tirexMeasure::TIREX_GIT_ROOT) != info.end());
				auto& entry = info[tirexMeasure::TIREX_GIT_ROOT];
				REQUIRE(std::holds_alternative<std::string>(entry));
				CHECK(std::filesystem::canonical({std::get<std::string>(entry)}) ==
					  std::filesystem::canonical(folder.path()));
			}
			{ // Check Uncommitted Changes
				REQUIRE(info.find(tirexMeasure::TIREX_GIT_UNCOMMITTED_CHANGES) != info.end());
				auto& entry = info[tirexMeasure::TIREX_GIT_UNCOMMITTED_CHANGES];
				REQUIRE(std::holds_alternative<std::string>(entry));
				CHECK(std::get<std::string>(entry) == "1");
			}
			{ // Check Unchecked Files
				REQUIRE(info.find(tirexMeasure::TIREX_GIT_UNCHECKED_FILES) != info.end());
				auto& entry = info[tirexMeasure::TIREX_GIT_UNCHECKED_FILES];
				REQUIRE(std::holds_alternative<std::string>(entry));
				CHECK(std::get<std::string>(entry) == "1");
			}
			{ // Check Archive
				REQUIRE(info.find(tirexMeasure::TIREX_GIT_ARCHIVE_PATH) != info.end());
				auto& entry = info[tirexMeasure::TIREX_GIT_ARCHIVE_PATH];
				REQUIRE(std::holds_alternative<TmpFile>(entry));
				TmpFile& file = std::get<TmpFile>(entry);
				REQUIRE(std::filesystem::exists(file.path));

				zip_t* archive;
				REQUIRE((archive = zip_open(file.path.c_str(), 0, nullptr)) != nullptr);
				auto numEntries = zip_get_num_entries(archive, 0);
				CHECK(numEntries == 4);

				auto check = [&](std::string_view path, const std::string& content) {
					zip_file_t* fd;
					REQUIRE((fd = zip_fopen(archive, path.data(), 0)) != nullptr);
					std::string actual;
					char buffer[4096];
					for (size_t n = 0; (n = zip_fread(fd, buffer, sizeof(buffer))) > 0;)
						actual += std::string_view{buffer, buffer + n};
					zip_fclose(fd);
					// Under macOS Catch2 does not seem to like std::string_view inside CHECK(...)
					CHECK(content == actual);
				};
				check("a.txt", "file a");
				check("b.txt", "file b");
				check("c.txt", "file c");
				check("d.txt", "modified d");

				zip_close(archive);
			}
		}
	}
}

#if defined(__linux__)
TEST_CASE("Git Stats tracks the target process's repository, not the caller's", "[TrackingTarget]") {
	MockFolder folder{"tirex-tests-target"};
	folder.addFile({.relpath = "a.txt", .stageInGit = true, .commitToGit = true, .content = "file a"});

	// Spawn a child process that lives inside the mock repo while the test process itself does not, so that observing
	// the mock repo's root proves GitStats resolved the target's cwd and not its own.
	int syncPipe[2];
	REQUIRE(pipe(syncPipe) == 0);
	pid_t child = fork();
	REQUIRE(child >= 0);
	if (child == 0) {
		close(syncPipe[0]);
		if (chdir(folder.path().c_str()) != 0)
			_exit(1);
		char signal = 'r';
		if (write(syncPipe[1], &signal, 1) != 1)
			_exit(1);
		close(syncPipe[1]);
		pause();
		_exit(0);
	}
	close(syncPipe[1]);
	char signal;
	REQUIRE(read(syncPipe[0], &signal, 1) == 1);
	close(syncPipe[0]);

	GitStats stats{TrackingTarget{.pid = child}};
	stats.requestMeasures({tirexMeasure::TIREX_GIT_ROOT});
	auto info = stats.getInfo();

	kill(child, SIGKILL);
	int status;
	waitpid(child, &status, 0);

	REQUIRE(info.find(tirexMeasure::TIREX_GIT_ROOT) != info.end());
	auto& entry = info[tirexMeasure::TIREX_GIT_ROOT];
	REQUIRE(std::holds_alternative<std::string>(entry));
	CHECK(std::filesystem::canonical({std::get<std::string>(entry)}) == std::filesystem::canonical(folder.path()));
}
#endif