#include "config.hpp"
#include "formatters.hpp"
#include "utils.hpp"

#include <measure.h>

#include <CLI/CLI.hpp>

#include <cstdlib>
#include <future>
#include <iostream>
#include <ranges>
#include <thread>

static std::map<std::string, std::vector<msrMeasureConf>> confGroups = {
		{"git",
		 {{MSR_GIT_IS_REPO, MSR_AGG_NO},
		  {MSR_GIT_HASH, MSR_AGG_NO},
		  {MSR_GIT_LAST_COMMIT_HASH, MSR_AGG_NO},
		  {MSR_GIT_BRANCH, MSR_AGG_NO},
		  {MSR_GIT_BRANCH_UPSTREAM, MSR_AGG_NO},
		  {MSR_GIT_TAGS, MSR_AGG_NO},
		  {MSR_GIT_REMOTE_ORIGIN, MSR_AGG_NO},
		  {MSR_GIT_UNCOMMITTED_CHANGES, MSR_AGG_NO},
		  {MSR_GIT_UNPUSHED_CHANGES, MSR_AGG_NO},
		  {MSR_GIT_UNCHECKED_FILES, MSR_AGG_NO}}},
		{"system",
		 {{MSR_OS_NAME, MSR_AGG_NO},
		  {MSR_OS_KERNEL, MSR_AGG_NO},
		  {MSR_TIME_ELAPSED_WALL_CLOCK_MS, MSR_AGG_NO},
		  {MSR_TIME_ELAPSED_USER_MS, MSR_AGG_NO},
		  {MSR_TIME_ELAPSED_SYSTEM_MS, MSR_AGG_NO},
		  {MSR_CPU_USED_PROCESS_PERCENT, MSR_AGG_NO},
		  {MSR_CPU_USED_SYSTEM_PERCENT, MSR_AGG_NO},
		  {MSR_CPU_AVAILABLE_SYSTEM_CORES, MSR_AGG_NO},
		  {MSR_CPU_FEATURES, MSR_AGG_NO},
		  {MSR_CPU_FREQUENCY_MHZ, MSR_AGG_NO},
		  {MSR_CPU_FREQUENCY_MIN_MHZ, MSR_AGG_NO},
		  {MSR_CPU_FREQUENCY_MAX_MHZ, MSR_AGG_NO},
		  {MSR_CPU_VENDOR_ID, MSR_AGG_NO},
		  {MSR_CPU_BYTE_ORDER, MSR_AGG_NO},
		  {MSR_CPU_ARCHITECTURE, MSR_AGG_NO},
		  {MSR_CPU_MODEL_NAME, MSR_AGG_NO},
		  {MSR_CPU_CORES_PER_SOCKET, MSR_AGG_NO},
		  {MSR_CPU_THREADS_PER_CORE, MSR_AGG_NO},
		  {MSR_CPU_CACHES, MSR_AGG_NO},
		  {MSR_CPU_VIRTUALIZATION, MSR_AGG_NO},
		  {MSR_RAM_USED_PROCESS_KB, MSR_AGG_NO},
		  {MSR_RAM_USED_SYSTEM_MB, MSR_AGG_NO},
		  {MSR_RAM_AVAILABLE_SYSTEM_MB, MSR_AGG_NO}}},
		{"energy",
		 {{MSR_CPU_ENERGY_SYSTEM_JOULES, MSR_AGG_NO},
		  {MSR_RAM_ENERGY_SYSTEM_JOULES, MSR_AGG_NO},
		  {MSR_GPU_ENERGY_SYSTEM_JOULES, MSR_AGG_NO}}},
		{"gpu",
		 {{MSR_GPU_SUPPORTED, MSR_AGG_NO},
		  {MSR_GPU_MODEL_NAME, MSR_AGG_NO},
		  {MSR_GPU_NUM_CORES, MSR_AGG_NO},
		  {MSR_GPU_USED_PROCESS_PERCENT, MSR_AGG_NO},
		  {MSR_GPU_USED_SYSTEM_PERCENT, MSR_AGG_NO},
		  {MSR_GPU_VRAM_USED_PROCESS_MB, MSR_AGG_NO},
		  {MSR_GPU_VRAM_USED_SYSTEM_MB, MSR_AGG_NO},
		  {MSR_GPU_VRAM_AVAILABLE_SYSTEM_MB, MSR_AGG_NO}}}
};

static void logCallback(msrLogLevel level, const char* component, const char* message) {
	static constexpr spdlog::level::level_enum levels[] = {
			[msrLogLevel::TRACE] = spdlog::level::level_enum::trace,
			[msrLogLevel::DEBUG] = spdlog::level::level_enum::debug,
			[msrLogLevel::INFO] = spdlog::level::level_enum::info,
			[msrLogLevel::WARN] = spdlog::level::level_enum::warn,
			[msrLogLevel::ERROR] = spdlog::level::level_enum::err,
			[msrLogLevel::CRITICAL] = spdlog::level::level_enum::critical
	};
	msr::getLogger(component)->log(levels[level], message);
}

using msr::MeasureCmdArgs;

static void setupLoggerArgs(CLI::App& app, msr::LoggerConf& conf) {
	app.add_flag(
			"-v,--verbose", conf.verbosity,
			"Sets the logger's verbosity. Passing it multiple times increases verbosity."
	);
	app.add_flag("-q,--quiet", conf.quiet, "Supresses all outputs");
}

static void runMeasureCmd(const MeasureCmdArgs& args) {
	// Initialization and setup
	msr::setVerbosity(args.logConf.getVerbosity());
	auto logger = msr::getLogger("measure");
	logger->info("Measuring command: {}", args.command);

	// Start measuring
	std::vector<msrMeasureConf> measures;
	for (const auto& provider : args.statproviders) {
		const auto& elements = confGroups.at(provider);
		measures.insert(measures.end(), elements.begin(), elements.end());
	}
	measures.emplace_back(msrNullConf);

	msrMeasureHandle* handle;
	assert(msrStartMeasure(measures.data(), args.pollIntervalMs, &handle) == MSR_SUCCESS);

	// Run the command
	auto exitcode = std::system(args.command.c_str());

	// Stop measuring
	msrResult* result;
	assert(msrStopMeasure(handle, &result) == MSR_SUCCESS);

	/** \todo Maybe add the exit code as a stat. **/
	std::cout << "\n== RESULTS ==" << std::endl;
	args.getFormatter()(std::cout, result);
	msrResultFree(result);
}

int main(int argc, char* argv[]) {
	msrSetLogCallback(logCallback);
	CLI::App app("Measures runtime, energy, and many other metrics of a specifed command.");
	app.set_version_flag("-V,--version", buildVersionString());
	app.set_help_flag("-h,--help", "Prints this help message");

	MeasureCmdArgs measureArgs;
	setupLoggerArgs(app, measureArgs.logConf);
	app.add_option("command", measureArgs.command, "The command to measure resources for")->required();
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
	app.add_flag("--pedantic", measureArgs.pedantic, "If set, measure will stop execution on errors")
			->default_val(false); /** \todo support pedantic **/

	app.callback([&measureArgs]() { runMeasureCmd(measureArgs); });

	CLI11_PARSE(app, argc, argv);
	return 0;
}