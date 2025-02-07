#ifndef LOGGING_HPP
#define LOGGING_HPP

#include <measure.h>

#include <format>
#include <string>

namespace msr {
	extern msrLogCallback logCallback;

	namespace log {

		template <msrLogLevel level, class... Args>
		void log(std::string component, std::format_string<Args...> fmt, Args&&... args) {
			const auto msg = std::format(fmt, std::forward<Args>(args)...);
			logCallback(level, component.c_str(), msg.c_str());
		}

		template <typename... Args>
		void trace(std::string component, std::format_string<Args...> fmt, Args&&... args) {
			log<msrLogLevel::TRACE>(component, fmt, std::forward<Args>(args)...);
		}
		template <typename... Args>
		void debug(std::string component, std::format_string<Args...> fmt, Args&&... args) {
			log<msrLogLevel::DEBUG>(component, fmt, std::forward<Args>(args)...);
		}
		template <typename... Args>
		void info(std::string component, std::format_string<Args...> fmt, Args&&... args) {
			log<msrLogLevel::INFO>(component, fmt, std::forward<Args>(args)...);
		}
		template <typename... Args>
		void warn(std::string component, std::format_string<Args...> fmt, Args&&... args) {
			log<msrLogLevel::WARN>(component, fmt, std::forward<Args>(args)...);
		}
		template <typename... Args>
		void error(std::string component, std::format_string<Args...> fmt, Args&&... args) {
			log<msrLogLevel::ERROR>(component, fmt, std::forward<Args>(args)...);
		}
		template <typename... Args>
		void critical(std::string component, std::format_string<Args...> fmt, Args&&... args) {
			log<msrLogLevel::CRITICAL>(component, fmt, std::forward<Args>(args)...);
		}
	} // namespace log
} // namespace msr

#endif