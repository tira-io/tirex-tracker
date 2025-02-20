/**
 * @file systemstats_linux.cpp
 * @brief Implements linux specific code of the systemstats.hpp header.
 */

#if __linux__
#include "systemstats.hpp"

#include "../../logging.hpp"

#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <cinttypes>
#include <filesystem>
#include <fstream>
#include <optional>

using namespace std::string_literals;
using std::chrono::steady_clock;

using msr::Stats;
using msr::SystemStats;

struct SysInfo {
	std::string osname;		  /**< The name of the operating system that is currently running **/
	std::string kerneldesc;	  /**< The os kernel that is currently running **/
	std::string architecture; /**< The architecture currently running on **/
	unsigned numCores;		  /**< The number of CPU cores of the system **/
	uint64_t totalRamMB;	  /**< The total amount of RAM (in Megabytes) installed in the system **/
};

struct SystemStats::Utilization {
	unsigned ramUsedKB;	 /**< Amount of RAM used by the monitored process alone **/
	unsigned userTimeMs; /**< Time spent by the monitored process in user mode **/
	unsigned sysTimeMs;	 /**< Time spent by the monitored process in system mode **/
	struct {
		unsigned ramUsedMB;		/**< Amount of RAM (in Megabytes) used by all processes **/
		uint8_t cpuUtilization; /**< CPU utilization of all processes **/
	} system;
};

SysInfo getSysInfo();
std::string readDistro();

extern "C" {
// Not part of the public API but we use them for now until there is a public API for frequency
uint32_t cpuinfo_linux_get_processor_cur_frequency(uint32_t processor);
}

void SystemStats::start() {
	msr::log::info("linuxstats", "Collecting resources for Process {}", getpid());
	starttime = steady_clock::now();
	Utilization tmp;
	parseStat(tmp); // Call parseStat once to init lastIdle and lastTotal
	parseStat(getpid(), tmp);
	startUTime = tmp.userTimeMs;
	startSysTime = tmp.sysTimeMs;
	msr::log::debug("linuxstats", "Start systime {} ms, utime {} ms", startSysTime, startUTime);
}
void SystemStats::stop() {
	stoptime = steady_clock::now();
	auto utilization = getUtilization();
	stopUTime = utilization.userTimeMs;
	stopSysTime = utilization.sysTimeMs;
}

void SystemStats::step() {
	auto utilization = getUtilization();
	ram.addValue(utilization.ramUsedKB);
	sysCpuUtil.addValue(utilization.system.cpuUtilization);
	sysRam.addValue(utilization.system.ramUsedMB);
	frequency.addValue(cpuinfo_linux_get_processor_cur_frequency(0));
}

Stats SystemStats::getStats() {
	/** \todo: filter by requested metrics */
	auto cpuInfo = getCPUInfo();

	auto wallclocktime =
			std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(stoptime - starttime).count());

	return {
			{{MSR_TIME_ELAPSED_WALL_CLOCK_MS, wallclocktime},
			 {MSR_TIME_ELAPSED_USER_MS, std::to_string(stopUTime - startUTime)},
			 {MSR_TIME_ELAPSED_SYSTEM_MS, std::to_string(stopSysTime - startSysTime)},
			 {MSR_CPU_USED_PROCESS_PERCENT, "TODO"s},
			 {MSR_CPU_USED_SYSTEM_PERCENT, sysCpuUtil},
			 {MSR_CPU_FREQUENCY_MHZ, frequency},
			 {MSR_RAM_USED_PROCESS_KB, ram},
			 {MSR_RAM_USED_SYSTEM_MB, sysRam}}
	};
}

Stats SystemStats::getInfo() {
	/** \todo: filter by requested metrics */
	auto info = getSysInfo();
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

	return {{MSR_OS_NAME, info.osname},
			{MSR_OS_KERNEL, info.kerneldesc},
			{MSR_CPU_AVAILABLE_SYSTEM_CORES, std::to_string(cpuInfo.numCores)},
			{MSR_CPU_FEATURES, cpuInfo.flags},
			{MSR_CPU_FREQUENCY_MIN_MHZ, std::to_string(cpuInfo.frequency_min)},
			{MSR_CPU_FREQUENCY_MAX_MHZ, std::to_string(cpuInfo.frequency_max)},
			{MSR_CPU_VENDOR_ID, cpuInfo.vendorId},
			{MSR_CPU_BYTE_ORDER, cpuInfo.endianness},
			{MSR_CPU_ARCHITECTURE, info.architecture},
			{MSR_CPU_MODEL_NAME, cpuInfo.modelname},
			{MSR_CPU_CORES_PER_SOCKET, std::to_string(cpuInfo.coresPerSocket)},
			{MSR_CPU_THREADS_PER_CORE, std::to_string(cpuInfo.threadsPerCore)},
			{MSR_CPU_CACHES, caches},
			{MSR_CPU_VIRTUALIZATION,
			 (cpuInfo.virtualization.svm ? "AMD-V "s : ""s) + (cpuInfo.virtualization.vmx ? "VT-x"s : ""s)},
			{MSR_RAM_AVAILABLE_SYSTEM_MB, std::to_string(info.totalRamMB)}};
}

