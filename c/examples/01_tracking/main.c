#include <tirex_tracker.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WINDOWS) || defined(_WIN32) || defined(WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

void tirex_sleep(int ms) {
#if defined(_WINDOWS) || defined(_WIN32) || defined(WIN32)
	Sleep(ms);
#else
	usleep(ms * 1000);
#endif
}

static void logcallback(tirexLogLevel level, const char* component, const char* message) {
	static const char* levlToStr[] = {[TRACE] = "TRACE", [DEBUG] = "DEBUG", [INFO] = "INFO",
									  [WARN] = "WARN",	 [ERROR] = "ERROR", [CRITICAL] = "CRITICAL"};
	printf("[%s] [%s] %s\n", levlToStr[level], component, message);
}

static const char* measureToName[] = {
		[TIREX_OS_NAME] = "os name",
		[TIREX_OS_KERNEL] = "os kernel",
		[TIREX_TIME_ELAPSED_WALL_CLOCK_MS] = "time elapsed wall clock ms",
		[TIREX_TIME_ELAPSED_USER_MS] = "time elapsed user ms",
		[TIREX_TIME_ELAPSED_SYSTEM_MS] = "time elapsed system ms",
		[TIREX_CPU_USED_PROCESS_PERCENT] = "cpu used process percent",
		[TIREX_CPU_USED_SYSTEM_PERCENT] = "cpu used system percent",
		[TIREX_CPU_AVAILABLE_SYSTEM_CORES] = "cpu available system cores",
		[TIREX_CPU_ENERGY_SYSTEM_JOULES] = "cpu energy system joules",
		[TIREX_CPU_FEATURES] = "cpu features",
		[TIREX_CPU_FREQUENCY_MHZ] = "cpu frequency mhz",
		[TIREX_CPU_FREQUENCY_MIN_MHZ] = "cpu frequency min mhz",
		[TIREX_CPU_FREQUENCY_MAX_MHZ] = "cpu frequency max mhz",
		[TIREX_CPU_VENDOR_ID] = "cpu vendor id",
		[TIREX_CPU_BYTE_ORDER] = "cpu byte order",
		[TIREX_CPU_ARCHITECTURE] = "cpu architecture",
		[TIREX_CPU_MODEL_NAME] = "cpu model name",
		[TIREX_CPU_CORES_PER_SOCKET] = "cpu cores per socket",
		[TIREX_CPU_THREADS_PER_CORE] = "cpu threads per core",
		[TIREX_CPU_CACHES] = "cpu caches",
		[TIREX_CPU_VIRTUALIZATION] = "cpu virtualization",
		[TIREX_RAM_USED_PROCESS_KB] = "ram used process kb",
		[TIREX_RAM_USED_SYSTEM_MB] = "ram used system mb",
		[TIREX_RAM_AVAILABLE_SYSTEM_MB] = "ram available system mb",
		[TIREX_RAM_ENERGY_SYSTEM_JOULES] = "ram energy system joules",
		[TIREX_GPU_SUPPORTED] = "gpu supported",
		[TIREX_GPU_MODEL_NAME] = "gpu model name",
		[TIREX_GPU_NUM_CORES] = "gpu num cores",
		[TIREX_GPU_USED_PROCESS_PERCENT] = "gpu used process percent",
		[TIREX_GPU_USED_SYSTEM_PERCENT] = "gpu used system percent",
		[TIREX_GPU_VRAM_USED_PROCESS_MB] = "gpu vram used process mb",
		[TIREX_GPU_VRAM_USED_SYSTEM_MB] = "gpu vram used system mb",
		[TIREX_GPU_VRAM_AVAILABLE_SYSTEM_MB] = "gpu vram available system mb",
		[TIREX_GPU_ENERGY_SYSTEM_JOULES] = "gpu energy system joules",
		[TIREX_GIT_IS_REPO] = "git is repo",
		[TIREX_GIT_HASH] = "git hash",
		[TIREX_GIT_LAST_COMMIT_HASH] = "git last commit hash",
		[TIREX_GIT_BRANCH] = "git branch",
		[TIREX_GIT_BRANCH_UPSTREAM] = "git branch upstream",
		[TIREX_GIT_TAGS] = "git tags",
		[TIREX_GIT_REMOTE_ORIGIN] = "git remote origin",
		[TIREX_GIT_UNCOMMITTED_CHANGES] = "git uncommitted changes",
		[TIREX_GIT_UNPUSHED_CHANGES] = "git unpushed changes",
		[TIREX_GIT_UNCHECKED_FILES] = "git unchecked files"
};

static void printResult(const tirexResult* result, const char* prefix) {
	size_t num;
	tirexResultEntry entry;
	int err = tirexResultEntryNum(result, &num);
	assert(err == TIREX_SUCCESS);
	for (size_t i = 0; i < num; ++i) {
		if (tirexResultEntryGetByIndex(result, i, &entry) != TIREX_SUCCESS)
			abort();
		assert(entry.source >= 0 && entry.source < TIREX_MEASURE_COUNT);
		printf("[%s] %s\n", measureToName[entry.source], (const char*)entry.value);
	}
}

