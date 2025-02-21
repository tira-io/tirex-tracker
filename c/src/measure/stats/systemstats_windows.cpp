/**
 * @file systemstats_macos.cpp
 * @brief Implements macos specific code of the systemstats.hpp header.
 */

#if defined(_WINDOWS)

#include "systemstats.hpp"

#include "../../logging.hpp"

#include <windows.h>
#include <versionhelpers.h>
#include <powrprof.h>

#include <tuple>

#pragma comment(lib, "Powrprof.lib")

using namespace std::string_literals;
using std::chrono::steady_clock;

using msr::Stats;
using msr::SystemStats;
static std::string getOSDesc() {
	if (IsWindows10OrGreater()) return "Windows 10+";
	if (IsWindows8Point1OrGreater()) return "Windows 8.10";
	if (IsWindows8OrGreater()) return "Windows 8";
	if (IsWindows7SP1OrGreater()) return "Windows 7 SP 1";
	if (IsWindows7OrGreater()) return "Windows 7";
	if (IsWindowsVistaSP2OrGreater()) return "Windows Vista SP 2";
	if (IsWindowsVistaSP1OrGreater()) return "Windows Vista SP 1";
	if (IsWindowsVistaOrGreater()) return "Windows Vista";
	if (IsWindowsXPSP3OrGreater()) return "Windows XP SP 3";
	if (IsWindowsXPSP2OrGreater()) return "Windows XP SP 2";
	if (IsWindowsXPSP1OrGreater()) return "Windows XP SP 1";
	if (IsWindowsXPOrGreater()) return "Windows XP";
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
		msr::log::error("windowsstats", "Could not fetch memory status");
        return 0;
    }
}

static void getProcessorFrequencies(std::vector<uint32_t>& freq) {
	struct PROCESSOR_POWER_INFORMATION {
		ULONG  Number;
		ULONG  MaxMhz;
		ULONG  CurrentMhz;
		ULONG  MhzLimit;
		ULONG  MaxIdleState;
		ULONG  CurrentIdleState;
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

static std::tuple<size_t, size_t> getSysAndUserTime() {
	FILETIME creationTime, exitTime, kernelTime, userTime;
    
    // Get the current process handle
    HANDLE hProcess = GetCurrentProcess();
    
    // Retrieve process times (user time, system time, etc.)
    if (GetProcessTimes(hProcess, &creationTime, &exitTime, &kernelTime, &userTime)) {
        ULARGE_INTEGER userTime64, systemTime64;
        userTime64.LowPart = userTime.dwLowDateTime;
        userTime64.HighPart = userTime.dwHighDateTime;
        systemTime64.LowPart = kernelTime.dwLowDateTime;
        systemTime64.HighPart = kernelTime.dwHighDateTime;
		return {systemTime64.QuadPart, userTime64.QuadPart};
    } else {
		msr::log::error("windowstats", "Failed to get process times");
		return {0,0};
    }
}

static size_t tickToMs(size_t tick) {
	// 1 tick = 100 ns = 10^-4 ms
	return tick / 10000;
}

void SystemStats::start() {
	starttime = steady_clock::now();
	std::tie(startSysTime, startUTime) = getSysAndUserTime();
	msr::log::debug("windowsstats", "Start systime {} ms, utime {} ms", tickToMs(startSysTime), tickToMs(startUTime));
}
void SystemStats::stop() {
	stoptime = steady_clock::now();
	std::tie(stopSysTime, stopUTime) = getSysAndUserTime();
}
void SystemStats::step() {
	thread_local static std::vector<uint32_t> cpuFreqs;
	getProcessorFrequencies(cpuFreqs);

	frequency.addValue(cpuFreqs[0]);
}

Stats SystemStats::getStats() {
	/** \todo: filter by requested metrics */
	auto wallclocktime =
			std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(stoptime - starttime).count());

	return {
			{MSR_TIME_ELAPSED_WALL_CLOCK_MS, wallclocktime},
			{MSR_TIME_ELAPSED_USER_MS, std::to_string(tickToMs(stopUTime - startUTime))},
			{MSR_TIME_ELAPSED_SYSTEM_MS, std::to_string(tickToMs(stopSysTime - startSysTime))},
			{MSR_CPU_USED_PROCESS_PERCENT, "TODO"s},
			{MSR_CPU_USED_SYSTEM_PERCENT, "TODO"s},
			{MSR_CPU_FREQUENCY_MHZ, frequency},
			{MSR_RAM_USED_PROCESS_KB, "TODO"s},
			{MSR_RAM_USED_SYSTEM_MB, "TODO"s}};
}


Stats SystemStats::getInfo() {
	/** \todo: filter by requested metrics */
	auto cpuInfo = getCPUInfo();

	std::string caches = "";
	size_t cacheIdx = 1;
	for (auto& [unified, instruct, data] : cpuInfo.caches) {
		if (unified)
			caches += _fmt::format("\"l{}\": \"{} KiB\",", cacheIdx, unified / 1024);
		if (instruct)
			caches += _fmt::format("\"l{}i\": \"{} KiB\",", cacheIdx, instruct / 1024);
		if (data)
			caches += _fmt::format("\"l{}d\": \"{} KiB\",", cacheIdx, data / 1024);
		++cacheIdx;
	}

	return {{MSR_OS_NAME, getOSDesc()},
			{MSR_OS_KERNEL, getKernelDesc()},
			{MSR_CPU_AVAILABLE_SYSTEM_CORES, std::to_string(cpuInfo.numCores)},
			{MSR_CPU_FEATURES, cpuInfo.flags},
			{MSR_CPU_FREQUENCY_MIN_MHZ, std::to_string(cpuInfo.frequency_min)},
			{MSR_CPU_FREQUENCY_MAX_MHZ, std::to_string(cpuInfo.frequency_max)},
			{MSR_CPU_VENDOR_ID, cpuInfo.vendorId},
			{MSR_CPU_BYTE_ORDER, cpuInfo.endianness},
			{MSR_CPU_ARCHITECTURE, getCPUArchitecture()},
			{MSR_CPU_MODEL_NAME, cpuInfo.modelname},
			{MSR_CPU_CORES_PER_SOCKET, std::to_string(cpuInfo.coresPerSocket)},
			{MSR_CPU_THREADS_PER_CORE, std::to_string(cpuInfo.threadsPerCore)},
			{MSR_CPU_CACHES, caches},
			{MSR_CPU_VIRTUALIZATION,
			 (cpuInfo.virtualization.svm ? "AMD-V "s : ""s) + (cpuInfo.virtualization.vmx ? "VT-x"s : ""s)},
			{MSR_RAM_AVAILABLE_SYSTEM_MB, std::to_string(getTotalRAM_MB())}};
}
#endif