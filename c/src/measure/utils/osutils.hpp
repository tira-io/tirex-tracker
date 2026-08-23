#ifndef MEASURE_UTILS_OSUTILS_HPP
#define MEASURE_UTILS_OSUTILS_HPP

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

namespace tirex::utils {
	/**
	 * @brief Best-effort lookup of the current working directory of the process identified by \p pid.
	 * @details TIREX_PID_SELF resolves to the calling process's working directory. Returns std::nullopt if \p pid does
	 * not exist, cannot be inspected (e.g. insufficient permissions), or its working directory could otherwise not be
	 * determined.
     * @param pid the process for which to find the current working directory.
	 */
	std::optional<std::filesystem::path> getProcessWorkingDirectory(int64_t pid);

	/**
	 * @brief Discovers every process in the tree rooted at (and including) \p root , as of the moment this function is
	 * called.
	 * @details Best-effort and inherently racy: processes can be created or exit between this scan and the caller using
	 * the result, so callers must tolerate PIDs from a previous call having since disappeared, and freshly spawned
	 * descendants not showing up until the next call. TIREX_PID_SELF resolves to the calling process. If \p root itself
	 * cannot be found, returns a single-element vector containing just \p root (consistent with treating an
	 * untracked/unknown process as a "tree" of size one, rather than an empty tree).
	 * 
	 * Only thread-group leaders (i.e. what a user would call "a process") are included; individual threads of a
	 * multi-threaded process are intentionally excluded. On Linux, every thread of a process also gets its own
	 * top-level /proc/<tid> entry indistinguishable from a real process at a glance; including those would double-count
	 * that process's own CPU/RAM once via its (already-aggregated) process-level entry and again via each individual
	 * thread's entry. Windows/macOS process enumeration APIs do not expose threads this way, but the filter is applied
	 * uniformly for a single documented contract across platforms.
	 * @param root the root of the process tree to discover.
	 * @return every thread-group leader in the tree rooted at \p root , including \p root itself.
	 */
	std::vector<int64_t> discoverProcessTree(int64_t root);
} // namespace tirex::utils

#endif
