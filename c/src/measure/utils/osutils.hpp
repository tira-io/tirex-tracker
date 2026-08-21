#ifndef MEASURE_UTILS_OSUTILS_HPP
#define MEASURE_UTILS_OSUTILS_HPP

#include <cstdint>
#include <filesystem>
#include <optional>

namespace tirex::utils {
	/**
	 * @brief Best-effort lookup of the current working directory of the process identified by \p pid.
	 * @details TIREX_PID_SELF resolves to the calling process's working directory. Returns std::nullopt if \p pid does
	 * not exist, cannot be inspected (e.g. insufficient permissions), or its working directory could otherwise not be
	 * determined.
     * @param pid the process for which to find the current working directory.
	 */
	std::optional<std::filesystem::path> getProcessWorkingDirectory(int64_t pid);
} // namespace tirex::utils

#endif
