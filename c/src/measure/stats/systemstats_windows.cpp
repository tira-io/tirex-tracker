/**
 * @file systemstats_macos.cpp
 * @brief Implements macos specific code of the systemstats.hpp header.
 */

#if defined(_WINDOWS) || defined(_WIN32) || defined(WIN32)

#include "systemstats.hpp"

#include "../../logging.hpp"
#include "../utils/osutils.hpp"
#include "details/windows/ntdll.hpp"

#include <powrprof.h>
#include <psapi.h>
#include <versionhelpers.h>

#include <cstring>
#include <optional>
#include <tuple>
#include <unordered_map>

using namespace std::string_literals;
using std::chrono::steady_clock;
using std::chrono::system_clock;

using tirex::Stats;
using tirex::SystemStats;

static std::string getOSDesc() {
	if (IsWindows10OrGreater())
		return "Windows 10+";
	if (IsWindows8Point1OrGreater())
		return "Windows 8.10";
	if (IsWindows8OrGreater())
		return "Windows 8";
	if (IsWindows7SP1OrGreater())
		return "Windows 7 SP 1";
	if (IsWindows7OrGreater())
		return "Windows 7";
	if (IsWindowsVistaSP2OrGreater())
		return "Windows Vista SP 2";
	if (IsWindowsVistaSP1OrGreater())
		return "Windows Vista SP 1";
	if (IsWindowsVistaOrGreater())
		return "Windows Vista";
	if (IsWindowsXPSP3OrGreater())
		return "Windows XP SP 3";
	if (IsWindowsXPSP2OrGreater())
		return "Windows XP SP 2";
	if (IsWindowsXPSP1OrGreater())
		return "Windows XP SP 1";
	if (IsWindowsXPOrGreater())
		return "Windows XP";
	return "Unknown Windows Version";
}

