/**
 * @file ioreportstats.cpp
 * @brief Implements IOReportStats; a macOS-only provider for energy metrics via IOReport.
 * @details The implementation in this file is based in large parts on the description by
 * https://medium.com/@vladkens/how-to-get-macos-power-metrics-with-rust-d42b0ad53967
 */

/** \todo On macOS, ioreport could report CPU frequency */

#include "ioreportstats.hpp"

#include "../../logging.hpp"

using tirex::IOReportStats;
using tirex::Stats;

const char* IOReportStats::version = nullptr;

#if !defined(__APPLE__)

// IOReport only exists on macOS; on other devices, we "disable" it by setting "providedMeasures" to empty.
const std::set<tirexMeasure> IOReportStats::measures{};

IOReportStats::IOReportStats() = default;
IOReportStats::~IOReportStats() = default;

std::set<tirexMeasure> IOReportStats::providedMeasures() noexcept { return {}; }
void IOReportStats::start() {}
void IOReportStats::stop() {}
Stats IOReportStats::getStats() { return {}; }

#else

#if __cpp_lib_format and __cplusplus >= 202207L // __cplusplus >= 202207L required for std::format_string
#include <format>
namespace _fmt = std;
#else
#include <fmt/core.h>
namespace _fmt = fmt;
#endif

#include <CoreFoundation/CoreFoundation.h>
#include <chrono>
#include <cstring>

#include "./details/macos/ioreport.h"

const std::set<tirexMeasure> IOReportStats::measures{
		TIREX_CPU_ENERGY_SYSTEM_JOULES, TIREX_RAM_ENERGY_SYSTEM_JOULES, TIREX_GPU_ENERGY_SYSTEM_JOULES
};

/**
 * @brief Translates the CFStringRef into a std::string.
 */
static std::string cfToString(CFStringRef s) {
	if (!s)
		return {};
	// Fast path: direct UTF-8 pointer (avoids allocation when possible)
	if (const char* ptr = CFStringGetCStringPtr(s, kCFStringEncodingUTF8))
		return ptr;
	CFIndex maxLen = CFStringGetMaximumSizeForEncoding(CFStringGetLength(s), kCFStringEncodingUTF8) + 1;
	std::string buf(maxLen, '\0');
	CFStringGetCString(s, buf.data(), maxLen, kCFStringEncodingUTF8);
	buf.resize(std::strlen(buf.c_str()));
	return buf;
}

/**
 * @brief Converts a raw IOReport energy value to Joules using the channel's unit label.
 * @param unit  The unit string from IOReportChannelGetUnitLabel (e.g. "mJ", "uJ", "nJ").
 * @param value The raw integer value from IOReportSimpleGetIntegerValue.
 * @return      Energy in Joules, or nullopt if the unit is unrecognised.
 */
static std::optional<double> unitToJoules(const std::string& unit, double value) {
	if (unit == "mJ")
		return value / 1e3;
	if (unit == "uJ")
		return value / 1e6;
	if (unit == "nJ")
		return value / 1e9;
	tirex::log::warn("ioreportstats", "Unrecognised energy unit '{}'", unit);
	tirex::abort(tirexLogLevel::WARN, _fmt::format("Unrecognised energy unit '{}'", unit).c_str());
	return std::nullopt;
}

struct tirex::IOReportStats::State {
	IOReportLib ioreport;
	CFMutableDictionaryRef channels = nullptr;
	IOReportSubscriptionRef subscription = nullptr;
	CFDictionaryRef sample1 = nullptr;
	CFDictionaryRef sample2 = nullptr;
	std::chrono::steady_clock::time_point startTime;
	std::chrono::steady_clock::time_point stopTime;

	~State() {
		// Note: IOReportSubscriptionRef is a plain struct pointer, not a CFType, so no CFRelease necessary.
		if (sample2)
			CFRelease(sample2);
		if (sample1)
			CFRelease(sample1);
		if (channels)
			CFRelease(channels);
	}
};

IOReportStats::IOReportStats() = default;
IOReportStats::~IOReportStats() = default;

std::set<tirexMeasure> IOReportStats::providedMeasures() noexcept {
	IOReportLib probe;
	if (!probe.good()) {
		tirex::log::warn("ioreportstats", "libIOReport.dylib not found");
		tirex::abort(tirexLogLevel::WARN, "libIOReport.dylib not found");
		return {};
	}
	return {TIREX_CPU_ENERGY_SYSTEM_JOULES, TIREX_RAM_ENERGY_SYSTEM_JOULES, TIREX_GPU_ENERGY_SYSTEM_JOULES};
}

