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
int runCommand(std::string_view command) {
	STARTUPINFOW si{};
	PROCESS_INFORMATION pi{};
	si.cb = sizeof(si);

	// CreateProcess requires a modifiable buffer
	std::wstring cmd(command);

	if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
		std::cerr << "CreateProcess failed: " << GetLastError() << "\n";
		return -1;
	}

	// Wait for process to finish
	WaitForSingleObject(pi.hProcess, INFINITE);

	DWORD exitCode = 0;
	GetExitCodeProcess(pi.hProcess, &exitCode);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return static_cast<int>(exitCode);
}
#else
#error "Unsupported OS"
#endif