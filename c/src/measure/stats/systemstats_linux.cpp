#if __linux__
#include "systemstats.hpp"

#include "../../logging.hpp"

#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#include <cinttypes>
#include <filesystem>
#include <fstream>

using std::chrono::steady_clock;

using am::Stats;
using am::SystemStats;

const char* SystemStats::version = nullptr;

struct SysInfo {
	unsigned numCores;	 /**< The number of CPU cores of the system **/
	uint64_t totalRamMB; /**< The total amount of RAM (in Megabytes) installed in the system **/
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

void SystemStats::start() {
	measureapi::log::info("linuxstats", "Collecting resources for Process {}", getpid());
	starttime = steady_clock::now();
	Utilization tmp;
	parseStat(tmp); // Call parseStat once to init lastIdle and lastTotal
	startUTime = tmp.userTimeMs;
	startSysTime = tmp.sysTimeMs;
	measureapi::log::debug("linuxstats", "Start systime {} ms, utime {} ms", startSysTime, startUTime);
}
void SystemStats::stop() { stoptime = steady_clock::now(); }
void SystemStats::step() {
	auto utilization = getUtilization();
	ram.addValue(utilization.ramUsedKB);
	sysCpuUtil.addValue(utilization.system.cpuUtilization);
	sysRam.addValue(utilization.system.ramUsedMB);
}

Stats SystemStats::getStats() {
	auto info = getSysInfo();
	auto utilization = getUtilization();
	/** \todo For more accurate reading: measure utime and stime in start() and report only the difference to the start
	 *   value **/

	return {
			{{"system",
			  {{"num cores", std::to_string(info.numCores)},
			   {"RAM (MB)", std::to_string(info.totalRamMB)},
			   {"CPU Utilization Max (%)", std::to_string(sysCpuUtil.maxValue())},
			   {"Max RAM used (MB)", std::to_string(sysRam.maxValue())}}},
			 {"elapsed time",
			  {{"wallclock (ms)",
				{std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(stoptime - starttime).count())}},
			   {"system (ms)", std::to_string(utilization.sysTimeMs - startSysTime)},
			   {"user (ms)", std::to_string(utilization.userTimeMs - startUTime)}}},
			 {"resources", {{"Max RAM used (KB)", {std::to_string(ram.maxValue())}}}}}
	};
}

SysInfo getSysInfo() {
	struct sysinfo info;
	sysinfo(&info);
	return {.numCores = static_cast<unsigned>(sysconf(_SC_NPROCESSORS_ONLN)),
			.totalRamMB = ((std::uint64_t)info.totalram * info.mem_unit) / 1000 / 1000};
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
	auto ticksPerSec = sysconf(_SC_CLK_TCK);
	utilization.userTimeMs = (utime * 1000) / ticksPerSec;
	utilization.sysTimeMs = (stime * 1000) / ticksPerSec;
}
#endif