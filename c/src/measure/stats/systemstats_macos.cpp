/**
 * @file systemstats_macos.cpp
 * @brief Implements macos specific code of the systemstats.hpp header.
 */

#if defined(__APPLE__)

#include "systemstats.hpp"

#include "../../logging.hpp"
#include "./details/macos/ioreport.h"
#include "./details/macos/sysctl.hpp"

#include <string>

#include <libproc.h>
#include <mach/mach.h>
#include <unistd.h>

using namespace std::string_literals;
using std::chrono::steady_clock;

using tirex::Stats;
using tirex::SystemStats;

static std::string getOSDesc() { return _fmt::format("MacOS {}", getSysctl<std::string>("kern.osproductversion")); }

static std::string getKernelDesc() {
	return _fmt::format("{} {}", getSysctl<std::string>("kern.ostype"), getSysctl<std::string>("kern.osrelease"));
}

uint8_t SystemStats::getProcCPUUtilization() {
	auto [systime, utime] = getSysAndUserTime();
	auto time = steady_clock::now();
	auto timeActiveMs = tickToMs(systime + utime);
	auto percent = static_cast<uint8_t>(
			(timeActiveMs - lastProcActiveMs) * 100 /
			std::chrono::duration_cast<std::chrono::milliseconds>(time - lastProcTime).count()
	);
	lastProcTime = time;
	lastProcActiveMs = timeActiveMs;
	return percent;
}

uint8_t SystemStats::getCPUUtilization() {
	static thread_local host_cpu_load_info cpuLoad;
	auto count = HOST_CPU_LOAD_INFO_COUNT;
	if (kern_return_t err;
		(err = host_statistics64(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info64_t)&cpuLoad, &count)) !=
		KERN_SUCCESS) {
		tirex::log::error("macosstats", "Failed to fetch VM statistics with error code {}", err);
		return 0;
	}
	size_t total = (size_t)cpuLoad.cpu_ticks[CPU_STATE_USER] + (size_t)cpuLoad.cpu_ticks[CPU_STATE_SYSTEM] +
				   (size_t)cpuLoad.cpu_ticks[CPU_STATE_IDLE] + (size_t)cpuLoad.cpu_ticks[CPU_STATE_NICE];
	uint8_t util = 0;
	if ((total - lastTotal) > 0) { // Otherwise not enough time has passed yet
		util = 100 - (((cpuLoad.cpu_ticks[CPU_STATE_IDLE] - lastIdle) * 100) / (total - lastTotal));
		lastIdle = cpuLoad.cpu_ticks[CPU_STATE_IDLE];
		lastTotal = total;
	} else {
		tirex::log::warn("macosstats", "Called too quickly apart ({} ticks)", total - lastTotal);
	}
	auto tmp = sysconf(_SC_CLK_TCK);
	return util;
}

std::tuple<size_t, size_t> SystemStats::getSysAndUserTime() const {
	proc_taskinfo taskInfo;
	if (int err; (err = proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &taskInfo, PROC_PIDTASKINFO_SIZE)) != 0) {
		return {taskInfo.pti_total_system, taskInfo.pti_total_user};
	} else {
		tirex::log::error("macosstats", "Failed to get task info for PID {} with error code {}", pid, err);
		return {0, 0};
	}
}

size_t SystemStats::tickToMs(size_t tick) {
	/** "Tick" may be the wrong word here but proc_pidinfo returns time in nanoseconds which we convert to ms here. **/
	return tick / 1000'000u;
}

static unsigned getRAMUsageKB(pid_t pid) {
	proc_taskinfo taskInfo;
	if (int err; (err = proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &taskInfo, PROC_PIDTASKINFO_SIZE)) != 0) {
		return taskInfo.pti_resident_size / 1000;
	} else {
		tirex::log::error("macosstats", "Failed to get task info for PID {} with error code {}", pid, err);
		return 0;
	}
}

static unsigned getSystemRAMUsageMB() {
	uint64_t page_size = sysconf(_SC_PAGE_SIZE);
	mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
	vm_statistics64 vmstat;
	if (kern_return_t err;
		(err = host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info_t)&vmstat, &count)) != KERN_SUCCESS) {
		tirex::log::error("macosstats", "Failed to fetch VM statistics with error code {}", err);
		return 0;
	}
	return ((uint64_t)vmstat.active_count + (uint64_t)vmstat.inactive_count + (uint64_t)vmstat.wire_count +
			(uint64_t)vmstat.speculative_count + (uint64_t)vmstat.compressor_page_count -
			(uint64_t)vmstat.purgeable_count - (uint64_t)vmstat.external_page_count) *
		   page_size / 1000'000u;
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
			.architecture = "ARM64"s,
			.totalRamMB = getSysctl<uint64_t>("hw.memsize_usable") / 1000 / 1000};
}

#include <dlfcn.h>

void SystemStats::start() {
	tirex::log::info("macosstats", "Collecting resources for Process {}", pid);
	starttime = steady_clock::now();
	std::tie(startSysTime, startUTime) = getSysAndUserTime();
	tirex::log::debug("macosstats", "Start systime {} ms, utime {} ms", tickToMs(startSysTime), tickToMs(startUTime));

	lastTotal = lastIdle = lastProcActiveMs = 0;
	getUtilization(); // Call getUtilization once to init CPU Utilization tracking

	// Experimenting around with ioreport (used to get energy readings and CPU frequency)
#if 0
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

	CFMutableDictionaryRef dict;
	auto sub = ioreport.createSubscription(nullptr, mutchan, dict, 0, nullptr);
	auto sample1 = ioreport.createSamples(sub, mutchan, nullptr);
	sleep(10);
	auto sample2 = ioreport.createSamples(sub, mutchan, nullptr);

	auto delta = ioreport.createSamplesDelta(sample1, sample2, nullptr);
	auto tmp = CFStringCreateWithCString(kCFAllocatorDefault, "IOReportChannels", kCFStringEncodingASCII);
	auto tmp2 = (CFArrayRef)CFDictionaryGetValue(delta, tmp);
	for (size_t i = 0; i < CFArrayGetCount(tmp2); ++i) {
		auto tmp = (CFDictionaryRef)CFArrayGetValueAtIndex(tmp2, i);
		auto group = ioreport.channelGetGroup(tmp);
		auto subgroup = ioreport.channelGetSubGroup(tmp);
		auto channel = ioreport.channelGetChannelName(tmp);
		auto unit = ioreport.channelGetUnitLabel(tmp);
	}
	CFShow(delta);
#endif
}

void SystemStats::stop() {
	stoptime = steady_clock::now();
	std::tie(stopSysTime, stopUTime) = getSysAndUserTime();
}

void SystemStats::step() {
	auto utilization = getUtilization();
	ram.addValue(utilization.ramUsedKB);
	sysRam.addValue(utilization.system.ramUsedMB);
	cpuUtil.addValue(utilization.cpuUtilization);
	sysCpuUtil.addValue(utilization.system.cpuUtilization);
	// frequency.addValue(); /** \todo implement **/
}

#endif