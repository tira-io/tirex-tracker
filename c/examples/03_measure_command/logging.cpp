#include "logging.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>

#include <format>

using LoggerPtr = tirex::LoggerPtr;

const char* tirex::getVersionStr() noexcept {
	// Uncomment and use std::format once we can assume enough adoption
	// static auto version = std::format("spdlog v.{}.{}.{}", SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH);
	static auto version = std::string("spdlog v.") + std::to_string(SPDLOG_VER_MAJOR) + "." +
						  std::to_string(SPDLOG_VER_MINOR) + "." + std::to_string(SPDLOG_VER_PATCH);
	return version.c_str();
}

void tirex::setVerbosity(tirex::Verbosity verbosity) noexcept {
	auto level = spdlog::level::off - static_cast<int>(verbosity);
	spdlog::set_level(static_cast<spdlog::level::level_enum>(level));
}

LoggerPtr tirex::getLogger(std::string name) {
	auto logger = spdlog::get(name);
	if (logger == nullptr) {
		logger = spdlog::stdout_color_mt(name);
	}
	return logger;
}