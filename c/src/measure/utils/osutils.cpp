#include "osutils.hpp"

#include "../../logging.hpp"

#include <tirex_tracker.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <queue>
#include <unordered_map>
#include <unordered_set>

#if defined(__linux__)
#include <system_error>
#include <unistd.h>
#elif defined(__APPLE__)
#include <climits>
#include <libproc.h>
#include <unistd.h>
#elif defined(_WINDOWS) || defined(_WIN32) || defined(WIN32)
#include "../stats/details/windows/ntdll.hpp"

#include <tlhelp32.h>
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
				process, static_cast<PROCESSINFOCLASS>(0) /* ProcessBasicInformation */, &pbi, sizeof(pbi),
				&returnLength
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
		if (dosPath.Length == 0 || ReadProcessMemory(process, dosPath.Buffer, buffer.data(), dosPath.Length, nullptr)) {
			result = fs::path(buffer);
		}
	}
	if (!result)
		tirex::log::error("osutils", "Failed to read the working directory of process {}", pid);
	CloseHandle(process);
	return result;
}
#endif

namespace {
	/** @brief BFS from `root` over a fully-built parent -> children adjacency map, always including \p root itself
	 * (even if it has no entry in \p childrenOf , e.g. because it could not be found by the caller's platform-specific
	 * scan). Shared by every platform's discoverProcessTree() once it has built the map its own way. */
	std::vector<int64_t>
	collectTree(int64_t root, const std::unordered_map<int64_t, std::vector<int64_t>>& childrenOf) {
		std::vector<int64_t> tree;
		std::unordered_set<int64_t> seen{root};
		std::queue<int64_t> pending;
		pending.push(root);
		while (!pending.empty()) {
			int64_t current = pending.front();
			pending.pop();
			tree.push_back(current);
			auto it = childrenOf.find(current);
			if (it == childrenOf.end())
				continue;
			for (int64_t child : it->second) {
				if (seen.insert(child).second)
					pending.push(child);
			}
		}
		return tree;
	}
} // namespace

#if defined(__linux__)
namespace {
	/** @brief Reads the Tgid and PPid of the task identified by \p pid from /proc/<pid>/status. Returns std::nullopt
	 * if \p pid doesn't exist, can't be read, or is not a thread-group leader (Tgid != pid); see discoverProcessTree()
	 * for why non-leaders are excluded. */
	std::optional<int64_t> parentIfThreadGroupLeader(int64_t pid) {
		std::ifstream is(fs::path("/proc") / std::to_string(pid) / "status");
		if (!is)
			return std::nullopt;
		std::optional<int64_t> tgid, ppid;
		for (std::string line; std::getline(is, line);) {
			if (line.starts_with("Tgid:"))
				tgid = std::stoll(line.substr(5));
			else if (line.starts_with("PPid:"))
				ppid = std::stoll(line.substr(5));
		}
		if (!tgid || !ppid || *tgid != pid)
			return std::nullopt;
		return ppid;
	}
} // namespace

std::vector<int64_t> tirex::utils::discoverProcessTree(int64_t root) {
	if (root == TIREX_PID_SELF)
		root = getpid();

	std::unordered_map<int64_t, std::vector<int64_t>> childrenOf;
	std::error_code ec;
	for (const auto& entry : fs::directory_iterator("/proc", ec)) {
		if (ec || !entry.is_directory())
			continue;
		const auto& name = entry.path().filename().string();
		if (name.empty() || !std::all_of(name.begin(), name.end(), [](unsigned char c) { return std::isdigit(c); }))
			continue; // Not a PID directory (e.g. /proc/cpuinfo, /proc/self).
		if (auto ppid = parentIfThreadGroupLeader(std::stoll(name)))
			childrenOf[*ppid].push_back(std::stoll(name));
	}
	return collectTree(root, childrenOf);
}
#elif defined(__APPLE__)
namespace {
	/** @brief Best-effort direct-children lookup via the undocumented-but-widely-used proc_listchildpids. Unlike
	 * Linux/Windows, macOS has no single "list every process" call that also reports each one's parent, so the tree
	 * is built by asking, for every already-discovered member, "what are your children" one call at a time (BFS),
	 * rather than by building one global parent -> children map up front. */
	std::vector<int64_t> getChildPids(int64_t parent) {
		int n = proc_listchildpids(static_cast<pid_t>(parent), nullptr, 0);
		if (n <= 0)
			return {};
		// A little slack in case children appear between the sizing call and the fill call below.
		std::vector<pid_t> buffer(static_cast<size_t>(n) + 16);
		int filled = proc_listchildpids(
				static_cast<pid_t>(parent), buffer.data(), static_cast<int>(buffer.size() * sizeof(pid_t))
		);
		if (filled <= 0)
			return {};
		size_t count = std::min(static_cast<size_t>(filled), buffer.size());
		return std::vector<int64_t>(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(count));
	}
} // namespace

std::vector<int64_t> tirex::utils::discoverProcessTree(int64_t root) {
	if (root == TIREX_PID_SELF)
		root = getpid();

	std::vector<int64_t> tree;
	std::unordered_set<int64_t> seen{root};
	std::queue<int64_t> pending;
	pending.push(root);
	while (!pending.empty()) {
		int64_t current = pending.front();
		pending.pop();
		tree.push_back(current);
		for (int64_t child : getChildPids(current)) {
			if (seen.insert(child).second)
				pending.push(child);
		}
	}
	return tree;
}
#elif defined(_WINDOWS) || defined(_WIN32) || defined(WIN32)
std::vector<int64_t> tirex::utils::discoverProcessTree(int64_t root) {
	if (root == TIREX_PID_SELF)
		root = static_cast<int64_t>(GetCurrentProcessId());

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE) {
		tirex::log::error("osutils", "Failed to snapshot the system's process list");
		return {root};
	}

	// Windows PIDs are reused once a process exits, and th32ParentProcessID is not cleared or validated when the
	// original parent exits -- so a long-lived process whose original parent exited and whose PID was later reused
	// by an unrelated process could, in principle, be mistaken for that unrelated process's child. This is an
	// inherent limitation of parent-PID-based tracking on Windows, not something this scan can detect or avoid.
	std::unordered_map<int64_t, std::vector<int64_t>> childrenOf;
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(entry);
	if (Process32First(snapshot, &entry)) {
		do {
			childrenOf[static_cast<int64_t>(entry.th32ParentProcessID)].push_back(
					static_cast<int64_t>(entry.th32ProcessID)
			);
		} while (Process32Next(snapshot, &entry));
	}
	CloseHandle(snapshot);

	return collectTree(root, childrenOf);
}
#endif
