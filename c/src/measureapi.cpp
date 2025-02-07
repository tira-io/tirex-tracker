#include <measure.h>

#include "logging.hpp"
#include "measure/stats/provider.hpp"

#include <cassert>
#include <cstring>
#include <future>
#include <iostream>
#include <ranges>
#include <sstream>
#include <thread>
#include <vector>

struct msrMeasureHandle_st final {
	size_t pollIntervalMs;
	const std::vector<std::unique_ptr<msr::StatsProvider>> providers;
	std::thread monitorthread;
	std::promise<void> signal;

	explicit msrMeasureHandle_st(
			std::vector<std::unique_ptr<msr::StatsProvider>>&& _providers, size_t pollIntervallMs
	) noexcept
			: providers(std::move(_providers)), pollIntervalMs(pollIntervalMs) {
		// Start measuring
		msr::log::info("measure", "Start Measuring");
		for (auto& provider : providers)
			provider->start();

		monitorthread = std::thread(msrMeasureHandle_st::monitorThread, this);
	}

	msr::Stats stop() {
		signal.set_value();
		monitorthread.join();

		// Stop measuring
		for (auto& provider : providers | std::views::reverse)
			provider->stop();

		// Collect statistics and print them
		msr::Stats stats{}; /** \todo ranges **/
		for (auto& provider : providers) {
			auto tmp = provider->getStats().value;
			stats.value.insert(stats.value.end(), tmp.begin(), tmp.end());
		}
		return stats;
	}

	static void monitorThread(msrMeasureHandle_st* self) {
		auto future = self->signal.get_future();
		auto intervall = std::chrono::milliseconds{self->pollIntervalMs};
		while (future.wait_for(intervall) != std::future_status::ready) {
			for (auto& provider : self->providers)
				provider->step();
		}
	}
};

msrError msrFetchInfo(const msrMeasureConf* measures, msrResult** result) {
	*result = new msrResult({{msrMeasure::MSR_OS_NAME, "msrFetchInfo is not implemented yet :("}});
	/** \todo implement **/
	return MSR_SUCCESS;
}

msrError msrStartMeasure(const msrMeasureConf* measures, size_t pollIntervalMs, msrMeasureHandle** handle) {
	std::vector<std::unique_ptr<msr::StatsProvider>> providers;
	std::set<msrMeasure> msrset;
	for (auto conf = measures; conf->source != msrMeasure::MSR_MEASURE_INVALID; ++conf) {
		auto [it, inserted] = msrset.insert(conf->source); /** \todo implement **/
		if (!inserted) {
			/** \todo if pedantic abort here **/
			msr::log::warn("measure", "The measure {} was requested more than once", static_cast<signed>(conf->source));
		}
	}
	auto unmatched = msr::initProviders(msrset, providers);
	if (!unmatched.empty()) {
		/** \todo if pedantic abort here **/
		/** \todo log which are not associated **/
		msr::log::warn("measure", "Not all requested measures are associated with a data provider");
	}
	*handle = new msrMeasureHandle{std::move(providers), pollIntervalMs};
	return MSR_SUCCESS;
}

msrError msrStopMeasure(msrMeasureHandle* measure, msrResult** result) {
	if (measure == nullptr)
		return MSR_INVALID_ARGUMENT;
	auto res = measure->stop();
	delete measure;
	*result = new msr::Stats(std::move(res));
	return MSR_SUCCESS;
}
