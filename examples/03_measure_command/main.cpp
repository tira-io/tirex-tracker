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
		/** \todo implement **/
		throw std::runtime_error("Not yet implemented");
	}
	measures.emplace_back(msrNullConf);

	msrMeasureHandle* handle;
	assert(msrStartMeasure(measures.data(), pollIntervalMs, &handle) == MSR_SUCCESS);

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