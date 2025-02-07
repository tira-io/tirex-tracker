#ifndef LOGGING_HPP
#define LOGGING_HPP

#include <memory>

#include <spdlog/spdlog.h>

namespace msr {
	using LoggerPtr = std::shared_ptr<spdlog::logger>;

	enum class Verbosity : int { Off, Critical, Error, Warning, Info, Debug, Trace };

	const char* getVersionStr() noexcept;

	/**
	 * @brief Globally set the verbosity for all loggers to the one specified.
	 * 
	 * @param verbosity The verbosity to use for logging.
	 */
	void setVerbosity(Verbosity verbosity) noexcept;

	/**
	 * @brief Creates a new named logger instance with the given name or fetches an existing one associated with the
	 * name.
	 * 
	 * @param name The name of the the logger to be returned.
	 * @return A logger with the specified name.
	 */
	LoggerPtr getLogger(std::string name);
} // namespace msr

#endif