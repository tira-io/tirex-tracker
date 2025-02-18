#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "formatters.hpp"
#include "logging.hpp"

#include <measure.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace msr {
	struct LoggerConf final {
		bool quiet = false;
		/**
         * @brief Sets the verbosity of the logger. Per default (verbosity=0) only critical, error and warnings are printed.
         * If verbosity is...
         *  - 1: info and above is printed
         *  - 2: debug and above is printed
         *  - 3+: trace and above is printed
         */
		int verbosity = 0;

		msr::Verbosity getVerbosity() const noexcept {
			if (quiet)
				return msr::Verbosity::Off;
			auto verb = static_cast<int>(msr::Verbosity::Warning) + verbosity;
			verb = std::min(verb, static_cast<int>(msr::Verbosity::Trace));
			return static_cast<msr::Verbosity>(verb);
		}
	};

	struct MeasureCmdArgs final {
		LoggerConf logConf;
		std::string command;   /**< The command that should be measured **/
		std::string formatter; /**< The identifier specifying the formatter to use for the output **/
		std::vector<std::string> statproviders;
		size_t pollIntervalMs;
		bool pedantic;

		const ResultFormatter& getFormatter() const {
			static const std::map<std::string, ResultFormatter> formatters{
					{"simple", simpleFormatter},
					{"json", jsonFormatter},
			};
			return formatters.at(formatter);
		}
	};
} // namespace msr

#endif