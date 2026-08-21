#include "osutils.hpp"

#include "../../logging.hpp"

#include <tirex_tracker.h>

#if defined(__linux__)
#include <system_error>
#elif defined(__APPLE__)
#include <climits>
#include <libproc.h>
#elif defined(_WINDOWS) || defined(_WIN32) || defined(WIN32)
#include "../stats/details/windows/ntdll.hpp"
#else
#error "Unsupported OS"
#endif

namespace fs = std::filesystem;

#if defined(__linux__)
std::optional<fs::path> tirex::utils::getProcessWorkingDirectory(int64_t pid) {
	if (pid == TIREX_PID_SELF)
		return fs::current_path();
	std::error_code ec;
	auto cwd = fs::read_symlink(fs::path("/proc") / std::to_string(pid) / "cwd", ec);
	if (ec) {
		tirex::log::error("osutils", "Failed to resolve the working directory of process {}: {}", pid, ec.message());
		return std::nullopt;
	}
	return cwd;
}
#elif defined(__APPLE__)
std::optional<fs::path> tirex::utils::getProcessWorkingDirectory(int64_t pid) {
	if (pid == TIREX_PID_SELF)
		return fs::current_path();
	struct proc_vnodepathinfo vpi;
	if (proc_pidinfo(static_cast<pid_t>(pid), PROC_PIDVNODEPATHINFO, 0, &vpi, sizeof(vpi)) <= 0) {
		tirex::log::error("osutils", "Failed to resolve the working directory of process {}", pid);
		return std::nullopt;
	}
	return fs::path(vpi.pvi_cdir.vip_path);
}
#elif defined(_WINDOWS) || defined(_WIN32) || defined(WIN32)
namespace {
	// Undocumented, but stable since Windows XP for both x86 and x64.
	struct CURDIR {
		UNICODE_STRING DosPath;
		HANDLE Handle;
	};

	// Only the prefix of RTL_USER_PROCESS_PARAMETERS that is needed to reach CurrentDirectory.
	struct RTL_USER_PROCESS_PARAMETERS_PREFIX {
		ULONG MaximumLength;
		ULONG Length;
		ULONG Flags;
		ULONG DebugFlags;
		HANDLE ConsoleHandle;
		ULONG ConsoleFlags;
		HANDLE StandardInput;
		HANDLE StandardOutput;
		HANDLE StandardError;
		CURDIR CurrentDirectory;
	};
} // namespace

std::optional<fs::path> tirex::utils::getProcessWorkingDirectory(int64_t pid) {
	if (pid == TIREX_PID_SELF)
		return fs::current_path();

	HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, static_cast<DWORD>(pid));
	if (process == nullptr) {
		tirex::log::error("osutils", "Failed to open process {} to resolve its working directory", pid);
		return std::nullopt;
	}

	auto queryInformationProcess = ntdll().queryInformationProcess;
	if (queryInformationProcess == nullptr) {
		CloseHandle(process);
		tirex::log::error("osutils", "Failed to locate NtQueryInformationProcess");
		return std::nullopt;
	}

	PROCESS_BASIC_INFORMATION pbi;
	ULONG returnLength;
	if (!NT_SUCCESS(queryInformationProcess(
				process, static_cast<PROCESSINFOCLASS>(0) /* ProcessBasicInformation */, &pbi, sizeof(pbi), &returnLength
		))) {
		CloseHandle(process);
		tirex::log::error("osutils", "Failed to query basic information of process {}", pid);
		return std::nullopt;
	}

	PEB peb;
	RTL_USER_PROCESS_PARAMETERS_PREFIX params;
	std::optional<fs::path> result;
	if (ReadProcessMemory(process, pbi.PebBaseAddress, &peb, sizeof(peb), nullptr) &&
		ReadProcessMemory(process, peb.ProcessParameters, &params, sizeof(params), nullptr)) {
		auto& dosPath = params.CurrentDirectory.DosPath;
		std::wstring buffer(dosPath.Length / sizeof(WCHAR), L'\0');
		if (dosPath.Length == 0 ||
			ReadProcessMemory(process, dosPath.Buffer, buffer.data(), dosPath.Length, nullptr)) {
			result = fs::path(buffer);
		}
	}
	if (!result)
		tirex::log::error("osutils", "Failed to read the working directory of process {}", pid);
	CloseHandle(process);
	return result;
}
#endif
