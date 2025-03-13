#include "config.hpp"
#include "formatters.hpp"
#include "utils.hpp"

#include <tirex_tracker.h>

#include <CLI/CLI.hpp>

#include <cstdlib>
#include <fstream>
#include <future>
#include <iostream>
#include <ranges>
#include <thread>

static std::map<std::string, std::vector<tirexMeasureConf>> confGroups = {
		{"git",
		 {{TIREX_GIT_IS_REPO, TIREX_AGG_NO},
		  {TIREX_GIT_HASH, TIREX_AGG_NO},
		  {TIREX_GIT_LAST_COMMIT_HASH, TIREX_AGG_NO},
		  {TIREX_GIT_BRANCH, TIREX_AGG_NO},
		  {TIREX_GIT_BRANCH_UPSTREAM, TIREX_AGG_NO},
		  {TIREX_GIT_TAGS, TIREX_AGG_NO},
		  {TIREX_GIT_REMOTE_ORIGIN, TIREX_AGG_NO},
		  {TIREX_GIT_UNCOMMITTED_CHANGES, TIREX_AGG_NO},
		  {TIREX_GIT_UNPUSHED_CHANGES, TIREX_AGG_NO},
		  {TIREX_GIT_UNCHECKED_FILES, TIREX_AGG_NO}}},
		{"system",
		 {{TIREX_OS_NAME, TIREX_AGG_NO},
		  {TIREX_OS_KERNEL, TIREX_AGG_NO},
		  {TIREX_TIME_ELAPSED_WALL_CLOCK_MS, TIREX_AGG_NO},
		  {TIREX_TIME_ELAPSED_USER_MS, TIREX_AGG_NO},
		  {TIREX_TIME_ELAPSED_SYSTEM_MS, TIREX_AGG_NO},
		  {TIREX_CPU_USED_PROCESS_PERCENT, TIREX_AGG_NO},
		  {TIREX_CPU_USED_SYSTEM_PERCENT, TIREX_AGG_NO},
		  {TIREX_CPU_AVAILABLE_SYSTEM_CORES, TIREX_AGG_NO},
		  {TIREX_CPU_FEATURES, TIREX_AGG_NO},
		  {TIREX_CPU_FREQUENCY_MHZ, TIREX_AGG_NO},
		  {TIREX_CPU_FREQUENCY_MIN_MHZ, TIREX_AGG_NO},
		  {TIREX_CPU_FREQUENCY_MAX_MHZ, TIREX_AGG_NO},
		  {TIREX_CPU_VENDOR_ID, TIREX_AGG_NO},
		  {TIREX_CPU_BYTE_ORDER, TIREX_AGG_NO},
		  {TIREX_CPU_ARCHITECTURE, TIREX_AGG_NO},
		  {TIREX_CPU_MODEL_NAME, TIREX_AGG_NO},
		  {TIREX_CPU_CORES_PER_SOCKET, TIREX_AGG_NO},
		  {TIREX_CPU_THREADS_PER_CORE, TIREX_AGG_NO},
		  {TIREX_CPU_CACHES, TIREX_AGG_NO},
		  {TIREX_CPU_VIRTUALIZATION, TIREX_AGG_NO},
		  {TIREX_RAM_USED_PROCESS_KB, TIREX_AGG_NO},
		  {TIREX_RAM_USED_SYSTEM_MB, TIREX_AGG_NO},
		  {TIREX_RAM_AVAILABLE_SYSTEM_MB, TIREX_AGG_NO}}},
		{"energy",
		 {{TIREX_CPU_ENERGY_SYSTEM_JOULES, TIREX_AGG_NO},
		  {TIREX_RAM_ENERGY_SYSTEM_JOULES, TIREX_AGG_NO},
		  {TIREX_GPU_ENERGY_SYSTEM_JOULES, TIREX_AGG_NO}}},
		{"gpu",
		 {{TIREX_GPU_SUPPORTED, TIREX_AGG_NO},
		  {TIREX_GPU_MODEL_NAME, TIREX_AGG_NO},
		  {TIREX_GPU_NUM_CORES, TIREX_AGG_NO},
		  {TIREX_GPU_USED_PROCESS_PERCENT, TIREX_AGG_NO},
		  {TIREX_GPU_USED_SYSTEM_PERCENT, TIREX_AGG_NO},
		  {TIREX_GPU_VRAM_USED_PROCESS_MB, TIREX_AGG_NO},
		  {TIREX_GPU_VRAM_USED_SYSTEM_MB, TIREX_AGG_NO},
		  {TIREX_GPU_VRAM_AVAILABLE_SYSTEM_MB, TIREX_AGG_NO}}}
};