static std::string getKernelDesc() {
	OSVERSIONINFOEX osvi;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	if (GetVersionEx((OSVERSIONINFO*)&osvi))
		return _fmt::format("Windows Kernel v.{}.{}.{}", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
	return "Unable to retrieve version.";
}

static std::string getCPUArchitecture() {
	SYSTEM_INFO sysInfo;
	GetNativeSystemInfo(&sysInfo);
	switch (sysInfo.wProcessorArchitecture) {
	case PROCESSOR_ARCHITECTURE_AMD64:
		return "x64";
	case PROCESSOR_ARCHITECTURE_ARM:
		return "ARM";
	case PROCESSOR_ARCHITECTURE_ARM64:
		return "ARM64";
	case PROCESSOR_ARCHITECTURE_INTEL:
		return "x86";
	case PROCESSOR_ARCHITECTURE_UNKNOWN:
	default:
		return "Unknown Architecture";
	}
}

static uint32_t getTotalRAM_MB() {
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(MEMORYSTATUSEX);

	if (GlobalMemoryStatusEx(&statex)) {
		return static_cast<uint32_t>(statex.ullTotalPhys / 1000 / 1000);
	} else {
		tirex::log::error("windowsstats", "Could not fetch memory status");
		return 0;
	}
}

static void getProcessorFrequencies(std::vector<uint32_t>& freq) {
	struct PROCESSOR_POWER_INFORMATION {
		ULONG Number;
		ULONG MaxMhz;
		ULONG CurrentMhz;
		ULONG MhzLimit;
		ULONG MaxIdleState;
		ULONG CurrentIdleState;
	};
	SYSTEM_INFO si = {0};
	GetSystemInfo(&si);

	std::vector<PROCESSOR_POWER_INFORMATION> data(si.dwNumberOfProcessors);
	DWORD dwSize = sizeof(PROCESSOR_POWER_INFORMATION) * si.dwNumberOfProcessors;
	CallNtPowerInformation(ProcessorInformation, NULL, 0, &data[0], dwSize);

	freq.resize(si.dwNumberOfProcessors);
	for (size_t i = 0; i < data.size(); ++i)
		freq[i] = data[i].CurrentMhz;
}

static uint64_t fileTimeToUint64(const FILETIME& ft) {
	return (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | static_cast<uint64_t>(ft.dwLowDateTime);
}

std::tuple<size_t, size_t> SystemStats::getSysAndUserTime() const {
	FILETIME creationTime, exitTime, kernelTime, userTime;
	if (GetProcessTimes(pid, &creationTime, &exitTime, &kernelTime, &userTime)) {
		return {fileTimeToUint64(kernelTime), fileTimeToUint64(userTime)};
	} else {
		tirex::log::error("windowstats", "Failed to get process times");
		return {0, 0};
	}
}

size_t SystemStats::tickToMs(size_t tick) {
	// 1 tick = 100 ns = 10^-4 ms
	return tick / 10000;
}

static unsigned getRAMUsageKB(HANDLE pid) {
	PROCESS_MEMORY_COUNTERS pmc;
	if (GetProcessMemoryInfo(pid, &pmc, sizeof(pmc))) {
		return pmc.WorkingSetSize / 1000;
	} else {
		tirex::log::error("windowsstats", "Failed to get process memory info");
		return 0;
	}
}

namespace {
	/**
	 * @brief Reads a process's private (non-shared) memory usage in bytes, via a fresh, short-lived handle and
	 * GetProcessMemoryInfo's PROCESS_MEMORY_COUNTERS_EX::PrivateUsage. Unlike Linux's PSS (which fair-shares each
	 * shared page's cost across every process mapping it), PrivateUsage *excludes* shared pages entirely from every
	 * process's count; so summing it across a tree can never double-count shared memory, but it also *undercounts*  the
	 * true unique footprint by whatever memory the tree's own members share with *each other* (there is no Windows
	 * equivalent of PSS to give partial credit for that). It also tracks committed private virtual memory (closer to
	 * Task Manager's "Commit Size"), not strictly currently-resident physical memory the way the single-process
	 * WorkingSetSize-based getRAMUsageKB() above does; the two are not directly comparable figures. This is the best
	 * available approximation without restructuring around Job Objects.
	 * 
	 * @returns std::nullopt if \p pid doesn't exist or can't be opened/inspected.
	 */
	std::optional<std::uint64_t> readProcessPrivateUsage(int64_t pid) {
		HANDLE handle =
				OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, static_cast<DWORD>(pid));
		if (handle == nullptr)
			return std::nullopt;
		PROCESS_MEMORY_COUNTERS_EX pmc;
		bool ok = GetProcessMemoryInfo(handle, reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc));
		CloseHandle(handle);
		if (!ok)
			return std::nullopt;
		return static_cast<std::uint64_t>(pmc.PrivateUsage);
	}
} // namespace

static unsigned getSystemRAMUsageMB() {
	MEMORYSTATUSEX stat;
	stat.dwLength = sizeof(MEMORYSTATUSEX);
	if (GlobalMemoryStatusEx(&stat)) {
		SIZE_T totalMemory = stat.ullTotalPhys;
		SIZE_T availableMemory = stat.ullAvailPhys;
		return (totalMemory - availableMemory) / 1000 / 1000;
	} else {
		tirex::log::error("windowsstats", "Failed to get system memory info");
		return 0;
	}
}

