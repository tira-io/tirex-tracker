#ifndef TIREX_TESTS_TESTUTILS_CWDSCOPEGUARD_HPP
#define TIREX_TESTS_TESTUTILS_CWDSCOPEGUARD_HPP

#include <filesystem>
#include <iostream>

/**
 * @class CWDScopeGuard
 * @brief RAII class to temporarily change the current working directory.
 * @details On construction, this class stores the current working directory and changes it to the specified path. When
 * the object goes out of scope, the original working directory is automatically restored.
 */
class CWDScopeGuard {
private:
	const std::filesystem::path oldworkdir; /**< The original working directory. */

	CWDScopeGuard() = delete;
	CWDScopeGuard(const CWDScopeGuard& other) = delete;
	CWDScopeGuard(CWDScopeGuard&& other) = delete;
	CWDScopeGuard& operator=(const CWDScopeGuard& other) = delete;
	CWDScopeGuard& operator=(CWDScopeGuard&& other) = delete;

public:
	explicit CWDScopeGuard(const std::filesystem::path& cwd) : oldworkdir(std::filesystem::current_path()) {
		std::filesystem::current_path(cwd);
		std::cout << "Changed Working Dir to " << cwd << std::endl;
	}
	~CWDScopeGuard() {
		std::filesystem::current_path(oldworkdir);
		std::cout << "Changed Working Dir to " << oldworkdir << std::endl;
	}
};

#endif