SysInfo getSysInfo() {
	struct utsname uts;
	struct sysinfo info;
	uname(&uts);
	sysinfo(&info);
	return {.osname = readDistro(),
			.kerneldesc = {_fmt::format("{} {} {}", uts.sysname, uts.release, uts.machine)},
			.architecture = uts.machine,
			.numCores = static_cast<unsigned>(sysconf(_SC_NPROCESSORS_ONLN)),
			.totalRamMB = ((std::uint64_t)info.totalram * info.mem_unit) / 1000 / 1000};
}

std::optional<std::string> readDistroFromLSB() {
	std::ifstream stream("/etc/lsb-release");
	if (!stream) {
		msr::log::error("linux", "Could not open /etc/lsb-release");
		return std::nullopt;
	}
	for (std::string line; std::getline(stream, line);) {
		/** \fixme could fail if there are spaces or has no quotes **/
		if (line.starts_with("DISTRIB_DESCRIPTION=\"")) {
			return line.substr(21, line.length() - 21 - 1);
		}
	}
	msr::log::error("linux", "/etc/lsb-release did not contain DISTRIB_DESCRIPTION");
	return std::nullopt;
}

std::optional<std::string> readDistroFromOS() {
	std::ifstream stream("/etc/os-release");
	if (!stream) {
		msr::log::error("linux", "Could not open /etc/os-release");
		return std::nullopt;
	}
	for (std::string line; std::getline(stream, line);) {
		/** \fixme could fail if there are spaces or has no quotes **/
		if (line.starts_with("PRETTY_NAME=\"")) {
			return line.substr(13, line.length() - 13 - 1);
		}
	}
	msr::log::error("linux", "/etc/os-release did not contain PRETTY_NAME");
	return std::nullopt;
}

std::string readDistro() {
	auto val = readDistroFromLSB();
	if (val.has_value())
		return val.value();
	return readDistroFromOS().value_or("(not found)");
}

SystemStats::Utilization SystemStats::getUtilization() {
	Utilization utilization;
	auto pid = getpid();
	parseStat(utilization);
	parseStat(pid, utilization);
	parseStatm(pid, utilization);

	struct sysinfo info;
	sysinfo(&info);
	utilization.system.ramUsedMB =
			((std::uint64_t)(info.totalram - info.freeram - info.bufferram - info.freehigh) * info.mem_unit) / 1000 /
			1000;

	return utilization;
}

void SystemStats::parseMemInfo(Utilization& utilization) {
	auto file = std::filesystem::path("/") / "proc" / "meminfo";
	auto is = std::ifstream(file.c_str());
	std::string key;

	while (is) {
		is >> key;
		if (key == "MemTotal:") {

		} else {
			std::getline(is, key); // Write to key to throw away
		}
	}
	/** \todo: memory used = MemTotal - MemFree - Buffers - Cached - SReclaimable **/
}

void SystemStats::parseStat(Utilization& utilization) {
	// Section 1.7 in https://www.kernel.org/doc/html/latest/filesystems/proc.html
	auto statFile = std::filesystem::path("/") / "proc" / "stat";
	auto is = std::ifstream(statFile.c_str());
	std::string cpu;
	size_t user, nice, system, idle, iowait, irq, softirq, steal, guest, guestnice;
	is >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal >> guest >> guestnice;

	auto total = user + nice + system + idle + iowait + irq + softirq + steal + guest + guestnice;
	if (total - lastTotal == 0) {
		// Not enough time has passed
		utilization.system.cpuUtilization = 100;
	} else {
		utilization.system.cpuUtilization = 100 - (((idle - lastIdle) * 100) / (total - lastTotal));

		lastIdle = idle;
		lastTotal = total;
	}
}

void SystemStats::parseStatm(pid_t pid, Utilization& utilization) {
	// Table 1-3 in https://www.kernel.org/doc/html/latest/filesystems/proc.html
	auto statFile = std::filesystem::path("/") / "proc" / std::to_string(pid) / "statm";
	auto is = std::ifstream(statFile.c_str());
	size_t ignore, resident;
	is >> ignore >> resident;
	utilization.ramUsedKB = (resident * getpagesize()) / 1000;
}

void SystemStats::parseStat(pid_t pid, Utilization& utilization) {
	// Table 1-4 in https://www.kernel.org/doc/html/latest/filesystems/proc.html
	auto statFile = std::filesystem::path("/") / "proc" / std::to_string(pid) / "stat";
	auto is = std::ifstream(statFile.c_str());
	size_t ignore, utime, stime;
	char cignore;
	is >> ignore;
	// Skip filename
	while (is && is.get() != ')')
		;
	is >> cignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >>
			utime >> stime;
	auto ticksPerSec = (unsigned)sysconf(_SC_CLK_TCK);
	utilization.userTimeMs = (utime * 1000u) / ticksPerSec;
	utilization.sysTimeMs = (stime * 1000u) / ticksPerSec;
}
#endif