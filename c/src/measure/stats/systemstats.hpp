#ifndef STATS_SYSTEMSTATS_HPP
#define STATS_SYSTEMSTATS_HPP

#include "../measure.hpp"
#include "provider.hpp"

#include <chrono>
#include <vector>

namespace msr {
	class SystemStats final : public StatsProvider {
	public:
		struct CPUInfo {
			struct Cache {
				unsigned unified;  /**< LX cache size in byte **/
				unsigned instruct; /**< LXi cache size in byte **/
				unsigned data;	   /**< LXd cache size in byte **/
			};
			std::string modelname;
			std::string vendorId;
			unsigned numCores;
			unsigned coresPerSocket;
			unsigned threadsPerCore;
			std::vector<Cache> caches;
			std::string endianness;
			uint32_t frequency_min;
			uint32_t frequency_max;
			std::string flags;
			struct VirtFlags {
				bool svm; /**< AMD-V support Ü*/
				bool vmx; /**< VT-x support **/
			} virtualization;
		};

	private:
		std::chrono::steady_clock::time_point starttime;
		std::chrono::steady_clock::time_point stoptime;

		msr::TimeSeries<unsigned> ram{true};
		msr::TimeSeries<unsigned> sysCpuUtil{true};
		msr::TimeSeries<unsigned> sysRam{true};
		msr::TimeSeries<uint32_t> frequency{true};

#if __linux__
		size_t startUTime, stopUTime;
		size_t startSysTime, stopSysTime;

		struct Utilization;

		size_t lastIdle;
		size_t lastTotal;

		Utilization getUtilization();
		void parseMemInfo(Utilization& utilization);
		void parseStat(Utilization& utilization);
		void parseStatm(pid_t pid, Utilization& utilization);
		void parseStat(pid_t pid, Utilization& utilization);
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
		static CPUInfo getCPUInfo();
	};
} // namespace msr

#endif