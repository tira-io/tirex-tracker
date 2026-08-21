#ifndef STATS_SYSTEMSTATS_HPP
#define STATS_SYSTEMSTATS_HPP

#include "../timeseries.hpp"
#include "provider.hpp"

#include <chrono>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#if defined(_WINDOWS) || defined(_WIN32) || defined(WIN32)
#define NOGDI // Otherwise we get problems with logging
#include <windows.h>
#endif

namespace tirex {
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
		/**
		 * @brief The starting timepoint of the tracking on the steady timer.
		 * @details This uses the steady clock. Use startTimepoint to get the timestamp.
		 */
		std::chrono::steady_clock::time_point starttimer;
		/**
		 * @brief The stopping timepoint of the tracking on the steady timer.
		 * @details This uses the steady clock. Use stopTimepoint to get the timestamp.
		 */
		std::chrono::steady_clock::time_point stoptimer;
		/**
		 * @brief The starting timepoint of the tracking on the system clock.
		 * @details This uses the system clock. Use starttimer to measure runtime.
		 */
		std::chrono::system_clock::time_point startTimepoint;
		/**
		 * @brief The stopping timepoint of the tracking on the system clock.
		 * @details This uses the system clock. Use stoptimer to measure runtime.
		 */
		std::chrono::system_clock::time_point stopTimepoint;

		tirex::TimeSeries<unsigned> ram{300, TIREX_AGG_MAX};		 /** \todo make agg configurable */
		tirex::TimeSeries<unsigned> sysRam{300, TIREX_AGG_MAX};		 /** \todo make agg configurable */
		tirex::TimeSeries<unsigned> cpuUtil{300, TIREX_AGG_MEAN};	 /** \todo make agg configurable */
		tirex::TimeSeries<unsigned> sysCpuUtil{300, TIREX_AGG_MEAN}; /** \todo make agg configurable */
		tirex::TimeSeries<uint32_t> frequency{300, TIREX_AGG_MAX};	 /** \todo make agg configurable */

		/** @brief Not yet acted upon; see tirexTrackingConf::trackSubprocesses. */
		bool trackSubprocesses;

		size_t startUTime, stopUTime;
		size_t startSysTime, stopSysTime;

		struct Utilization {
			unsigned ramUsedKB;		 /**< Amount of RAM used by the monitored process alone **/
			unsigned cpuUtilization; /**< CPU utilization (in percent) of the tracked process **/
			struct {
				unsigned ramUsedMB;		 /**< Amount of RAM (in Megabytes) used by all processes **/
				unsigned cpuUtilization; /**< CPU utilization of all processes **/
			} system;
		};
		Utilization getUtilization();
		std::tuple<size_t, size_t> getSysAndUserTime() const;
		static size_t tickToMs(size_t tick);

		unsigned getProcCPUUtilization();
		/**
		 * @brief Like getProcCPUUtilization(), but sums the CPU usage of `pid` and every descendant process discovered
		 * via tirex::utils::discoverProcessTree() at the time of the call.
		 * @details Only used when trackSubprocesses is set; safe to sum across processes since CPU time, unlike RAM, is
		 * never shared between them.
		 * @see SystemStats::getProcCPUUtilization

		 */
		unsigned getTreeCPUUtilization();
		/**
		 * @brief Sums an approximation of unique RAM usage across `pid` and every descendant process discovered via
		 * tirex::utils::discoverProcessTree() at the time of the call, without double-counting memory pages shared
		 * between them (e.g. shared libraries, fork()-inherited copy-on-write pages).
		 * @details Only used when trackSubprocesses is set. Unlike CPU time, RAM cannot simply be summed per-process
		 * (that would double-count shared pages), so each platform uses its own best-available deduplicated
		 * accounting: PSS on Linux, ri_phys_footprint on macOS, PrivateWorkingSetSize on Windows. Each has different
		 * precision characteristics (documented in their respective implementations) so this remains an approximation.
		 */
		unsigned getTreeRAMUsageKB();

		/**
		 * @brief Gets the command line arguments of the invocation of the tracked process.
		 * 
		 * @return A vector of strings, where the i-th entry corresponds to `argv[i]` passed to the program.
		 */
		std::vector<std::string> getInvocationCmd();
#if __linux__
		pid_t pid; /**< The process identifier of the tracked process. */
		size_t lastIdle = 0;
		size_t lastTotal = 0;
		size_t lastProcActiveMs = 0;
		std::chrono::steady_clock::time_point lastProcTime{};
		/** @brief Per-PID active-CPU-ms as of the last getTreeCPUUtilization() call, for every process that was part of
		 * the tree at that time. Pruned to the current tree on every call, so a PID that leaves the tree (or was reused
		 * by an unrelated process) doesn't linger and doesn't get a misleading delta computed against its old state. */
		std::unordered_map<int64_t, size_t> treeLastActiveMs;

		void parseMemInfo(Utilization& utilization);
		void parseStat(Utilization& utilization);
		void parseStatm(pid_t pid, Utilization& utilization);
#elif defined(_WINDOWS) || defined(_WIN32) || defined(WIN32)
		HANDLE pid; /**< The process identifier of the tracked process. */
		FILETIME prevSysIdle, prevSysKernel, prevSysUser;
		ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
		/** @brief Per-PID cumulative (kernel+user) CPU time as of the last getTreeCPUUtilization() call. See the Linux
		 * treeLastActiveMs member for why this is pruned to the current tree on every call. */
		std::unordered_map<int64_t, ULONGLONG> treeLastActiveCpu;

		unsigned getCPUUtilization();
#elif __APPLE__
		pid_t pid; /**< The process identifier of the tracked process. */
		size_t lastIdle = 0;
		size_t lastTotal = 0;
		size_t lastProcActiveMs = 0;
		std::chrono::steady_clock::time_point lastProcTime{};
		/** @brief Per-PID active-CPU-ms as of the last getTreeCPUUtilization() call. See the Linux treeLastActiveMs
		 * member for why this is pruned to the current tree on every call. */
		std::unordered_map<int64_t, size_t> treeLastActiveMs;

		unsigned getCPUUtilization();
#endif

	public:
		explicit SystemStats(const TrackingTarget& target);

		std::set<tirexMeasure> providedMeasures() noexcept override;
		void start() override;
		void stop() override;
		void step() override;
		Stats getStats() override;
		Stats getInfo() override;

		static constexpr const char* description = "Collects system components and utilization metrics.";
		static const char* version;
		static const std::set<tirexMeasure> measures;

	private:
		static SysInfo getSysInfo();
		static CPUInfo getCPUInfo();
	};
} // namespace tirex

#endif