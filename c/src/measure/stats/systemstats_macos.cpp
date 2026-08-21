/**
 * @file systemstats_macos.cpp
 * @brief Implements macos specific code of the systemstats.hpp header.
 */

#if defined(__APPLE__)

#include "systemstats.hpp"

#include "../../logging.hpp"
#include "../utils/osutils.hpp"
#include "./details/macos/ioreport.h"
#include "./details/macos/sysctl.hpp"

#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>

#include <libproc.h>
#include <mach/mach.h>
#include <sys/resource.h>
#include <unistd.h>

using namespace std::string_literals;
using std::chrono::steady_clock;
using std::chrono::system_clock;

using tirex::Stats;
using tirex::SystemStats;

static std::string getOSDesc() { return _fmt::format("MacOS {}", getSysctl<std::string>("kern.osproductversion")); }

static std::string getKernelDesc() {
	return _fmt::format("{} {}", getSysctl<std::string>("kern.ostype"), getSysctl<std::string>("kern.osrelease"));
}

namespace {
	/**
	 * @brief Reads a process's cumulative (stime, utime) CPU ticks from /proc/<pid>/stat. 
	 * @returns std::nullopt if \p pid doesn't exist or the file is unreadable/malformed.
	 */
	std::optional<std::tuple<size_t, size_t>> readProcessCpuTicks(int64_t pid) {
		proc_taskinfo taskInfo;
		if (int err;
			(err = proc_pidinfo(static_cast<pid_t>(pid), PROC_PIDTASKINFO, 0, &taskInfo, PROC_PIDTASKINFO_SIZE)) != 0)
			return std::make_tuple(taskInfo.pti_total_system, taskInfo.pti_total_user);
		return std::nullopt;
	}
} // namespace

unsigned SystemStats::getProcCPUUtilization() {
	auto [systime, utime] = getSysAndUserTime();
	auto time = steady_clock::now();
	auto timeActiveMs = tickToMs(systime + utime);
	auto totTime = std::chrono::duration_cast<std::chrono::milliseconds>(time - lastProcTime).count();
	if (totTime != 0) {
		auto percent = static_cast<unsigned>((timeActiveMs - lastProcActiveMs) * 100 / totTime);
		lastProcTime = time;
		lastProcActiveMs = timeActiveMs;
		return percent;
	} else {
		tirex::log::warn("macosstats", "Called too quickly apart ({} ms)", totTime);
	}
	return 0;
}

/**
 * @brief Like getProcCPUUtilization(), but sums the CPU usage of `pid` and every descendant process discovered via
 * tirex::utils::discoverProcessTree() at the time of the call. See the Linux implementation of this function for the
 * full rationale; this mirrors it exactly, just backed by proc_pidinfo instead of /proc/<pid>/stat.
 */
unsigned SystemStats::getTreeCPUUtilization() {
	auto time = steady_clock::now();
	auto totTime = std::chrono::duration_cast<std::chrono::milliseconds>(time - lastProcTime).count();
	if (totTime == 0) {
		tirex::log::warn("macosstats", "Called too quickly apart (0 ms)");
		return 0;
	}

	unsigned totalPercent = 0;
	std::unordered_map<int64_t, size_t> newState;
	for (int64_t member : tirex::utils::discoverProcessTree(pid)) {
		auto ticks = readProcessCpuTicks(member);
		if (!ticks)
			continue; // Vanished mid-scan or otherwise unreadable; simply excluded from this tick's total.
		auto activeMs = tickToMs(std::get<0>(*ticks) + std::get<1>(*ticks));
		if (auto it = treeLastActiveMs.find(member); it != treeLastActiveMs.end() && activeMs >= it->second)
			totalPercent += static_cast<unsigned>(((activeMs - it->second) * 100) / totTime);
		// Else: first sighting of this PID (or its CPU time went backwards, e.g. PID reuse) -- contributes nothing
		// this tick, matching the single-process "not enough history yet" behavior above.
		newState[member] = activeMs;
	}
	treeLastActiveMs = std::move(newState); // Prune state for PIDs no longer in the tree.
	lastProcTime = time;
	return totalPercent;
}

