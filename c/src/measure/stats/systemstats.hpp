#ifndef STATS_SYSTEMSTATS_HPP
#define STATS_SYSTEMSTATS_HPP

#include "../timeseries.hpp"
#include "provider.hpp"

#include <chrono>
#include <string>
#include <tuple>
#include <vector>

#if defined(_WINDOWS) || defined(_WIN32) || defined(WIN32)
#include <windows.h>
#undef ERROR //  Make problems with logging.h otherwise
#endif

namespace tirex {
	using namespace std::chrono_literals;

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

		tirex::TimeSeries<unsigned> ram = ts::store<unsigned>() | ts::Limit(300, TIREX_AGG_MAX) |
										  ts::Batched(100ms, TIREX_AGG_MAX, 300); /** \todo make agg configurable */
		tirex::TimeSeries<unsigned> sysRam = ts::store<unsigned>() | ts::Limit(300, TIREX_AGG_MAX) |
											 ts::Batched(100ms, TIREX_AGG_MAX, 300); /** \todo make agg configurable */
		tirex::TimeSeries<unsigned> cpuUtil =
				ts::store<unsigned>() | ts::Limit(300, TIREX_AGG_MEAN) |
				ts::Batched(100ms, TIREX_AGG_MEAN, 300); /** \todo make agg configurable */
		tirex::TimeSeries<unsigned> sysCpuUtil =
				ts::store<unsigned>() | ts::Limit(300, TIREX_AGG_MEAN) |
				ts::Batched(100ms, TIREX_AGG_MEAN, 300); /** \todo make agg configurable */
		tirex::TimeSeries<uint32_t> frequency =
				ts::store<unsigned>() | ts::Limit(300, TIREX_AGG_MAX) |
				ts::Batched(100ms, TIREX_AGG_MAX, 300); /** \todo make agg configurable */

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

		uint8_t getProcCPUUtilization();

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

		void parseMemInfo(Utilization& utilization);
		void parseStat(Utilization& utilization);
		void parseStatm(pid_t pid, Utilization& utilization);
#elif defined(_WINDOWS) || defined(_WIN32) || defined(WIN32)
		HANDLE pid; /**< The process identifier of the tracked process. */
		FILETIME prevSysIdle, prevSysKernel, prevSysUser;
		ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
		unsigned numProcessors;

		uint8_t getCPUUtilization();
#elif __APPLE__
		pid_t pid; /**< The process identifier of the tracked process. */
		size_t lastIdle = 0;
		size_t lastTotal = 0;
		size_t lastProcActiveMs = 0;
		std::chrono::steady_clock::time_point lastProcTime{};

		uint8_t getCPUUtilization();
#endif

	public:
		SystemStats();

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