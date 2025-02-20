/**
 * @file systemstats_macos.cpp
 * @brief Implements macos specific code of the systemstats.hpp header.
 */

#if defined(_WINDOWS)

#include "systemstats.hpp"

#include "../../logging.hpp"

using namespace std::string_literals;
using std::chrono::steady_clock;

using msr::Stats;
using msr::SystemStats;
/*
static std::string getOSDesc() { return _fmt::format("MacOS {}", getSysctl<std::string>("kern.osproductversion")); }

static std::string getKernelDesc() {
	return _fmt::format("{} {}", getSysctl<std::string>("kern.ostype"), getSysctl<std::string>("kern.osrelease"));
}*/

void SystemStats::start() {
	//msr::log::info("macosstats", "Collecting resources for Process {}", getpid());
	starttime = steady_clock::now();
	// msr::log::debug("macosstats", "Start systime {} ms, utime {} ms", startSysTime, startUTime);
}
void SystemStats::stop() { stoptime = steady_clock::now(); }
void SystemStats::step() {}

Stats SystemStats::getStats() {
	/** \todo: filter by requested metrics */
	auto wallclocktime =
			std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(stoptime - starttime).count());

	return {
			{MSR_TIME_ELAPSED_WALL_CLOCK_MS, wallclocktime},
			{MSR_TIME_ELAPSED_USER_MS, "TODO"s},
			{MSR_TIME_ELAPSED_SYSTEM_MS, "TODO"s},
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

	return {{MSR_OS_NAME, "TODO"s},
			{MSR_OS_KERNEL, "TODO"s},
			{MSR_CPU_AVAILABLE_SYSTEM_CORES, std::to_string(cpuInfo.numCores)},
			{MSR_CPU_FEATURES, cpuInfo.flags},
			{MSR_CPU_FREQUENCY_MIN_MHZ, std::to_string(cpuInfo.frequency_min)},
			{MSR_CPU_FREQUENCY_MAX_MHZ, std::to_string(cpuInfo.frequency_max)},
			{MSR_CPU_VENDOR_ID, cpuInfo.vendorId},
			{MSR_CPU_BYTE_ORDER, cpuInfo.endianness},
			{MSR_CPU_ARCHITECTURE, "TODO"s},
			{MSR_CPU_MODEL_NAME, cpuInfo.modelname},
			{MSR_CPU_CORES_PER_SOCKET, std::to_string(cpuInfo.coresPerSocket)},
			{MSR_CPU_THREADS_PER_CORE, std::to_string(cpuInfo.threadsPerCore)},
			{MSR_CPU_CACHES, caches},
			{MSR_CPU_VIRTUALIZATION,
			 (cpuInfo.virtualization.svm ? "AMD-V "s : ""s) + (cpuInfo.virtualization.vmx ? "VT-x"s : ""s)},
			{MSR_RAM_AVAILABLE_SYSTEM_MB, "TODO"s}};
}
#endif