int fib(int n) {
	if (n <= 1)
		return n;
	return fib(n - 1) + fib(n - 2);
}

int main(int argc, char* argv[]) {
	tirexMeasureHandle* measure;
	tirexResult* result;

	const tirexMeasureConf providers[] = {
			{TIREX_OS_NAME, TIREX_AGG_NO},
			{TIREX_OS_KERNEL, TIREX_AGG_NO},
			{TIREX_TIME_ELAPSED_WALL_CLOCK_MS, TIREX_AGG_NO},
			{TIREX_TIME_ELAPSED_USER_MS, TIREX_AGG_NO},
			{TIREX_TIME_ELAPSED_SYSTEM_MS, TIREX_AGG_NO},
			{TIREX_CPU_USED_PROCESS_PERCENT, TIREX_AGG_NO},
			{TIREX_CPU_USED_SYSTEM_PERCENT, TIREX_AGG_NO},
			{TIREX_CPU_AVAILABLE_SYSTEM_CORES, TIREX_AGG_NO},
			{TIREX_CPU_ENERGY_SYSTEM_JOULES, TIREX_AGG_NO},
			{TIREX_CPU_FEATURES, TIREX_AGG_NO},
			{TIREX_CPU_FREQUENCY_MHZ, TIREX_AGG_NO},
			{TIREX_CPU_FREQUENCY_MIN_MHZ, TIREX_AGG_NO},
			{TIREX_CPU_FREQUENCY_MAX_MHZ, TIREX_AGG_NO},
			{TIREX_CPU_VENDOR_ID, TIREX_AGG_NO},
			{TIREX_CPU_BYTE_ORDER, TIREX_AGG_NO},
			{TIREX_CPU_ARCHITECTURE, TIREX_AGG_NO},
			{TIREX_CPU_MODEL_NAME, TIREX_AGG_NO},
			{TIREX_CPU_CORES_PER_SOCKET, TIREX_AGG_NO},
			{TIREX_CPU_THREADS_PER_CORE, TIREX_AGG_NO},
			{TIREX_CPU_CACHES, TIREX_AGG_NO},
			{TIREX_CPU_VIRTUALIZATION, TIREX_AGG_NO},
			{TIREX_RAM_USED_PROCESS_KB, TIREX_AGG_NO},
			{TIREX_RAM_USED_SYSTEM_MB, TIREX_AGG_NO},
			{TIREX_RAM_AVAILABLE_SYSTEM_MB, TIREX_AGG_NO},
			{TIREX_RAM_ENERGY_SYSTEM_JOULES, TIREX_AGG_NO},
			{TIREX_GPU_SUPPORTED, TIREX_AGG_NO},
			{TIREX_GPU_USED_PROCESS_PERCENT, TIREX_AGG_NO},
			{TIREX_GPU_USED_SYSTEM_PERCENT, TIREX_AGG_NO},
			{TIREX_GPU_VRAM_USED_PROCESS_MB, TIREX_AGG_NO},
			{TIREX_GPU_VRAM_USED_SYSTEM_MB, TIREX_AGG_NO},
			{TIREX_GPU_VRAM_AVAILABLE_SYSTEM_MB, TIREX_AGG_NO},
			{TIREX_GPU_ENERGY_SYSTEM_JOULES, TIREX_AGG_NO},
			{TIREX_GIT_IS_REPO, TIREX_AGG_NO},
			{TIREX_GIT_HASH, TIREX_AGG_NO},
			{TIREX_GIT_LAST_COMMIT_HASH, TIREX_AGG_NO},
			{TIREX_GIT_BRANCH, TIREX_AGG_NO},
			{TIREX_GIT_BRANCH_UPSTREAM, TIREX_AGG_NO},
			{TIREX_GIT_TAGS, TIREX_AGG_NO},
			{TIREX_GIT_REMOTE_ORIGIN, TIREX_AGG_NO},
			{TIREX_GIT_UNCOMMITTED_CHANGES, TIREX_AGG_NO},
			{TIREX_GIT_UNPUSHED_CHANGES, TIREX_AGG_NO},
			{TIREX_GIT_UNCHECKED_FILES, TIREX_AGG_NO},
			tirexNullConf
	};
	tirexSetLogCallback(logcallback);

	// Print information about the system (e.g., OS Information, HW Specs, ...)
	if (tirexFetchInfo(providers, &result) != TIREX_SUCCESS)
		abort();
	printResult(result, NULL);
	tirexResultFree(result);

	// Track metadata
	if (tirexStartTracking(providers, 100, &measure) != TIREX_SUCCESS)
		abort();
	{
		tirex_sleep(1000);
		char* data = calloc(24 * 1000 * 1000, 1);	  // allocate 24 MB
		for (size_t i = 0; i < 24 * 1000 * 1000; ++i) // Access the data so it is not optimized away
			data[i] = 1;
		tirex_sleep(1000);
		free(data);
		fib(45);
	}
	if (tirexStopTracking(measure, &result) != TIREX_SUCCESS)
		abort();
	printResult(result, NULL);
	tirexResultFree(result);
	return 0;
}