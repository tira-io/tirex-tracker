#include <tirex_tracker.h>

#include "measure/stats/provider.hpp"

size_t tirexDataProviderGetAll(tirexDataProvider* buf, size_t bufsize) {
	if (buf != nullptr) {
		// When ranges support can be assumed:
		// for (const auto& [key, value] : tirex::providers | std::views::take(bufsize)) {
		size_t i = 0;
		for (const auto& [key, value] : tirex::providers) {
			if (i++ >= bufsize)
				break;
			*buf = {.name = key.c_str(), .description = value.description, .version = value.version};
			++buf;
		}
	}
	return tirex::providers.size();
}

static const tirexMeasureInfo measureInfos[]{
		// OS
		/*[TIREX_OS_NAME] = */
		{.description = "Name and version of the operating system under which is currently running.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "Fedora Linux 41 (Workstation Edition)"},
		/*[TIREX_OS_KERNEL] = */
		{.description = "The version of the kernel that the operating system is running on.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "Linux 6.12.8-200.fc41.x86_64 x86_64"},
		// Time
		/*[TIREX_TIME_START] = */
		{.description = "Timestamp when the tracking was started.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "2025-04-17T08:00:50.996022428+0000"},
		/*[TIREX_TIME_STOP] = */
		{.description = "Timestamp when the tracking was stopped.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "2025-04-17T08:00:55.666974375+0000"},
		/*[TIREX_TIME_ELAPSED_WALL_CLOCK_MS] = */
		{.description = "The (\"real\") wall clock time in milliseconds elapsed during tracking.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "1234"},
		/*[TIREX_TIME_ELAPSED_USER_MS] = */
		{.description = "Time spent in the platform's user mode.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "1234"},
		/*[TIREX_TIME_ELAPSED_SYSTEM_MS] = */
		{.description = "Time spent in the platform's system mode.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "1234"},
		// CPU
		/*[TIREX_CPU_USED_PROCESS_PERCENT] = */
		{.description = "CPU usage of the tracked process in percent per logical CPU cores.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "{\"max\": 117, \"min\": 0, \"avg\": 0, \"timeseries\": {\"timestamps\": [\"108ms\"], \"values\": "
					"[57]}}"},
		/*[TIREX_CPU_USED_SYSTEM_PERCENT] = */
		{.description = "CPU usage of the entire system in percent per logical CPU cores.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "{\"max\": 17, \"min\": 0, \"avg\": 0, \"timeseries\": {\"timestamps\": [\"108ms\"], \"values\": "
					"[9]}}"},
		/*[TIREX_CPU_AVAILABLE_SYSTEM_CORES] = */
		{.description = "Number of CPU cores available in the system.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "8"},
		/*[TIREX_CPU_ENERGY_SYSTEM_JOULES] = */
		{.description = "The energy consumed by the CPU by the entire system over the tracked period in joules. ",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "2970137"},
		/*[TIREX_CPU_FEATURES] = */
		{.description = "List of hardware features the CPU supports (e.g., the instruction set, encryption "
						"capabilities).",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "rdtsc rdtscp rdpid clzero mwait mwaitx fxsave xsave fpu mmx mmx_plus prefetch prefetchw daz sse "
					"sse2 sse3 ssse3 sse4_1 sse4_2 sse4a misaligned_sse avx fma3 f16c avx2 avx512f avx512cd avx512dq "
					"avx512bw avx512vl avx512ifma avx512vbmi avx512vbmi2 avx512bitalg avx512vpopcntdq avx512vnni "
					"avx512bf16 cmov cmpxchg8b cmpxchg16b clwb movbe lahf_sahf lzcnt popcnt bmi bmi2 adx aes vaes "
					"pclmulqdq vpclmulqdq gfni rdrand rdseed sha"},
		/*[TIREX_CPU_FREQUENCY_MHZ] = */
		{.description = "Current CPU speed in megahertz.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "{\"max\": 0, \"min\": 0, \"avg\": 0, \"timeseries\": {\"timestamps\": [\"108ms\"], \"values\": "
					"[0]}}"},
		/*[TIREX_CPU_FREQUENCY_MIN_MHZ] = */
		{.description = "Minimum possible CPU speed in megahertz.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "400"},
		/*[TIREX_CPU_FREQUENCY_MAX_MHZ] = */
		{.description = "Maximum possible CPU speed in megahertz. ",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "3801"},
		/*[TIREX_CPU_VENDOR_ID] = */
		{.description = "A textual name for the vendor of the CPU.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "Intel Corporation"},
		/*[TIREX_CPU_BYTE_ORDER] = */
		{.description = "The endianness (big-, little-, or mixed-endian) used by the CPU. ",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "Little Endian"},
		/*[TIREX_CPU_ARCHITECTURE] = */
		{.description = "The architecture (x86, x86_64, ARM, ...) of the CPU.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "aarch64"},
		/*[TIREX_CPU_MODEL_NAME] = */
		{.description = "The name of the concrete CPU model.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "Intel(R) Core(TM)2 Quad  CPU   Q8200  @ 2.33GHz"},
		/*[TIREX_CPU_CORES_PER_SOCKET] = */
		{.description = "Number of CPU cores located on a single physical socket.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "8"},
		/*[TIREX_CPU_THREADS_PER_CORE] = */
		{.description = "Number of logical CPU cores (threads) per core.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "2"},
		/*[TIREX_CPU_CACHES] = */
		{.description = "The sizes of each CPU cache (e.g., L1, L2, L3) in kibibytes.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "{\"l1i\": \"384 KiB\",\"l1d\": \"192 KiB\",\"l2\": \"3072 KiB\",\"l3\": \"16384 KiB\"}"},
		/*[TIREX_CPU_VIRTUALIZATION] = */
		{.description = "The virtualization technology supported by the CPU (e.g., VT-x or AMD-V), if any.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "VT-x"},
		// RAM
		/*[TIREX_RAM_USED_PROCESS_KB] = */
		{.description = "RAM usage of the tracked process in kilobytes.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "{\"max\": 21630, \"min\": 0, \"avg\": 0, \"timeseries\": {\"timestamps\": [\"108ms\"], "
					"\"values\": [21630]}}"},
		/*[TIREX_RAM_USED_SYSTEM_MB] = */
		{.description = "RAM usage of the entire system in megabytes. ",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "{\"max\": 22040, \"min\": 0, \"avg\": 0, \"timeseries\": {\"timestamps\": [\"108ms\"], "
					"\"values\": [22040]}}"},
		/*[TIREX_RAM_AVAILABLE_SYSTEM_MB] = */
		{.description = "Amount of RAM available in the system in megabytes.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "32888"},
		/*[TIREX_RAM_ENERGY_SYSTEM_JOULES] = */
		{.description = "The energy consumed by the DRAM by the entire system over the tracked period in joules.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "2970137"},
		// GPU
		/*[TIREX_GPU_SUPPORTED] = */
		{.description = "1 if a GPU is detected in the system, and we support tracking it; 0 otherwise.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "1"},
		/*[TIREX_GPU_MODEL_NAME] = */
		{.description = "The name of the GPU model detected in the system.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "NVIDIA GeForce RTX 2060"},
		/*[TIREX_GPU_NUM_CORES] = */
		{.description = "Number of GPU cores available in the system.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "1920"},
		/*[TIREX_GPU_USED_PROCESS_PERCENT] = */
		{.description = "GPU usage of the tracked process in percent.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "{\"max\": 23, \"min\": 0, \"avg\": 0, \"timeseries\": {\"timestamps\": [\"108ms\"], \"values\": "
					"[23]}}"},
		/*[TIREX_GPU_USED_SYSTEM_PERCENT] = */
		{.description = "GPU utilization of the entire system in percent.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "{\"max\": 23, \"min\": 0, \"avg\": 0, \"timeseries\": {\"timestamps\": [\"108ms\"], \"values\": "
					"[23]}}"},
		/*[TIREX_GPU_VRAM_USED_PROCESS_MB] = */
		{.description = "GPU VRAM usage of the tracked process in megabyte.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "{\"max\": 1557, \"min\": 0, \"avg\": 0, \"timeseries\": {\"timestamps\": [\"105ms\"], \"values\": "
					"[1557]}}"},
		/*[TIREX_GPU_VRAM_USED_SYSTEM_MB] = */
		{.description = "GPU VRAM usage of the entire system in megabytes.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "{\"max\": 1557, \"min\": 0, \"avg\": 0, \"timeseries\": {\"timestamps\": [\"105ms\"], \"values\": "
					"[1557]}}"},
		/*[TIREX_GPU_VRAM_AVAILABLE_SYSTEM_MB] = */
		{.description = "Amount of GPU VRAM available in the system in megabytes.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "6442"},
		/*[TIREX_GPU_ENERGY_SYSTEM_JOULES] = */
		{.description = "The energy consumed by the GPU for the entire system in joules.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "2970137"},
		// Git
		/*[TIREX_GIT_IS_REPO] = */
		{.description = "1 if the current working directory is (part of) a Git repository; 0 otherwise",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "1"},
		/*[TIREX_GIT_HASH] =*/
		{.description = "SHA1 hash of all files checked into the repository.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "aa5fba7feff8605c3b253b46fc86d7ac1732a586"},
		/*[TIREX_GIT_LAST_COMMIT_HASH] =*/
		{.description = "Latest Git commit SHA1 hash.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "ff52eaf7c0291edbba93c87917e555c720267740"},
		/*[TIREX_GIT_BRANCH] = */
		{.description = "Checked-out Git branch name.", .datatype = tirexResultType::TIREX_STRING, .example = "main"},
		/*[TIREX_GIT_BRANCH_UPSTREAM] = */
		{.description = "Upstream branch of the checked-out Git branch name.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "main"},
		/*[TIREX_GIT_TAGS] = */
		{.description = "List of Git tag(s) at the current commit, if any.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "[\"1.0.0\"]"},
		/*[TIREX_GIT_REMOTE_ORIGIN] = */
		{.description = "URL of the `origin` remote if it is set.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "git@github.com:tira-io/measure.git"},
		/*[TIREX_GIT_UNCOMMITTED_CHANGES] = */
		{.description = "1 if some changes are not yet committed and 0 otherwise",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "1"},
		/*[TIREX_GIT_UNPUSHED_CHANGES] = */
		{.description = "1 if some changes are not yet pushed and 0 otherwise.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "0"},
		/*[TIREX_GIT_UNCHECKED_FILES] = */
		{.description = "1 if there are files that are not ignored (by a .gitignore file) and also not checked "
						"into the repository; 0 otherwise.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "1"},
		/*[TIREX_VERSION_MEASURE] = */
		{.description = "Reports the version of the TIREx Tracker that was used to collect the metadata.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = TIREX_VERSION},
		/*[TIREX_INVOCATION] = */
		{.description = "Reports the command that was used to spawn the tracked process.",
		 .datatype = tirexResultType::TIREX_STRING,
		 .example = "sleep 50"},
};
static_assert((sizeof(measureInfos) / sizeof(*measureInfos)) == TIREX_MEASURE_COUNT);

tirexError tirexMeasureInfoGet(tirexMeasure measure, const tirexMeasureInfo** info) {
	if (measure < 0 || measure >= tirexMeasure::TIREX_MEASURE_COUNT)
		return tirexError::TIREX_INVALID_ARGUMENT;
	*info = &measureInfos[measure];
	return tirexError::TIREX_SUCCESS;
}