#include "formatters.hpp"

#include <cassert>

static const char* measureToName[] = {
		[MSR_OS_NAME] = "os name",
		[MSR_OS_KERNEL] = "os kernel",
		[MSR_TIME_ELAPSED_WALL_CLOCK_MS] = "time elapsed wall clock ms",
		[MSR_TIME_ELAPSED_USER_MS] = "time elapsed user ms",
		[MSR_TIME_ELAPSED_WALL_CLOCK_MS] = "time elapsed wall clock ms",
		[MSR_TIME_ELAPSED_USER_MS] = "time elapsed user ms",
		[MSR_TIME_ELAPSED_SYSTEM_MS] = "time elapsed system ms",
		[MSR_CPU_USED_PROCESS_PERCENT] = "cpu used process percent",
		[MSR_CPU_USED_SYSTEM_PERCENT] = "cpu used system percent",
		[MSR_CPU_AVAILABLE_SYSTEM_CORES] = "cpu available system cores",
		[MSR_CPU_ENERGY_SYSTEM_JOULES] = "cpu energy system joules",
		[MSR_CPU_FEATURES] = "cpu features",
		[MSR_CPU_FREQUENCY_MHZ] = "cpu frequency mhz",
		[MSR_CPU_FREQUENCY_MIN_MHZ] = "cpu frequency min mhz",
		[MSR_CPU_FREQUENCY_MAX_MHZ] = "cpu frequency max mhz",
		[MSR_CPU_VENDOR_ID] = "cpu vendor id",
		[MSR_CPU_BYTE_ORDER] = "cpu byte order",
		[MSR_CPU_ARCHITECTURE] = "cpu architecture",
		[MSR_CPU_MODEL_NAME] = "cpu model name",
		[MSR_CPU_CORES_PER_SOCKET] = "cpu cores per socket",
		[MSR_CPU_THREADS_PER_CORE] = "cpu threads per core",
		[MSR_CPU_CACHES_KB] = "cpu caches kb",
		[MSR_CPU_VIRTUALIZATION] = "cpu virtualization",
		[MSR_RAM_USED_PROCESS_KB] = "ram used process kb",
		[MSR_RAM_USED_SYSTEM_MB] = "ram used system mb",
		[MSR_RAM_AVAILABLE_SYSTEM_MB] = "ram available system mb",
		[MSR_RAM_ENERGY_SYSTEM_JOULES] = "ram energy system joules",
		[MSR_GPU_USED_PROCESS_PERCENT] = "gpu used process percent",
		[MSR_GPU_USED_SYSTEM_PERCENT] = "gpu used system percent",
		[MSR_GPU_VRAM_USED_PROCESS_MB] = "gpu vram used process mb",
		[MSR_GPU_VRAM_USED_SYSTEM_MB] = "gpu vram used system mb",
		[MSR_GPU_VRAM_AVAILABLE_SYSTEM_MB] = "gpu vram available system mb",
		[MSR_GIT_IS_REPO] = "git is repo",
		[MSR_GIT_HASH] = "git hash",
		[MSR_GIT_LAST_COMMIT_HASH] = "git last commit hash",
		[MSR_GIT_BRANCH] = "git branch",
		[MSR_GIT_BRANCH_UPSTREAM] = "git branch upstream",
		[MSR_GIT_TAGS] = "git tags",
		[MSR_GIT_REMOTE_ORIGIN] = "git remote origin",
		[MSR_GIT_UNCOMMITTED_CHANGES] = "git uncommitted changes",
		[MSR_GIT_UNPUSHED_CHANGES] = "git unpushed changes",
		[MSR_GIT_UNCHECKED_FILES] = "git unchecked files"
};

/* SIMPLE FORMATTER */
void msr::simpleFormatter(std::ostream& stream, const msrResult* result) noexcept {
	size_t num;
	msrResultEntry entry;
	assert(msrResultEntryNum(result, &num) == MSR_SUCCESS);
	for (size_t i = 0; i < num; ++i) {
		assert(msrResultEntryGetByIndex(result, i, &entry) == MSR_SUCCESS);
		stream << '[' << measureToName[entry.source] << "] " << reinterpret_cast<const char*>(entry.value) << std::endl;
	}
}

/* JSON FORMATTER */
void msr::jsonFormatter(std::ostream& stream, const msrResult* result) noexcept {
	throw std::runtime_error("Not implemented");
}
