#ifndef STATS_MACOS_IOREPORT_H
#define STATS_MACOS_IOREPORT_H

#include "../../utils/sharedlib.hpp"

#include <inttypes.h>

#include <CoreFoundation/CoreFoundation.h>

struct ioreportsub;
typedef ioreportsub* IOReportSubscriptionRef;

struct IOReportLib final : tirex::utils::SharedLib {
	using COPY_ALL_CHANNELS = CFDictionaryRef (*)(uint64_t a, uint64_t b);
	COPY_ALL_CHANNELS copyAllChannels = load<COPY_ALL_CHANNELS>({"IOReportCopyAllChannels"});
	using COPY_CHANNELS_IN_GROUP =
			CFDictionaryRef (*)(CFStringRef a, CFStringRef b, uint64_t c, uint64_t d, uint64_t e);
	COPY_CHANNELS_IN_GROUP copyChannelsInGroup = load<COPY_CHANNELS_IN_GROUP>({"IOReportCopyChannelsInGroup"});
	using MERGE_CHANNELS = void (*)(CFDictionaryRef a, CFDictionaryRef b, CFTypeRef c);
	MERGE_CHANNELS mergeChannels = load<MERGE_CHANNELS>({"IOReportMergeChannels"});
	using CREATE_SUBSCRIPTION = IOReportSubscriptionRef (*)(
			const void* a, CFMutableDictionaryRef b, CFMutableDictionaryRef c, uint64_t d, CFTypeRef e
	);
	CREATE_SUBSCRIPTION createSubscription = load<CREATE_SUBSCRIPTION>({"IOReportCreateSubscription"});
	using CREATE_SAMPLES = CFDictionaryRef (*)(IOReportSubscriptionRef a, CFMutableDictionaryRef b, CFTypeRef c);
	CREATE_SAMPLES createSamples = load<CREATE_SAMPLES>({"IOReportCreateSamples"});
	using CREATE_SAMPLES_DELTA = CFDictionaryRef (*)(CFDictionaryRef a, CFDictionaryRef b, CFTypeRef c);
	CREATE_SAMPLES_DELTA createSamplesDelta = load<CREATE_SAMPLES_DELTA>({"IOReportCreateSamplesDelta"});
	using CHANNEL_GET_GROUP = CFStringRef (*)(CFDictionaryRef a);
	CHANNEL_GET_GROUP channelGetGroup = load<CHANNEL_GET_GROUP>({"IOReportChannelGetGroup"});
	using CHANNEL_GET_SUB_GROUP = CFStringRef (*)(CFDictionaryRef a);
	CHANNEL_GET_SUB_GROUP channelGetSubGroup = load<CHANNEL_GET_SUB_GROUP>({"IOReportChannelGetSubGroup"});
	using CHANNEL_GET_CHANNEL_NAME = CFStringRef (*)(CFDictionaryRef a);
	CHANNEL_GET_CHANNEL_NAME channelGetChannelName = load<CHANNEL_GET_CHANNEL_NAME>({"IOReportChannelGetChannelName"});
	using SIMPLE_GET_INTEGER_VALUE = int64_t (*)(CFDictionaryRef a, int32_t b);
	SIMPLE_GET_INTEGER_VALUE simpleGetIntegerValue = load<SIMPLE_GET_INTEGER_VALUE>({"IOReportSimpleGetIntegerValue"});
	using CHANNEL_GET_UNIT_LABEL = CFStringRef (*)(CFDictionaryRef a);
	CHANNEL_GET_UNIT_LABEL channelGetUnitLabel = load<CHANNEL_GET_UNIT_LABEL>({"IOReportChannelGetUnitLabel"});
	using STATE_GET_COUNT = int32_t (*)(CFDictionaryRef a);
	STATE_GET_COUNT stateGetCount = load<STATE_GET_COUNT>({"IOReportStateGetCount"});
	using STATE_GET_NAME_FOR_INDEX = CFStringRef (*)(CFDictionaryRef a, int32_t b);
	STATE_GET_NAME_FOR_INDEX stateGetNameForIndex = load<STATE_GET_NAME_FOR_INDEX>({"IOReportStateGetNameForIndex"});
	using STATE_GET_RESIDENCY = int64_t (*)(CFDictionaryRef a, int32_t b);
	STATE_GET_RESIDENCY stateGetResidency = load<STATE_GET_RESIDENCY>({"IOReportStateGetResidency"});

#ifdef __APPLE__
	IOReportLib() : tirex::utils::SharedLib("/usr/lib/libIOReport.dylib") {}
#else
	IOReportLib() : tirex::utils::SharedLib() {}
#endif
};

#endif