void IOReportStats::start() {
	state = std::make_unique<State>();
	auto& s = *state;

	if (!s.ioreport.good())
		return;

	auto groupStr = CFStringCreateWithCString(kCFAllocatorDefault, "Energy Model", kCFStringEncodingASCII);
	auto rawChannels = s.ioreport.copyChannelsInGroup(groupStr, nullptr, 0, 0, 0);
	CFRelease(groupStr);

	if (!rawChannels) {
		tirex::log::warn("ioreportstats", "No Energy Model channels found");
		tirex::abort(tirexLogLevel::WARN, "No Energy Model channels found");
		return;
	}

	s.channels = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, CFDictionaryGetCount(rawChannels), rawChannels);
	CFRelease(rawChannels);

	CFMutableDictionaryRef subDict = nullptr;
	s.subscription = s.ioreport.createSubscription(nullptr, s.channels, &subDict, 0, nullptr);
	if (subDict)
		CFRelease(subDict); // output parameter; we only need the subscription handle
	if (!s.subscription) {
		tirex::log::warn("ioreportstats", "Failed to create subscription");
		tirex::abort(tirexLogLevel::WARN, "Failed to create subscription");
		return;
	}

	s.sample1 = s.ioreport.createSamples(s.subscription, s.channels, nullptr);
	s.startTime = std::chrono::steady_clock::now();
}

void IOReportStats::stop() {
	if (!state || !state->subscription || !state->sample1)
		return;
	auto& s = *state;
	s.sample2 = s.ioreport.createSamples(s.subscription, s.channels, nullptr);
	s.stopTime = std::chrono::steady_clock::now();
}

Stats IOReportStats::getStats() {
	using json = nlohmann::json;

	auto nullStats = [this]() {
		return makeFilteredStats(
				enabled, std::pair{TIREX_CPU_ENERGY_SYSTEM_JOULES, json(nullptr)},
				std::pair{TIREX_RAM_ENERGY_SYSTEM_JOULES, json(nullptr)},
				std::pair{TIREX_GPU_ENERGY_SYSTEM_JOULES, json(nullptr)}
		);
	};

	if (!state || !state->sample1 || !state->sample2)
		return nullStats();

	auto& s = *state;
	auto delta = s.ioreport.createSamplesDelta(s.sample1, s.sample2, nullptr);
	if (!delta)
		return nullStats();

	double cpuJ = 0, gpuJ = 0, dramJ = 0;
	bool hasCPU = false, hasGPU = false, hasDRAM = false;

	auto ioKey = CFStringCreateWithCString(kCFAllocatorDefault, "IOReportChannels", kCFStringEncodingASCII);
	auto channelsArr = (CFArrayRef)CFDictionaryGetValue(delta, ioKey);
	CFRelease(ioKey);

	if (channelsArr) {
		for (CFIndex i = 0; i < CFArrayGetCount(channelsArr); ++i) {
			auto entry = (CFDictionaryRef)CFArrayGetValueAtIndex(channelsArr, i);
			auto name = cfToString(s.ioreport.channelGetChannelName(entry));
			auto unit = cfToString(s.ioreport.channelGetUnitLabel(entry));
			auto value = static_cast<double>(s.ioreport.simpleGetIntegerValue(entry, 0));

			tirex::log::debug("ioreportstats", "channel='{}' unit='{}' value={}", name, unit, value);

			auto joules = unitToJoules(unit, value);
			if (!joules)
				continue;

			// Typical Apple Silicon channel names:
			//   "CPU Energy", "CPU 0 Energy", "GPU 0 Energy", "DRAM Energy", "ANE Energy"
			auto contains = [&name](const char* s) { return name.find(s) != std::string::npos; };

			if (contains("CPU")) {
				cpuJ += *joules;
				hasCPU = true;
			} else if (contains("GPU")) {
				gpuJ += *joules;
				hasGPU = true;
			} else if (contains("DRAM")) {
				dramJ += *joules;
				hasDRAM = true;
			}
			/** \todo support Apple Neural Engine (ANE)? */
		}
	}

	CFRelease(delta);

	std::optional<double> optCpuJ = hasCPU ? std::make_optional(cpuJ) : std::nullopt;
	std::optional<double> optGpuJ = hasGPU ? std::make_optional(gpuJ) : std::nullopt;
	std::optional<double> optDramJ = hasDRAM ? std::make_optional(dramJ) : std::nullopt;

	return makeFilteredStats(
			enabled, std::pair{TIREX_CPU_ENERGY_SYSTEM_JOULES, json(optCpuJ)},
			std::pair{TIREX_RAM_ENERGY_SYSTEM_JOULES, json(optDramJ)},
			std::pair{TIREX_GPU_ENERGY_SYSTEM_JOULES, json(optGpuJ)}
	);
}

#endif