unsigned SystemStats::getCPUUtilization() {
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
	unsigned util = 0;
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
	if (auto ticks = readProcessCpuTicks(pid))
		return *ticks;
	tirex::log::error("macosstats", "Failed to get task info for PID {}", pid);
	return {0, 0};
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
	uint64_t usedPages = (uint64_t)vmstat.active_count + (uint64_t)vmstat.inactive_count + (uint64_t)vmstat.wire_count +
						 (uint64_t)vmstat.speculative_count + (uint64_t)vmstat.compressor_page_count;
	uint64_t excludedPages = (uint64_t)vmstat.purgeable_count + (uint64_t)vmstat.external_page_count;
	/** Guard against unsigned underflow: excludedPages could in principle exceed usedPages (e.g. a system with a large
	 * amount of purgeable/file-backed memory relative to the other categories), which would otherwise wrap around to a
	 * value near UINT64_MAX. */
	if (excludedPages > usedPages)
		return 0;
	return (usedPages - excludedPages) * page_size / 1000'000u;
}

namespace {
	/**
	 * @brief Reads a process's "memory footprint" (ri_phys_footprint) via proc_pid_rusage(RUSAGE_INFO_V4), in bytes.
	 * This is Apple's own per-process memory accounting (the same one Activity Monitor shows), which already excludes
	 * some clean/reclaimable file-backed pages better than raw RSS; but unlike Linux's PSS, it is *not* a fair-share
	 * accounting of pages shared between processes, so summing it across a tree can still overcount memory that tree's
	 * own members share with *each other* (e.g. a shared mapping between two tracked children). This is the best
	 * available approximation on macOS, which has no PSS or Job-Object equivalent.
	 * 
	 * @returns std::nullopt if `pid` doesn't exist or can't be inspected.
	 */
	std::optional<std::uint64_t> readProcessPhysFootprint(int64_t pid) {
		rusage_info_v4 info;
		if (proc_pid_rusage(static_cast<pid_t>(pid), RUSAGE_INFO_V4, reinterpret_cast<rusage_info_t*>(&info)) != 0)
			return std::nullopt;
		return static_cast<std::uint64_t>(info.ri_phys_footprint);
	}
} // namespace

unsigned SystemStats::getTreeRAMUsageKB() {
	std::uint64_t totalBytes = 0;
	for (int64_t member : tirex::utils::discoverProcessTree(pid)) {
		if (auto footprint = readProcessPhysFootprint(member))
			totalBytes += *footprint;
		// Else: vanished mid-scan or otherwise uninspectable; simply excluded from this tick's total.
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
			.architecture = "ARM64"s,
			.totalRamMB = getSysctl<uint64_t>("hw.memsize_usable") / 1000 / 1000};
}

#include <dlfcn.h>

std::vector<std::string> SystemStats::getInvocationCmd() {
	int mib[3] = {CTL_KERN, KERN_PROCARGS2, pid};
	size_t size = 0;
	if (sysctl(mib, sizeof(mib) / sizeof(*mib), nullptr, &size, nullptr, 0) != 0) {
		tirex::log::error("macosstats", "Failed to get task info for PID {} with error code", pid);
		return {};
	}

	std::vector<char> buffer(size);
	if (sysctl(mib, sizeof(mib) / sizeof(*mib), buffer.data(), &size, nullptr, 0) != 0) {
		tirex::log::error("macosstats", "Failed to get task info for PID {} with error code", pid);
		return {};
	}
	int argc = *reinterpret_cast<int*>(buffer.data());
	char* argv = buffer.data() + sizeof(int);
	std::string arg;
	std::vector<std::string> args;
	std::istringstream stream{std::string{argv, buffer.size() - sizeof(int)}};
	std::getline(stream, arg, '\0'); // Discard first entry since it is a repetition of the command name.
	size_t argi = 0;
	for (size_t argi = 0; std::getline(stream, arg, '\0') && argi < argc; ++argi)
		args.emplace_back(arg);
	// The stream will now be followed by environment variables and others such that we need to check that argi<argc to
	// terminate the loop.
	return args;
}

void SystemStats::start() {
	for (auto tmp : getInvocationCmd())
		tirex::log::info("macosstats", "{}", tmp);
	tirex::log::info("macosstats", "Collecting resources for Process {}", pid);
	starttimer = steady_clock::now();
	startTimepoint = system_clock::now();
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

void SystemStats::step() {
	auto utilization = getUtilization();
	ram.addValue(utilization.ramUsedKB);
	sysRam.addValue(utilization.system.ramUsedMB);
	cpuUtil.addValue(utilization.cpuUtilization);
	sysCpuUtil.addValue(utilization.system.cpuUtilization);
	// frequency.addValue(); /** \todo implement **/
}

#endif