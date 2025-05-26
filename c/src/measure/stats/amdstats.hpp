#ifndef STATS_AMDSTATS_HPP
#define STATS_AMDSTATS_HPP

#include "provider.hpp"

namespace tirex {

	class AMDStats final : public StatsProvider {
	private:
	public:
		AMDStats();

		std::set<tirexMeasure> providedMeasures() noexcept override;
		void step() override;
		Stats getStats() override;
		Stats getInfo() override;

		static constexpr const char* description = "Collects CPU and GPU related metrics for AMD CPUs and GPUs.";
		static const char* version;
		static const std::set<tirexMeasure> measures;
	};

} // namespace tirex

#endif