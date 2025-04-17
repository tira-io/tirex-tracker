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

using std::chrono::steady_clock;
using std::chrono::system_clock;

using tirex::Stats;
using tirex::SystemStats;

std::string readDistro();

extern "C" {
// Not part of the public API but we use them for now until there is a public API for frequency
uint32_t cpuinfo_linux_get_processor_cur_frequency(uint32_t processor);
}

uint8_t SystemStats::getProcCPUUtilization() {
	auto [systime, utime] = getSysAndUserTime();
	auto time = steady_clock::now();
	auto timeActiveMs = tickToMs(systime + utime);
	auto totTime = std::chrono::duration_cast<std::chrono::milliseconds>(time - lastProcTime).count();
	if (totTime != 0) {
		auto percent = static_cast<uint8_t>((timeActiveMs - lastProcActiveMs) * 100 / totTime);
		lastProcTime = time;
		lastProcActiveMs = timeActiveMs;
		return percent;
	} else {
		tirex::log::warn("linuxstats", "Called too quickly apart ({} ms)", totTime);
	}
	return 0;
}

size_t SystemStats::tickToMs(size_t tick) {
	static const auto ticksPerSec = static_cast<unsigned>(sysconf(_SC_CLK_TCK));
	return (tick * 1000u) / ticksPerSec;
}

std::tuple<size_t, size_t> SystemStats::getSysAndUserTime() const {
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
	return {stime, utime};
}

SystemStats::SysInfo SystemStats::getSysInfo() {
	struct utsname uts;
	struct sysinfo info;
	uname(&uts);
	sysinfo(&info);
	return {.osname = readDistro(),
			.kerneldesc = {_fmt::format("{} {} {}", uts.sysname, uts.release, uts.machine)},
			.architecture = uts.machine,
			.totalRamMB = ((std::uint64_t)info.totalram * info.mem_unit) / 1000 / 1000};
}

void SystemStats::start() {
	tirex::log::info("linuxstats", "Collecting resources for Process {}", pid);
	starttimer = steady_clock::now();
	startTimepoint = system_clock::now();
	std::tie(startSysTime, startUTime) = getSysAndUserTime();
	tirex::log::debug("linuxstats", "Start systime {} ms, utime {} ms", tickToMs(startSysTime), tickToMs(startUTime));
	getUtilization(); // Call getUtilization once to init CPU Utilization tracking
}

void SystemStats::step() {
	auto utilization = getUtilization();
	ram.addValue(utilization.ramUsedKB);
	sysRam.addValue(utilization.system.ramUsedMB);
	cpuUtil.addValue(utilization.cpuUtilization);
	sysCpuUtil.addValue(utilization.system.cpuUtilization);
	frequency.addValue(cpuinfo_linux_get_processor_cur_frequency(0));
}

std::optional<std::string> readDistroFromLSB() {
	std::ifstream stream("/etc/lsb-release");
	if (!stream) {
		tirex::log::error("linux", "Could not open /etc/lsb-release");
		return std::nullopt;
	}
	for (std::string line; std::getline(stream, line);) {
		/** \fixme could fail if there are spaces or has no quotes **/
		if (line.starts_with("DISTRIB_DESCRIPTION=\"")) {
			return line.substr(21, line.length() - 21 - 1);
		}
	}
	tirex::log::error("linux", "/etc/lsb-release did not contain DISTRIB_DESCRIPTION");
	return std::nullopt;
}

std::optional<std::string> readDistroFromOS() {
	std::ifstream stream("/etc/os-release");
	if (!stream) {
		tirex::log::error("linux", "Could not open /etc/os-release");
		return std::nullopt;
	}
	for (std::string line; std::getline(stream, line);) {
		/** \fixme could fail if there are spaces or has no quotes **/
		if (line.starts_with("PRETTY_NAME=\"")) {
			return line.substr(13, line.length() - 13 - 1);
		}
	}
	tirex::log::error("linux", "/etc/os-release did not contain PRETTY_NAME");
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
	parseStat(utilization);
	parseStatm(pid, utilization);

	struct sysinfo info;
	sysinfo(&info);
	utilization.system.ramUsedMB =
			((std::uint64_t)(info.totalram - info.freeram - info.bufferram - info.freehigh) * info.mem_unit) / 1000 /
			1000;
	utilization.cpuUtilization = getProcCPUUtilization();

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

#endif