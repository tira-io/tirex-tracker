#include "formatters.hpp"

#include <cassert>

static const char* measureToName[] = {
		/*[TIREX_OS_NAME] =*/"os name",
		/*[TIREX_OS_KERNEL] =*/"os kernel",
		/*[TIREX_TIME_ELAPSED_WALL_CLOCK_MS] =*/"time elapsed wall clock ms",
		/*[TIREX_TIME_ELAPSED_USER_MS] =*/"time elapsed user ms",
		/*[TIREX_TIME_ELAPSED_SYSTEM_MS] =*/"time elapsed system ms",
		/*[TIREX_CPU_USED_PROCESS_PERCENT] =*/"cpu used process percent",
		/*[TIREX_CPU_USED_SYSTEM_PERCENT] =*/"cpu used system percent",
		/*[TIREX_CPU_AVAILABLE_SYSTEM_CORES] =*/"cpu available system cores",
		/*[TIREX_CPU_ENERGY_SYSTEM_JOULES] =*/"cpu energy system joules",
		/*[TIREX_CPU_FEATURES] =*/"cpu features",
		/*[TIREX_CPU_FREQUENCY_MHZ] =*/"cpu frequency mhz",
		/*[TIREX_CPU_FREQUENCY_MIN_MHZ] =*/"cpu frequency min mhz",
		/*[TIREX_CPU_FREQUENCY_MAX_MHZ] =*/"cpu frequency max mhz",
		/*[TIREX_CPU_VENDOR_ID] =*/"cpu vendor id",
		/*[TIREX_CPU_BYTE_ORDER] =*/"cpu byte order",
		/*[TIREX_CPU_ARCHITECTURE] =*/"cpu architecture",
		/*[TIREX_CPU_MODEL_NAME] =*/"cpu model name",
		/*[TIREX_CPU_CORES_PER_SOCKET] =*/"cpu cores per socket",
		/*[TIREX_CPU_THREADS_PER_CORE] =*/"cpu threads per core",
		/*[TIREX_CPU_CACHES] =*/"cpu caches kb",
		/*[TIREX_CPU_VIRTUALIZATION] =*/"cpu virtualization",
		/*[TIREX_RAM_USED_PROCESS_KB] =*/"ram used process kb",
		/*[TIREX_RAM_USED_SYSTEM_MB] =*/"ram used system mb",
		/*[TIREX_RAM_AVAILABLE_SYSTEM_MB] =*/"ram available system mb",
		/*[TIREX_RAM_ENERGY_SYSTEM_JOULES] =*/"ram energy system joules",
		/*[TIREX_GPU_SUPPORTED] =*/"gpu supported",
		/*[TIREX_GPU_MODEL_NAME] =*/"gpu model name",
		/*[TIREX_GPU_NUM_CORES] =*/"gpu num cores",
		/*[TIREX_GPU_USED_PROCESS_PERCENT] =*/"gpu used process percent",
		/*[TIREX_GPU_USED_SYSTEM_PERCENT] =*/"gpu used system percent",
		/*[TIREX_GPU_VRAM_USED_PROCESS_MB] =*/"gpu vram used process mb",
		/*[TIREX_GPU_VRAM_USED_SYSTEM_MB] =*/"gpu vram used system mb",
		/*[TIREX_GPU_VRAM_AVAILABLE_SYSTEM_MB] =*/"gpu vram available system mb",
		/*[TIREX_GPU_ENERGY_SYSTEM_JOULES] =*/"gpu energy system joules",
		/*[TIREX_GIT_IS_REPO] =*/"git is repo",
		/*[TIREX_GIT_HASH] =*/"git hash",
		/*[TIREX_GIT_LAST_COMMIT_HASH] =*/"git last commit hash",
		/*[TIREX_GIT_BRANCH] =*/"git branch",
		/*[TIREX_GIT_BRANCH_UPSTREAM] =*/"git branch upstream",
		/*[TIREX_GIT_TAGS] =*/"git tags",
		/*[TIREX_GIT_REMOTE_ORIGIN] =*/"git remote origin",
		/*[TIREX_GIT_UNCOMMITTED_CHANGES] =*/"git uncommitted changes",
		/*[TIREX_GIT_UNPUSHED_CHANGES] =*/"git unpushed changes",
		/*[TIREX_GIT_UNCHECKED_FILES] =*/"git unchecked files"
};

/* SIMPLE FORMATTER */
void tirex::simpleFormatter(std::ostream& stream, const tirexResult* info, const tirexResult* result) noexcept {
	// Info
	size_t num;
	tirexResultEntry entry;
	auto err = tirexResultEntryNum(info, &num);
	assert(err == TIREX_SUCCESS);
	for (size_t i = 0; i < num; ++i) {
		err = tirexResultEntryGetByIndex(info, i, &entry);
		assert(err == TIREX_SUCCESS);
		stream << '[' << measureToName[entry.source] << "] " << reinterpret_cast<const char*>(entry.value) << std::endl;
	}
	// Result
	err = tirexResultEntryNum(result, &num);
	assert(err == TIREX_SUCCESS);
	for (size_t i = 0; i < num; ++i) {
		err = tirexResultEntryGetByIndex(result, i, &entry);
		assert(err == TIREX_SUCCESS);
		stream << '[' << measureToName[entry.source] << "] " << reinterpret_cast<const char*>(entry.value) << std::endl;
	}
}

/* JSON FORMATTER */
void tirex::jsonFormatter(std::ostream& stream, const tirexResult* info, const tirexResult* result) noexcept {
	tirex::simpleFormatter(stream, info, result); /** \todo implement properly **/
}

/** IR_METADATA FORMATTER */
extern tirexError writeIrMetadata(const tirexResult* info, const tirexResult* result, std::ostream& stream);

void tirex::irmetadataFormatter(std::ostream& stream, const tirexResult* info, const tirexResult* result) noexcept {
	writeIrMetadata(result, nullptr, stream);
}
