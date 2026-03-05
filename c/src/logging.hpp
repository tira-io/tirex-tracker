#ifndef LOGGING_HPP
#define LOGGING_HPP

#include <tirex_tracker.h>

#if __cpp_lib_format and __cplusplus >= 202207L // __cplusplus >= 202207L required for std::format_string
#include <format>
namespace _fmt = std;
#else
#include <fmt/core.h>
namespace _fmt = fmt;
#endif
#include <string>

namespace tirex {
	extern tirexLogCallback logCallback;

	namespace log {
		/**
		 * @brief Formats and logs a new log message.
		 * @details The log message is formatted using the std::format / fmtlib syntax and is logged using the currently
		 * configured tirexLogCallback. You can use tirexSetLogCallback(tirexLogCallback) to override the logger.
		 *  
		 * @tparam level The level (tirexLogLevel) to log at.
		 * @param component A string which represents the component that should be logged for. This is argument is
		 * passed on to the internal log callback that was configured using tirexSetLogCallback(tirexLogCallback).
		 * @param fmt The std::format / fmtlib format string
		 * @param args The std::format / fmtlib format arguments.
		 */
		template <tirexLogLevel level, class... Args>
		void log(std::string component, _fmt::format_string<Args...> fmt, Args&&... args) {
			const auto msg = _fmt::format(fmt, std::forward<Args>(args)...);
			logCallback(level, component.c_str(), msg.c_str());
		}

		/**
		 * @brief Alias for log<tirexLogLevel::TRACE>
		 * 
		 * @param component A string which represents the component that should be logged for. This is argument is
		 * passed on to the internal log callback that was configured using tirexSetLogCallback(tirexLogCallback).
		 * @param fmt The std::format / fmtlib format string
		 * @param args The std::format / fmtlib format arguments.
		 */
		template <typename... Args>
		void trace(std::string component, _fmt::format_string<Args...> fmt, Args&&... args) {
			log<tirexLogLevel::TRACE>(component, fmt, std::forward<Args>(args)...);
		}

		/**
		 * @brief Alias for log<tirexLogLevel::DEBUG>
		 * 
		 * @param component A string which represents the component that should be logged for. This is argument is
		 * passed on to the internal log callback that was configured using tirexSetLogCallback(tirexLogCallback).
		 * @param fmt The std::format / fmtlib format string
		 * @param args The std::format / fmtlib format arguments.
		 */
		template <typename... Args>
		void debug(std::string component, _fmt::format_string<Args...> fmt, Args&&... args) {
			log<tirexLogLevel::DEBUG>(component, fmt, std::forward<Args>(args)...);
		}

		/**
		 * @brief Alias for log<tirexLogLevel::INFO>
		 * 
		 * @param component A string which represents the component that should be logged for. This is argument is
		 * passed on to the internal log callback that was configured using tirexSetLogCallback(tirexLogCallback).
		 * @param fmt The std::format / fmtlib format string
		 * @param args The std::format / fmtlib format arguments.
		 */
		template <typename... Args>
		void info(std::string component, _fmt::format_string<Args...> fmt, Args&&... args) {
			log<tirexLogLevel::INFO>(component, fmt, std::forward<Args>(args)...);
		}

		/**
		 * @brief Alias for log<tirexLogLevel::WARN>
		 * 
		 * @param component A string which represents the component that should be logged for. This is argument is
		 * passed on to the internal log callback that was configured using tirexSetLogCallback(tirexLogCallback).
		 * @param fmt The std::format / fmtlib format string
		 * @param args The std::format / fmtlib format arguments.
		 */
		template <typename... Args>
		void warn(std::string component, _fmt::format_string<Args...> fmt, Args&&... args) {
			log<tirexLogLevel::WARN>(component, fmt, std::forward<Args>(args)...);
		}

		/**
		 * @brief Alias for log<tirexLogLevel::ERROR>
		 * 
		 * @param component A string which represents the component that should be logged for. This is argument is
		 * passed on to the internal log callback that was configured using tirexSetLogCallback(tirexLogCallback).
		 * @param fmt The std::format / fmtlib format string
		 * @param args The std::format / fmtlib format arguments.
		 */
		template <typename... Args>
		void error(std::string component, _fmt::format_string<Args...> fmt, Args&&... args) {
			log<tirexLogLevel::ERROR>(component, fmt, std::forward<Args>(args)...);
		}

		/**
		 * @brief Alias for log<tirexLogLevel::CRITICAL>
		 * 
		 * @param component A string which represents the component that should be logged for. This is argument is
		 * passed on to the internal log callback that was configured using tirexSetLogCallback(tirexLogCallback).
		 * @param fmt The std::format / fmtlib format string
		 * @param args The std::format / fmtlib format arguments.
		 */
		template <typename... Args>
		void critical(std::string component, _fmt::format_string<Args...> fmt, Args&&... args) {
			log<tirexLogLevel::CRITICAL>(component, fmt, std::forward<Args>(args)...);
		}
	} // namespace log
} // namespace tirex

#endif