unsigned SystemStats::getCPUUtilization() {
	FILETIME sysIdle, sysKernel, sysUser;
	if (GetSystemTimes(&sysIdle, &sysKernel, &sysUser) == 0) {
		tirex::log::error("windowsstats", "Failed to get system times");
		return 0;
	}
	unsigned util = 1;
	if (prevSysIdle.dwLowDateTime != 0 && prevSysIdle.dwHighDateTime != 0) {
		auto sysIdleDiff = fileTimeToUint64(sysIdle) - fileTimeToUint64(prevSysIdle);
		auto sysKernelDiff = fileTimeToUint64(sysKernel) - fileTimeToUint64(prevSysKernel);
		auto sysUserDiff = fileTimeToUint64(sysUser) - fileTimeToUint64(prevSysUser);
		auto sysTotal = sysKernelDiff + sysUserDiff;
		auto kernelTotal =
				sysKernelDiff - sysIdleDiff; // kernelTime - IdleTime = kernelTime, because sysKernel include IdleTime

		if (sysTotal > 0) // sometimes kernelTime > idleTime
			util = static_cast<unsigned>(((kernelTotal + sysUserDiff) * 100) / sysTotal);
	}

	prevSysIdle = sysIdle;
	prevSysKernel = sysKernel;
	prevSysUser = sysUser;

	return util;
}

unsigned SystemStats::getProcCPUUtilization() {
	FILETIME ftime, fsys, fuser;
	ULARGE_INTEGER now, sys, user;
	size_t percent = 0;

	GetSystemTimeAsFileTime(&ftime);
	std::memcpy(&now, &ftime, sizeof(FILETIME));

	GetProcessTimes(pid, &ftime, &ftime, &fsys, &fuser);
	std::memcpy(&sys, &fsys, sizeof(FILETIME));
	std::memcpy(&user, &fuser, sizeof(FILETIME));
	auto elapsed = now.QuadPart - lastCPU.QuadPart;
	if (elapsed != 0) {
		percent = ((sys.QuadPart - lastSysCPU.QuadPart) + (user.QuadPart - lastUserCPU.QuadPart)) * 100;
		percent /= elapsed;
	} else {
		tirex::log::warn("windowsstats", "Called too quickly apart (0 ticks elapsed)");
	}
	lastCPU = now;
	lastUserCPU = user;
	lastSysCPU = sys;
	return static_cast<unsigned>(percent);
}

namespace {
	/**
	 * @brief Reads a process's cumulative (kernel, user) CPU time via GetProcessTimes, opening (and closing) a fresh,
	 * short-lived handle for \p pid ; unlike the root process's own long-lived handle stored in SystemStats::pid, tree
	 * members are only ever touched transiently, once per poll.
	 * 
	 * @returns std::nullopt if \p pid doesn't exist or can't be opened/inspected (e.g. a privileged system process).
	 */
	std::optional<std::tuple<ULONGLONG, ULONGLONG>> readProcessCpuTicks(int64_t pid) {
		HANDLE handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(pid));
		if (handle == nullptr)
			return std::nullopt;
		FILETIME creationTime, exitTime, kernelTime, userTime;
		bool ok = GetProcessTimes(handle, &creationTime, &exitTime, &kernelTime, &userTime);
		CloseHandle(handle);
		if (!ok)
			return std::nullopt;
		return std::make_tuple(fileTimeToUint64(kernelTime), fileTimeToUint64(userTime));
	}
} // namespace

/**
 * @brief Like getProcCPUUtilization(), but sums the CPU usage of `pid` and every descendant process discovered via
 * tirex::utils::discoverProcessTree() at the time of the call. See the Linux implementation of this function for the
 * full rationale; this mirrors it exactly (including *not* dividing by the number of processors, matching
 * getProcCPUUtilization()'s own normalization above), just backed by GetProcessTimes on a per-member handle instead
 * of /proc/<pid>/stat.
 */
