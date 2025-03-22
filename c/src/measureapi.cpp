#include <tirex_tracker.h>

#include "logging.hpp"
#include "measure/stats/provider.hpp"
#include "measure/utils/rangeutils.hpp"

#include <cassert>
#include <cstring>
#include <future>
#include <iostream>
#include <ranges>
#include <sstream>
#include <thread>
#include <vector>

struct tirexMeasureHandle_st final {
	size_t pollIntervalMs;
	const std::vector<std::unique_ptr<tirex::StatsProvider>> providers;
	std::thread monitorthread;
	std::promise<void> signal;

	tirexMeasureHandle_st(tirexMeasureHandle_st& other) = delete;

	explicit tirexMeasureHandle_st(
			std::vector<std::unique_ptr<tirex::StatsProvider>>&& _providers, size_t pollIntervalMs
	) noexcept
			: pollIntervalMs(pollIntervalMs), providers(std::move(_providers)) {
		// Start measuring
		tirex::log::info("measure", "Start Measuring");
		for (auto& provider : providers)
			provider->start();

		monitorthread = std::thread(tirexMeasureHandle_st::monitorThread, this);
	}

	tirex::Stats stop() {
		signal.set_value();
		monitorthread.join();

		// Stop measuring
		for (auto& provider : providers | std::views::reverse)
			provider->stop();

		// Collect statistics and print them
		tirex::Stats stats{};
		for (auto& provider : providers)
			stats.merge(provider->getStats());
		return stats;
	}

	static void monitorThread(tirexMeasureHandle_st* self) {
		auto future = self->signal.get_future();
		std::chrono::milliseconds intervall{self->pollIntervalMs};
		while (future.wait_for(intervall) != std::future_status::ready) {
			for (auto& provider : self->providers)
				provider->step();
		}
	}
};

static tirexError
initProviders(const tirexMeasureConf* measures, std::vector<std::unique_ptr<tirex::StatsProvider>>& providers) {
	std::set<tirexMeasure> tirexset;
	for (auto conf = measures; conf->source != tirexMeasure::TIREX_MEASURE_INVALID; ++conf) {
		auto [it, inserted] = tirexset.insert(conf->source);
		if (!inserted) {
			/** \todo if pedantic abort here **/
			tirex::log::warn(
					"measure", "The measure {} was requested more than once", static_cast<signed>(conf->source)
			);
		}
	}
	auto unmatched = tirex::initProviders(tirexset, providers);
	if (!unmatched.empty()) {
		/** \todo if pedantic abort here **/
		tirex::log::warn("measure", "Not all requested measures are associated with a data provider");
		tirex::log::warn("measure", "Unmatched: {}", tirex::utils::join(unmatched));
	}
	return TIREX_SUCCESS;
}

tirexError tirexFetchInfo(const tirexMeasureConf* measures, tirexResult** result) {
	std::vector<std::unique_ptr<tirex::StatsProvider>> providers;
	if (tirexError err; (err = initProviders(measures, providers)) != TIREX_SUCCESS)
		return err;
	tirex::Stats stats{};
	for (auto& provider : providers)
		stats.merge(provider->getInfo());
	*result = createMsrResultFromStats(std::move(stats));
	return TIREX_SUCCESS;
}

tirexError tirexStartTracking(const tirexMeasureConf* measures, size_t pollIntervalMs, tirexMeasureHandle** handle) {
	std::vector<std::unique_ptr<tirex::StatsProvider>> providers;
	if (tirexError err; (err = initProviders(measures, providers)) != TIREX_SUCCESS)
		return err;
	*handle = new tirexMeasureHandle{std::move(providers), pollIntervalMs};
	return TIREX_SUCCESS;
}

tirexError tirexStopTracking(tirexMeasureHandle* measure, tirexResult** result) {
	if (measure == nullptr)
		return TIREX_INVALID_ARGUMENT;
	auto res = measure->stop();
	delete measure;
	*result = createMsrResultFromStats(std::move(res));
	return TIREX_SUCCESS;
}
