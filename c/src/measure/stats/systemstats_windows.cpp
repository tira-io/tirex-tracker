/**
 * @file systemstats_macos.cpp
 * @brief Implements macos specific code of the systemstats.hpp header.
 */

#if defined(_WINDOWS) || defined(_WIN32) || defined(WIN32)

#include "systemstats.hpp"

#include "../../logging.hpp"

#include <powrprof.h>
#include <psapi.h>
#include <versionhelpers.h>
#include <windows.h>

#include <tuple>

using namespace std::string_literals;
using std::chrono::steady_clock;

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

	/** \todo keep this buffer as a member fo avoid reallocation **/
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

uint8_t SystemStats::getCPUUtilization() {
	FILETIME sysIdle, sysKernel, sysUser;
	if (GetSystemTimes(&sysIdle, &sysKernel, &sysUser) == 0) {
		tirex::log::error("windowsstats", "Failed to get system times");
		return 0;
	}
	uint8_t util = 1;
	if (prevSysIdle.dwLowDateTime != 0 && prevSysIdle.dwHighDateTime != 0) {
		auto sysIdleDiff = fileTimeToUint64(sysIdle) - fileTimeToUint64(prevSysIdle);
		auto sysKernelDiff = fileTimeToUint64(sysKernel) - fileTimeToUint64(prevSysKernel);
		auto sysUserDiff = fileTimeToUint64(sysUser) - fileTimeToUint64(prevSysUser);
		auto sysTotal = sysKernelDiff + sysUserDiff;
		auto kernelTotal =
				sysKernelDiff - sysIdleDiff; // kernelTime - IdleTime = kernelTime, because sysKernel include IdleTime

		if (sysTotal > 0) // sometimes kernelTime > idleTime
			util = static_cast<uint8_t>(((kernelTotal + sysUserDiff) * 100) / sysTotal);
	}

	prevSysIdle = sysIdle;
	prevSysKernel = sysKernel;
	prevSysUser = sysUser;

	return util;
}

uint8_t SystemStats::getProcCPUUtilization() {
	FILETIME ftime, fsys, fuser;
	ULARGE_INTEGER now, sys, user;
	size_t percent;

	GetSystemTimeAsFileTime(&ftime);
	memcpy(&now, &ftime, sizeof(FILETIME));

	GetProcessTimes(pid, &ftime, &ftime, &fsys, &fuser);
	memcpy(&sys, &fsys, sizeof(FILETIME));
	memcpy(&user, &fuser, sizeof(FILETIME));
	percent = (sys.QuadPart - lastSysCPU.QuadPart) + (user.QuadPart - lastUserCPU.QuadPart);
	percent /= (now.QuadPart - lastCPU.QuadPart);
	percent *= 100;
	percent /= numProcessors;
	lastCPU = now;
	lastUserCPU = user;
	lastSysCPU = sys;
	return static_cast<uint8_t>(percent);
}

SystemStats::Utilization SystemStats::getUtilization() {
	return Utilization{
			.ramUsedKB = getRAMUsageKB(pid),
			.cpuUtilization = getProcCPUUtilization(),
			.system = {.ramUsedMB = getSystemRAMUsageMB(), .cpuUtilization = getCPUUtilization()}
	};
}

SystemStats::SysInfo SystemStats::getSysInfo() {
	return {.osname = getOSDesc(),
			.kerneldesc = getKernelDesc(),
			.architecture = getCPUArchitecture(),
			.totalRamMB = getTotalRAM_MB()};
}

void SystemStats::start() {
	starttime = steady_clock::now();
	std::tie(startSysTime, startUTime) = getSysAndUserTime();
	tirex::log::debug("windowsstats", "Start systime {} ms, utime {} ms", tickToMs(startSysTime), tickToMs(startUTime));
	getUtilization(); // Call getUtilization once to init CPU Utilization tracking
	//
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	numProcessors = sysInfo.dwNumberOfProcessors;
}
void SystemStats::stop() {
	stoptime = steady_clock::now();
	std::tie(stopSysTime, stopUTime) = getSysAndUserTime();
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