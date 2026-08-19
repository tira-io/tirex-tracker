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
#include <limits>
#include <optional>
#include <sstream>

using std::chrono::steady_clock;
using std::chrono::system_clock;

using tirex::Stats;
using tirex::SystemStats;

std::string readDistro();

extern "C" {
// Not part of the public API but we use them for now until there is a public API for frequency
uint32_t cpuinfo_linux_get_processor_cur_frequency(uint32_t processor);
}

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
	std::ifstream is(statFile.c_str());
	std::string line;
	if (!std::getline(is, line)) {
		tirex::log::error("linuxstats", "Failed to read {}", statFile.string());
		return {0, 0};
	}
	/** The `comm` field is parenthesized and, per proc(5), may itself contain ')' (e.g. a process renamed via
	 *  prctl(PR_SET_NAME)) without the kernel escaping it. As such, we need to scan for the *last* ')' to correctly
	 *  locate the end of the comm field. */
	auto closeParen = line.rfind(')');
	if (closeParen == std::string::npos) {
		tirex::log::error("linuxstats", "Unexpected format in {}", statFile.string());
		return {0, 0};
	}
	std::istringstream rest(line.substr(closeParen + 1));
	size_t ignore, utime, stime;
	char cignore;
	if (!(rest >> cignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >>
		  ignore >> utime >> stime)) {
		tirex::log::error("linuxstats", "Failed to parse {}", statFile.string());
		return {0, 0};
	}
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

std::vector<std::string> SystemStats::getInvocationCmd() {
	std::ifstream fstream(_fmt::format("/proc/{}/cmdline", pid), std::ios::in | std::ios::binary);
	std::vector<std::string> args;
	for (std::string line; std::getline(fstream, line, '\0');)
		args.emplace_back(std::move(line));
	return args;
}

void SystemStats::start() {
	for (auto tmp : getInvocationCmd())
		tirex::log::info("linuxstats", "{}", tmp);
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
	parseMemInfo(utilization);
	utilization.cpuUtilization = getProcCPUUtilization();

	return utilization;
}

namespace {
	/** @brief The subset of /proc/meminfo fields needed to compute "used" RAM without counting reclaimable page
	 *  cache/slab as used. All values are in kibibytes, as reported by the kernel. */
	struct MemInfoFields {
		std::optional<std::uint64_t> memTotalKB;
		std::optional<std::uint64_t> memFreeKB;
		std::optional<std::uint64_t> buffersKB;
		std::optional<std::uint64_t> cachedKB;
		std::optional<std::uint64_t> sreclaimableKB;
	};

	/**
	 * @brief Parses the fields of MemInfoFields out of a stream formatted like /proc/meminfo.
	 * @details Factored out from the file-reading code so it can be unit tested against a synthetic input without
	 * needing a real /proc/meminfo.
	 */
	MemInfoFields parseMemInfoFields(std::istream& is) {
		MemInfoFields fields;
		std::string key;
		std::uint64_t value;
		while (is >> key >> value) {
			if (key == "MemTotal:")
				fields.memTotalKB = value;
			else if (key == "MemFree:")
				fields.memFreeKB = value;
			else if (key == "Buffers:")
				fields.buffersKB = value;
			else if (key == "Cached:")
				fields.cachedKB = value;
			else if (key == "SReclaimable:")
				fields.sreclaimableKB = value;
			// Discard the rest of the line (e.g. the trailing "kB" unit).
			is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		}
		return fields;
	}
} // namespace

void SystemStats::parseMemInfo(Utilization& utilization) {
	auto file = std::filesystem::path("/") / "proc" / "meminfo";
	auto is = std::ifstream(file.c_str());
	auto fields = parseMemInfoFields(is);

	if (fields.memTotalKB && fields.memFreeKB && fields.buffersKB && fields.cachedKB && fields.sreclaimableKB) {
		auto usedKB =
				*fields.memTotalKB - *fields.memFreeKB - *fields.buffersKB - *fields.cachedKB - *fields.sreclaimableKB;
		utilization.system.ramUsedMB = static_cast<unsigned>(usedKB / 1000);
		return;
	}

	tirex::log::warn(
			"linuxstats", "Failed to parse /proc/meminfo; falling back to a sysinfo()-based approximation that "
						   "does not exclude reclaimable page cache from \"used\" RAM"
	);
	struct sysinfo info;
	sysinfo(&info);
	utilization.system.ramUsedMB =
			static_cast<unsigned>(
					((std::uint64_t)(info.totalram - info.freeram - info.bufferram - info.freehigh) * info.mem_unit)
			) /
			1000 / 1000;
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
		tirex::log::warn("linuxstats", "Called too quickly apart (0 jiffies elapsed)");
		utilization.system.cpuUtilization = 0;
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
	if (is >> ignore >> resident) {
		utilization.ramUsedKB = static_cast<unsigned>((resident * getpagesize()) / 1000);
	} else {
		tirex::log::error("linuxstats", "Failed to read {}", statFile.string());
		utilization.ramUsedKB = 0;
	}
}

#endif