static void logCallback(tirexLogLevel level, const char* component, const char* message) {
	static constexpr spdlog::level::level_enum levels[] = {
			/*[tirexLogLevel::TRACE] =*/spdlog::level::level_enum::trace,
			/*[tirexLogLevel::DEBUG] =*/spdlog::level::level_enum::debug,
			/*[tirexLogLevel::INFO] =*/spdlog::level::level_enum::info,
			/*[tirexLogLevel::WARN] =*/spdlog::level::level_enum::warn,
			/*[tirexLogLevel::ERROR] =*/spdlog::level::level_enum::err,
			/*[tirexLogLevel::CRITICAL] =*/spdlog::level::level_enum::critical
	};
	tirex::getLogger(component)->log(levels[level], message);
}

using tirex::MeasureCmdArgs;

static void setupLoggerArgs(CLI::App& app, tirex::LoggerConf& conf) {
	app.add_flag(
			"-v,--verbose", conf.verbosity,
			"Sets the logger's verbosity. Passing it multiple times increases verbosity."
	);
	app.add_flag("-q,--quiet", conf.quiet, "Supresses all outputs");
}

static void runMeasureCmd(const MeasureCmdArgs& args) {
	// Initialization and setup
	tirex::setVerbosity(args.logConf.getVerbosity());
	auto logger = tirex::getLogger("tirex-tracker");
	logger->info("Measuring command: {}", args.command);

	// Start measuring
	std::vector<tirexMeasureConf> measures;
	for (const auto& provider : args.statproviders) {
		const auto& elements = confGroups.at(provider);
		measures.insert(measures.end(), elements.begin(), elements.end());
	}
	measures.emplace_back(tirexNullConf);

	// Fetch info
	tirexResult* info;
	tirexError err = tirexFetchInfo(measures.data(), &info);
	assert(err == TIREX_SUCCESS);

	tirexMeasureHandle* handle;
	err = tirexStartTracking(measures.data(), args.pollIntervalMs, &handle);
	assert(err == TIREX_SUCCESS);

	// Run the command
	auto exitcode = std::system(args.command.c_str());

	// Stop measuring
	tirexResult* result;
	err = tirexStopTracking(handle, &result);
	assert(err == TIREX_SUCCESS);

	/** \todo Maybe add the exit code as a stat. **/
	if (args.outfile) {
		std::ofstream stream{*args.outfile};
		args.getFormatter()(stream, info, result);
	} else
		args.getFormatter()(std::cout, info, result);
	tirexResultFree(result);
}

int main(int argc, char* argv[]) {
	tirexSetLogCallback(logCallback);
	CLI::App app("Measures runtime, energy, and many other metrics of a specifed command.");
	app.set_version_flag("-V,--version", buildVersionString());
	app.set_help_flag("-h,--help", "Prints this help message");

	MeasureCmdArgs measureArgs;
	setupLoggerArgs(app, measureArgs.logConf);
	app.add_option("command", measureArgs.command, "The command to track resources for")->required();
	app.add_option("--format,-f", measureArgs.formatter, "Specified how the output should be formatted")
			->default_val("simple");
	app.add_option("--source,-s", measureArgs.statproviders, "The datasources to poll information from")
			->default_val(std::vector<std::string>{"git", "system", "energy", "gpu"});
	app.add_option("--poll-interval", measureArgs.pollIntervalMs)
			->description(
					"The interval in milliseconds in which to poll for updated stats like energy consumption and RAM "
					"usage. Smaller intervalls allow for higher accuracy."
			)
			->default_val(100);
	app.add_flag("--pedantic", measureArgs.pedantic, "If set, stop execution on errors")
			->default_val(false); /** \todo support pedantic **/
	app.add_option("-o", measureArgs.outfile)->description("Sets the file to write the result measurements into.");

	app.callback([&measureArgs]() { runMeasureCmd(measureArgs); });

	CLI11_PARSE(app, argc, argv);
	return 0;
}