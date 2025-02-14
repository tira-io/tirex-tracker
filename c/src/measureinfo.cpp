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
				{.description = "The (\"real\") wall clock time in milliseconds elapsed during the measurement.",
				 .datatype = msrResultType::MSR_STRING,
				 .example = "1234"},
		[MSR_TIME_ELAPSED_USER_MS] =
				{.description = "Time spent in the platform's user mode.",
				 .datatype = msrResultType::MSR_STRING,
				 .example = "1234"},
		[MSR_TIME_ELAPSED_SYSTEM_MS] =
				{.description = "Time spent in the platform's system mode.",
				 .datatype = msrResultType::MSR_STRING,
				 .example = "1234"},
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
		[MSR_CPU_VENDOR_ID] =
				{.description = "A humand legible name for the vendor of the CPU.",
				 .datatype = msrResultType::MSR_STRING,
				 .example = "Intel Corporation"},
		[MSR_CPU_BYTE_ORDER] =
				{.description = "The endianness (big-, little-, mixed-endian) used by the CPU.",
				 .datatype = msrResultType::MSR_STRING,
				 .example = "Little Endian"},
		[MSR_CPU_ARCHITECTURE] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_CPU_MODEL_NAME] =
				{.description = "The name of the concrete CPU model.",
				 .datatype = msrResultType::MSR_STRING,
				 .example = "Intel(R) Core(TM)2 Quad  CPU   Q8200  @ 2.33GHz"},
		[MSR_CPU_CORES_PER_SOCKET] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_CPU_THREADS_PER_CORE] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_CPU_CACHES_KB] =
				{.description = "Collects a dictionary which contains the size of each CPU cache (in kilobytes).",
				 .datatype = msrResultType::MSR_STRING,
				 .example = "{\"l1\": \"TODO KB\", \"l1d\": \"TODO KB\" \"l2\": \"TODO KB\", \"l3\": \"TODO KB\"}"},
		[MSR_CPU_VIRTUALIZATION] =
				{.description = "The virtualization technology supported by the CPU (e.g., VT-x or AMD-V), if any.",
				 .datatype = msrResultType::MSR_STRING,
				 .example = "VT-x"},
		[MSR_RAM_USED_PROCESS_KB] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_RAM_USED_SYSTEM_MB] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_RAM_AVAILABLE_SYSTEM_MB] =
				{.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_RAM_ENERGY_SYSTEM_JOULES] =
				{.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_GPU_SUPPORTED] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_GPU_MODEL_NAME] = {.description = "TODO", .datatype = msrResultType::MSR_STRING, .example = "TODO"},
		[MSR_GPU_NUM_CORES] =
				{.description = "Number of GPU cores available in the system.",
				 .datatype = msrResultType::MSR_STRING,
				 .example = "TODO"},
		[MSR_GPU_USED_PROCESS_PERCENT] =
				{.description = "GPU usage of the measured process in percent.",
				 .datatype = msrResultType::MSR_STRING,
				 .example = "TODO"},
		[MSR_GPU_USED_SYSTEM_PERCENT] =
				{.description = "GPU utilization of the entire system in percent.",
				 .datatype = msrResultType::MSR_STRING,
				 .example = "TODO"},
		[MSR_GPU_VRAM_USED_PROCESS_MB] =
				{.description = "GPU VRAM usage of the measured process in megabyte.",
				 .datatype = msrResultType::MSR_STRING,
				 .example = "TODO"},
		[MSR_GPU_VRAM_USED_SYSTEM_MB] =
				{.description = "GPU VRAM usage of the entire system in megabytes.",
				 .datatype = msrResultType::MSR_STRING,
				 .example = "TODO"},
		[MSR_GPU_VRAM_AVAILABLE_SYSTEM_MB] =
				{.description = "Amount of GPU VRAM available in the system in megabytes.",
				 .datatype = msrResultType::MSR_STRING,
				 .example = "TODO"},
		[MSR_GPU_ENERGY_SYSTEM_JOULES] =
				{.description = "The energy consumed by the GPU for the entire system in joules.",
				 .datatype = msrResultType::MSR_STRING,
				 .example = "TODO"},
		[MSR_GIT_IS_REPO] =
				{.description = "1 if the current working directory is (part of) a Git repository; 0 otherwise",
				 .datatype = msrResultType::MSR_STRING,
				 .example = "1"},
		[MSR_GIT_HASH] =
				{.description = "SHA1 hash of all files checked into the repository.",
				 .datatype = msrResultType::MSR_STRING,
				 .example = "aa5fba7feff8605c3b253b46fc86d7ac1732a586"},
		[MSR_GIT_LAST_COMMIT_HASH] =
				{.description = "Latest Git commit SHA1 hash.",
				 .datatype = msrResultType::MSR_STRING,
				 .example = "ff52eaf7c0291edbba93c87917e555c720267740"},
		[MSR_GIT_BRANCH] =
				{.description = "Checked-out Git branch name.", .datatype = msrResultType::MSR_STRING, .example = "main"
				},
		[MSR_GIT_BRANCH_UPSTREAM] =
				{.description = "Upstream branch of the checked-out Git branch name.",
				 .datatype = msrResultType::MSR_STRING,
				 .example = "main"},
		[MSR_GIT_TAGS] =
				{.description = "ist of Git tag(s) at the current commit, if any.",
				 .datatype = msrResultType::MSR_STRING,
				 .example = "[\"1.0.0\"]"},
		[MSR_GIT_REMOTE_ORIGIN] =
				{.description = "URL of the `origin` remote if it is set.",
				 .datatype = msrResultType::MSR_STRING,
				 .example = "git@github.com:tira-io/measure.git"},
		[MSR_GIT_UNCOMMITTED_CHANGES] =
				{.description = "1 if some changes are not yet committed and 0 otherwise",
				 .datatype = msrResultType::MSR_STRING,
				 .example = "1"},
		[MSR_GIT_UNPUSHED_CHANGES] =
				{.description = "1 if some changes are not yet pushed and 0 otherwise.",
				 .datatype = msrResultType::MSR_STRING,
				 .example = "0"},
		[MSR_GIT_UNCHECKED_FILES] =
				{.description = "1 if there are files that are not ignored (by a .gitignore file) and also not checked "
								"into the repository; 0 otherwise.",
				 .datatype = msrResultType::MSR_STRING,
				 .example = "1"},
};

msrError msrMeasureInfoGet(msrMeasure measure, const msrMeasureInfo** info) {
	if (measure < 0 || measure >= msrMeasure::MSR_MEASURE_COUNT)
		return msrError::MSR_INVALID_ARGUMENT;
	*info = &measureInfos[measure];
	return msrError::MSR_SUCCESS;
}