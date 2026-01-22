#include "utils.hpp"

#if defined(__linux__) || defined(__APPLE__)
#include <sys/wait.h>

int runCommand(std::string_view command) {
	int status = std::system(command.data());
	if (status == -1 || !WIFEXITED(status))
		return status;
	return WEXITSTATUS(status);
}
#elif defined(_WINDOWS) || defined(_WIN32) || defined(WIN32)
#include <windows.h>

int runCommand(std::string_view command) {
	STARTUPINFO si{};
	PROCESS_INFORMATION pi{};
	si.cb = sizeof(si);

	std::string cmd(command.begin(), command.end());
	if (!CreateProcess(nullptr, cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
		return static_cast<int>(GetLastError());
	}

	// Wait for process to finish
	WaitForSingleObject(pi.hProcess, INFINITE);

	DWORD exitCode = 0;
	if (!GetExitCodeProcess(pi.hProcess, &exitCode)) {
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return static_cast<int>(GetLastError());
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return static_cast<int>(exitCode);
}
#else
#error "Unsupported OS"
#endif