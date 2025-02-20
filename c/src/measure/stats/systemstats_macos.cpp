/**
 * @file systemstats_macos.cpp
 * @brief Implements macos specific code of the systemstats.hpp header.
 */

#if defined(__APPLE__)

#include "systemstats.hpp"

#include "../../logging.hpp"
#include "./macos/ioreport.h"
#include "./macos/sysctl.hpp"

#include <string>

#include <unistd.h>

using namespace std::string_literals;
using std::chrono::steady_clock;

using msr::Stats;
using msr::SystemStats;

static std::string getOSDesc() { return _fmt::format("MacOS {}", getSysctl<std::string>("kern.osproductversion")); }

static std::string getKernelDesc() {
	return _fmt::format("{} {}", getSysctl<std::string>("kern.ostype"), getSysctl<std::string>("kern.osrelease"));
}

#include <dlfcn.h>

void SystemStats::start() {
	msr::log::info("macosstats", "Collecting resources for Process {}", getpid());
	starttime = steady_clock::now();
	// msr::log::debug("macosstats", "Start systime {} ms, utime {} ms", startSysTime, startUTime);

	IOReportLib ioreport;

	CFDictionaryRef channel;
	auto str1 = CFStringCreateWithCString(kCFAllocatorDefault, "Energy Model", kCFStringEncodingASCII);
	auto str2 = CFStringCreateWithCString(kCFAllocatorDefault, "CPU Stats", kCFStringEncodingASCII);
	auto str3 = CFStringCreateWithCString(kCFAllocatorDefault, "CPU Core Performance States", kCFStringEncodingASCII);
	channel = ioreport.copyChannelsInGroup(str1, nullptr, 0, 0, 0);
	auto channel2 = ioreport.copyChannelsInGroup(str2, str3, 0, 0, 0);
	ioreport.mergeChannels(channel, channel2, nullptr);
	CFRelease(channel2);
	CFRelease(str1);
	CFRelease(str2);
	CFRelease(str3);
	auto size = CFDictionaryGetCount(channel);
	auto mutchan = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, size, channel);
}
void SystemStats::stop() { stoptime = steady_clock::now(); }
void SystemStats::step() {}

Stats SystemStats::getStats() {
	/** \todo: filter by requested metrics */
	auto cpuInfo = getCPUInfo();

	/** \todo this should not be needed once https://github.com/pytorch/cpuinfo/pull/246/files is merged. **/
	cpuInfo.modelname = getSysctl<std::string>("machdep.cpu.brand_string");

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

	auto wallclocktime =
			std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(stoptime - starttime).count());

	return {{MSR_OS_NAME, getOSDesc()},
			{MSR_OS_KERNEL, getKernelDesc()},
			{MSR_TIME_ELAPSED_WALL_CLOCK_MS, wallclocktime},
			{MSR_TIME_ELAPSED_USER_MS, "TODO"s},
			{MSR_TIME_ELAPSED_SYSTEM_MS, "TODO"s},
			{MSR_CPU_USED_PROCESS_PERCENT, "TODO"s},
			{MSR_CPU_USED_SYSTEM_PERCENT, "TODO"s},
			{MSR_CPU_AVAILABLE_SYSTEM_CORES, std::to_string(cpuInfo.numCores)},
			{MSR_CPU_FEATURES, cpuInfo.flags},
			{MSR_CPU_FREQUENCY_MHZ, std::to_string(cpuInfo.frequency)},
			{MSR_CPU_FREQUENCY_MIN_MHZ, "TODO"s},
			{MSR_CPU_FREQUENCY_MAX_MHZ, "TODO"s},
			{MSR_CPU_VENDOR_ID, cpuInfo.vendorId},
			{MSR_CPU_BYTE_ORDER, cpuInfo.endianness},
			{MSR_CPU_ARCHITECTURE, "ARM64"s},
			{MSR_CPU_MODEL_NAME, cpuInfo.modelname},
			{MSR_CPU_CORES_PER_SOCKET, std::to_string(cpuInfo.coresPerSocket)},
			{MSR_CPU_THREADS_PER_CORE, std::to_string(cpuInfo.threadsPerCore)},
			{MSR_CPU_CACHES, caches},
			{MSR_CPU_VIRTUALIZATION, ""s},
			{MSR_RAM_USED_PROCESS_KB, "TODO"s},
			{MSR_RAM_USED_SYSTEM_MB, "TODO"s},
			{MSR_RAM_AVAILABLE_SYSTEM_MB, std::to_string(getSysctl<uint64_t>("hw.memsize_usable") / 1000 / 1000)}};
}
#endif