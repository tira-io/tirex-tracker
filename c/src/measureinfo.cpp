#include <measure.h>

#include <ranges>

#include "measure/stats/provider.hpp"

size_t msrDataProviderGetAll(msrDataProvider* buf, size_t bufsize) {
	if (buf != nullptr) {
		for (const auto& [key, value] : msr::providers | std::views::take(bufsize)) {
			*buf = {.name = key.c_str(), .description = value.description, .version = value.version};
			++buf;
		}
	}
	return msr::providers.size();
}

static const msrMeasureInfo measureInfos[]{
		[MSR_OS_NAME] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_OS_KERNEL] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_TIME_ELAPSED_WALL_CLOCK_MS] =
				{.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_TIME_ELAPSED_USER_MS] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_TIME_ELAPSED_SYSTEM_MS] =
				{.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_CPU_USED_PROCESS_PERCENT] =
				{.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_CPU_USED_SYSTEM_PERCENT] =
				{.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_CPU_AVAILABLE_SYSTEM_CORES] =
				{.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_CPU_ENERGY_SYSTEM_JOULES] =
				{.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_CPU_FEATURES] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_CPU_FREQUENCY_MHZ] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_CPU_FREQUENCY_MIN_MHZ] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_CPU_FREQUENCY_MAX_MHZ] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_CPU_VENDOR_ID] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_CPU_BYTE_ORDER] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_CPU_ARCHITECTURE] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_CPU_MODEL_NAME] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_CPU_CORES_PER_SOCKET] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_CPU_THREADS_PER_CORE] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_CPU_CACHES_L1_KB] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_CPU_CACHES_L2_KB] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_CPU_CACHES_L3_KB] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_CPU_VIRTUALIZATION] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_CPU_BOGO_MIPS] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_RAM_USED_PROCESS_KB] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_RAM_USED_SYSTEM_MB] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_RAM_AVAILABLE_SYSTEM_MB] =
				{.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_RAM_ENERGY_SYSTEM_JOULES] =
				{.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_GPU_SUPPORTED] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_GPU_MODEL_NAME] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_GPU_NUM_CORES] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_GPU_USED_PROCESS_PERCENT] =
				{.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_GPU_USED_SYSTEM_PERCENT] =
				{.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_GPU_AVAILABLE_SYTEM_CORES] =
				{.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_GPU_VRAM_USED_PROCESS_MB] =
				{.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_GPU_VRAM_USED_SYSTEM_MB] =
				{.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_GPU_VRAM_AVAILABLE_SYSTEM_MB] =
				{.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_GPU_ENERGY_SYSTEM_JOULES] =
				{.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_GIT_IS_REPO] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_GIT_HASH] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_GIT_LAST_COMMIT_HASH] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_GIT_BRANCH] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_GIT_TAGS] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_GIT_REMOTE_ORIGIN] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_GIT_UNCOMMITTED_CHANGES] =
				{.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_GIT_UNPUSHED_CHANGES] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_GIT_UNCHECKED_FILES] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
};

msrError msrMeasureInfoGet(msrMeasure measure, const msrMeasureInfo** info) {
	if (measure < 0 || measure >= msrMeasure::MSR_MEASURE_COUNT)
		return msrError::MSR_INVALID_ARGUMENT;
	*info = &measureInfos[measure];
	return msrError::MSR_SUCCESS;
}