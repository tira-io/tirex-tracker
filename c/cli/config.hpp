#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "formatters.hpp"
#include "logging.hpp"

#include <tirex_tracker.h>

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace tirex {
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

		tirex::Verbosity getVerbosity() const noexcept {
			if (quiet)
				return tirex::Verbosity::Off;
			auto verb = static_cast<int>(tirex::Verbosity::Warning) + verbosity;
			verb = std::min(verb, static_cast<int>(tirex::Verbosity::Trace));
			return static_cast<tirex::Verbosity>(verb);
		}
	};

	struct MeasureCmdArgs final {
		LoggerConf logConf;
		std::string command;   /**< The command that should be measured **/
		std::string formatter; /**< The identifier specifying the formatter to use for the output **/
		std::vector<std::string> statproviders;
		size_t pollIntervalMs;
		bool pedantic;
		std::optional<std::string> outfile;

		const ResultFormatter& getFormatter() const {
			static const std::map<std::string, ResultFormatter> formatters{
					{"simple", simpleFormatter},
					{"json", jsonFormatter},
					{"irmetadata", irmetadataFormatter},
			};
			return formatters.at(formatter);
		}
	};
} // namespace tirex

#endif