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
			std::string virtualization;
			std::string endianness;
			uint64_t frequency;
			std::string flags;
		};

	private:
		std::chrono::steady_clock::time_point starttime;
		std::chrono::steady_clock::time_point stoptime;

		msr::TimeSeries<unsigned> ram;
		msr::TimeSeries<unsigned> sysCpuUtil;
		msr::TimeSeries<unsigned> sysRam;

#if __linux__
		size_t startUTime;
		size_t startSysTime;

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
		SystemStats() = default;

		void start() override;
		void stop() override;
		void step() override;
		Stats getStats() override;

		static constexpr const char* description = "Collects system components and utilization metrics.";
		static const char* version;
		static const std::set<msrMeasure> measures;

	private:
		static CPUInfo getCPUInfo();
	};
} // namespace msr

#endif