#ifndef STATS_SYSTEMSTATS_HPP
#define STATS_SYSTEMSTATS_HPP

#include "../measure.hpp"
#include "provider.hpp"

#include <chrono>
#include <string>
#include <tuple>
#include <vector>

#if _WINDOWS
#include <windows.h>
#undef ERROR //  Make problems with logging.h otherwise
#endif

namespace msr {
	class SystemStats final : public StatsProvider {
	public:
		struct SysInfo {
			std::string osname;		  /**< The name of the operating system that is currently running **/
			std::string kerneldesc;	  /**< The os kernel that is currently running **/
			std::string architecture; /**< The architecture currently running on **/
			uint64_t totalRamMB;	  /**< The total amount of RAM (in Megabytes) installed in the system **/
		};
		struct CPUInfo {
			struct Cache {
				unsigned unified;  /**< LX cache size in byte **/
				unsigned instruct; /**< LXi cache size in byte **/
				unsigned data;	   /**< LXd cache size in byte **/
			};
			std::string modelname;
			std::string vendorId;
			unsigned numCores; /**< The number of CPU cores of the system **/
			unsigned coresPerSocket;
			unsigned threadsPerCore;
			std::vector<Cache> caches;
			std::string endianness;
			uint32_t frequency_min;
			uint32_t frequency_max;
			std::string flags;
			struct VirtFlags {
				bool svm; /**< AMD-V support **/
				bool vmx; /**< VT-x support **/
			} virtualization;
		};

	private:
		std::chrono::steady_clock::time_point starttime;
		std::chrono::steady_clock::time_point stoptime;

		msr::TimeSeries<unsigned> ram{true};
		msr::TimeSeries<unsigned> sysRam{true};
		msr::TimeSeries<unsigned> cpuUtil{true};
		msr::TimeSeries<unsigned> sysCpuUtil{true};
		msr::TimeSeries<uint32_t> frequency{true};

		size_t startUTime, stopUTime;
		size_t startSysTime, stopSysTime;

		struct Utilization {
			unsigned ramUsedKB;		/**< Amount of RAM used by the monitored process alone **/
			uint8_t cpuUtilization; /**< CPU utilization (in percent) of the tracked process **/
			struct {
				unsigned ramUsedMB;		/**< Amount of RAM (in Megabytes) used by all processes **/
				uint8_t cpuUtilization; /**< CPU utilization of all processes **/
			} system;
		};
		Utilization getUtilization();
		std::tuple<size_t, size_t> getSysAndUserTime() const;
		static size_t tickToMs(size_t tick);

#if __linux__
		size_t lastIdle;
		size_t lastTotal;
		size_t lastProcActiveMs;
		std::chrono::steady_clock::time_point lastProcTime;

		void parseMemInfo(Utilization& utilization);
		void parseStat(Utilization& utilization);
		void parseStatm(pid_t pid, Utilization& utilization);
		uint8_t getProcCPUUtilization();
#elif _WINDOWS
		FILETIME prevSysIdle, prevSysKernel, prevSysUser;
		ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
		unsigned numProcessors;

		uint8_t getCPUUtilization();
		uint8_t getProcCPUUtilization(HANDLE pid);
#endif

	public:
		SystemStats();

		void start() override;
		void stop() override;
		void step() override;
		Stats getStats() override;
		Stats getInfo() override;

		static constexpr const char* description = "Collects system components and utilization metrics.";
		static const char* version;
		static const std::set<msrMeasure> measures;

	private:
		static SysInfo getSysInfo();
		static CPUInfo getCPUInfo();
	};
} // namespace msr

#endif