unsigned SystemStats::getTreeCPUUtilization() {
	FILETIME ftime;
	ULARGE_INTEGER now;
	GetSystemTimeAsFileTime(&ftime);
	std::memcpy(&now, &ftime, sizeof(FILETIME));
	auto elapsed = now.QuadPart - lastCPU.QuadPart;
	if (elapsed == 0) {
		tirex::log::warn("windowsstats", "Called too quickly apart (0 ticks elapsed)");
		return 0;
	}

	size_t totalPercent = 0;
	std::unordered_map<int64_t, ULONGLONG> newState;
	for (int64_t member : tirex::utils::discoverProcessTree(static_cast<int64_t>(GetProcessId(pid)))) {
		auto ticks = readProcessCpuTicks(member);
		if (!ticks)
			continue; // Vanished mid-scan, or couldn't be opened (e.g. a privileged process); excluded this tick.
		ULONGLONG activeTicks = std::get<0>(*ticks) + std::get<1>(*ticks);
		if (auto it = treeLastActiveCpu.find(member); it != treeLastActiveCpu.end() && activeTicks >= it->second) {
			totalPercent += static_cast<size_t>((activeTicks - it->second) * 100) / static_cast<size_t>(elapsed);
		}
		// Else: first sighting of this PID (or its CPU time went backwards, e.g. PID reuse) -- contributes nothing
		// this tick, matching the single-process "not enough history yet" behavior above.
		newState[member] = activeTicks;
	}
	treeLastActiveCpu = std::move(newState); // Prune state for PIDs no longer in the tree.
	lastCPU = now;
	return static_cast<unsigned>(totalPercent);
}

unsigned SystemStats::getTreeRAMUsageKB() {
	std::uint64_t totalBytes = 0;
	for (int64_t member : tirex::utils::discoverProcessTree(static_cast<int64_t>(GetProcessId(pid)))) {
		if (auto priv = readProcessPrivateUsage(member))
			totalBytes += *priv;
		// Else: vanished mid-scan, or couldn't be opened (e.g. a privileged process); excluded from this tick.
	}
	return static_cast<unsigned>(totalBytes / 1000);
}

SystemStats::Utilization SystemStats::getUtilization() {
	return Utilization{
			.ramUsedKB = trackSubprocesses ? getTreeRAMUsageKB() : getRAMUsageKB(pid),
			.cpuUtilization = trackSubprocesses ? getTreeCPUUtilization() : getProcCPUUtilization(),
			.system = {.ramUsedMB = getSystemRAMUsageMB(), .cpuUtilization = getCPUUtilization()}
	};
}

SystemStats::SysInfo SystemStats::getSysInfo() {
	return {.osname = getOSDesc(),
			.kerneldesc = getKernelDesc(),
			.architecture = getCPUArchitecture(),
			.totalRamMB = getTotalRAM_MB()};
}

std::vector<std::string> SystemStats::getInvocationCmd() {
	ULONG bufsize;
	char buffer[4096];
	HANDLE handle = GetCurrentProcess();
	NTSTATUS status = ntdll().queryInformationProcess(
			handle, static_cast<PROCESSINFOCLASS>(60) /* ProcessCommandLineInformation */, buffer, sizeof(buffer),
			&bufsize
	);
	if (!NT_SUCCESS(status))
		return {};
	auto ustr = reinterpret_cast<PUNICODE_STRING>(&buffer[0]);
	/** \todo When everything is unicode, this lossy conversion is not necessary **/
	return {std::string(ustr->Buffer, ustr->Buffer + ustr->Length)};
}

void SystemStats::start() {
	for (auto tmp : getInvocationCmd())
		tirex::log::info("windowsstats", "{}", tmp);
	starttimer = steady_clock::now();
	startTimepoint = system_clock::now();
	std::tie(startSysTime, startUTime) = getSysAndUserTime();
	tirex::log::debug("windowsstats", "Start systime {} ms, utime {} ms", tickToMs(startSysTime), tickToMs(startUTime));
	getUtilization(); // Call getUtilization once to init CPU Utilization tracking
}
void SystemStats::step() {
	thread_local static std::vector<uint32_t> cpuFreqs;
	getProcessorFrequencies(cpuFreqs);

	auto utilization = getUtilization();
	ram.addValue(utilization.ramUsedKB);
	sysRam.addValue(utilization.system.ramUsedMB);
	cpuUtil.addValue(utilization.cpuUtilization);
	sysCpuUtil.addValue(utilization.system.cpuUtilization);
	frequency.addValue(cpuFreqs[0]);
}
#endif