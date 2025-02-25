#ifndef LOGGING_HPP
#define LOGGING_HPP

#include <tirex_tracker.h>

#if __cpp_lib_format
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

		template <tirexLogLevel level, class... Args>
		void log(std::string component, _fmt::format_string<Args...> fmt, Args&&... args) {
			const auto msg = _fmt::format(fmt, std::forward<Args>(args)...);
			logCallback(level, component.c_str(), msg.c_str());
		}

		template <typename... Args>
		void trace(std::string component, _fmt::format_string<Args...> fmt, Args&&... args) {
			log<tirexLogLevel::TRACE>(component, fmt, std::forward<Args>(args)...);
		}
		template <typename... Args>
		void debug(std::string component, _fmt::format_string<Args...> fmt, Args&&... args) {
			log<tirexLogLevel::DEBUG>(component, fmt, std::forward<Args>(args)...);
		}
		template <typename... Args>
		void info(std::string component, _fmt::format_string<Args...> fmt, Args&&... args) {
			log<tirexLogLevel::INFO>(component, fmt, std::forward<Args>(args)...);
		}
		template <typename... Args>
		void warn(std::string component, _fmt::format_string<Args...> fmt, Args&&... args) {
			log<tirexLogLevel::WARN>(component, fmt, std::forward<Args>(args)...);
		}
		template <typename... Args>
		void error(std::string component, _fmt::format_string<Args...> fmt, Args&&... args) {
			log<tirexLogLevel::ERROR>(component, fmt, std::forward<Args>(args)...);
		}
		template <typename... Args>
		void critical(std::string component, _fmt::format_string<Args...> fmt, Args&&... args) {
			log<tirexLogLevel::CRITICAL>(component, fmt, std::forward<Args>(args)...);
		}
	} // namespace log
} // namespace tirex

#endif