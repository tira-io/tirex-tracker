#include <measure.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WINDOWS
#include <windows.h>
#include <synchapi.h>
#include <time.h>

static int thrd_sleep(const struct timespec* duration, struct timespec* remaining) {
	Sleep(duration->tv_sec*1000);
	return 0;
}
#else
#if defined(__has_include)
#if !__has_include(<threads.h>)
// Under MacOS this is not set properly :(
#define __STDC_NO_THREADS__ 0
#endif
#endif
#if (!defined(__STDC_NO_THREADS__) || __STDC_NO_THREADS__)
#include <threads.h>
#else
#include <time.h>
#include <unistd.h>

static int thrd_sleep(const struct timespec* duration, struct timespec* remaining) {
	return nanosleep(duration, remaining);
}
#endif
#endif

static void logcallback(msrLogLevel level, const char* component, const char* message) {
	static const char* levlToStr[] = {[TRACE] = "TRACE", [DEBUG] = "DEBUG", [INFO] = "INFO",
									  [WARN] = "WARN",	 [ERROR] = "ERROR", [CRITICAL] = "CRITICAL"};
	printf("[%s] [%s] %s\n", levlToStr[level], component, message);
}

static const char* measureToName[] = {
		[MSR_OS_NAME] = "os name",
		[MSR_OS_KERNEL] = "os kernel",
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
		[MSR_CPU_CACHES] = "cpu caches",
		[MSR_CPU_VIRTUALIZATION] = "cpu virtualization",
		[MSR_RAM_USED_PROCESS_KB] = "ram used process kb",
		[MSR_RAM_USED_SYSTEM_MB] = "ram used system mb",
		[MSR_RAM_AVAILABLE_SYSTEM_MB] = "ram available system mb",
		[MSR_RAM_ENERGY_SYSTEM_JOULES] = "ram energy system joules",
		[MSR_GPU_SUPPORTED] = "gpu supported",
		[MSR_GPU_MODEL_NAME] = "gpu model name",
		[MSR_GPU_NUM_CORES] = "gpu num cores",
		[MSR_GPU_USED_PROCESS_PERCENT] = "gpu used process percent",
		[MSR_GPU_USED_SYSTEM_PERCENT] = "gpu used system percent",
		[MSR_GPU_VRAM_USED_PROCESS_MB] = "gpu vram used process mb",
		[MSR_GPU_VRAM_USED_SYSTEM_MB] = "gpu vram used system mb",
		[MSR_GPU_VRAM_AVAILABLE_SYSTEM_MB] = "gpu vram available system mb",
		[MSR_GPU_ENERGY_SYSTEM_JOULES] = "gpu energy system joules",
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

static void printResult(const msrResult* result, const char* prefix) {
	size_t num;
	msrResultEntry entry;
	int err = msrResultEntryNum(result, &num);
	assert(err == MSR_SUCCESS);
	for (size_t i = 0; i < num; ++i) {
		if (msrResultEntryGetByIndex(result, i, &entry) != MSR_SUCCESS)
			abort();
		assert(entry.source >= 0 && entry.source < MSR_MEASURE_COUNT);
		printf("[%s] %s\n", measureToName[entry.source], (const char*)entry.value);
	}
}

int fib(int n) {
	if (n <= 1)
		return n;
	return fib(n - 1) + fib(n - 2);
}

int main(int argc, char* argv[]) {
	msrMeasureHandle* measure;
	msrResult* result;

	const msrMeasureConf providers[] = {
			{MSR_OS_NAME, MSR_AGG_NO},
			{MSR_OS_KERNEL, MSR_AGG_NO},
			{MSR_TIME_ELAPSED_WALL_CLOCK_MS, MSR_AGG_NO},
			{MSR_TIME_ELAPSED_USER_MS, MSR_AGG_NO},
			{MSR_TIME_ELAPSED_SYSTEM_MS, MSR_AGG_NO},
			{MSR_CPU_USED_PROCESS_PERCENT, MSR_AGG_NO},
			{MSR_CPU_USED_SYSTEM_PERCENT, MSR_AGG_NO},
			{MSR_CPU_AVAILABLE_SYSTEM_CORES, MSR_AGG_NO},
			{MSR_CPU_ENERGY_SYSTEM_JOULES, MSR_AGG_NO},
			{MSR_CPU_FEATURES, MSR_AGG_NO},
			{MSR_CPU_FREQUENCY_MHZ, MSR_AGG_NO},
			{MSR_CPU_FREQUENCY_MIN_MHZ, MSR_AGG_NO},
			{MSR_CPU_FREQUENCY_MAX_MHZ, MSR_AGG_NO},
			{MSR_CPU_VENDOR_ID, MSR_AGG_NO},
			{MSR_CPU_BYTE_ORDER, MSR_AGG_NO},
			{MSR_CPU_ARCHITECTURE, MSR_AGG_NO},
			{MSR_CPU_MODEL_NAME, MSR_AGG_NO},
			{MSR_CPU_CORES_PER_SOCKET, MSR_AGG_NO},
			{MSR_CPU_THREADS_PER_CORE, MSR_AGG_NO},
			{MSR_CPU_CACHES, MSR_AGG_NO},
			{MSR_CPU_VIRTUALIZATION, MSR_AGG_NO},
			{MSR_RAM_USED_PROCESS_KB, MSR_AGG_NO},
			{MSR_RAM_USED_SYSTEM_MB, MSR_AGG_NO},
			{MSR_RAM_AVAILABLE_SYSTEM_MB, MSR_AGG_NO},
			{MSR_RAM_ENERGY_SYSTEM_JOULES, MSR_AGG_NO},
			{MSR_GPU_SUPPORTED, MSR_AGG_NO},
			{MSR_GPU_USED_PROCESS_PERCENT, MSR_AGG_NO},
			{MSR_GPU_USED_SYSTEM_PERCENT, MSR_AGG_NO},
			{MSR_GPU_VRAM_USED_PROCESS_MB, MSR_AGG_NO},
			{MSR_GPU_VRAM_USED_SYSTEM_MB, MSR_AGG_NO},
			{MSR_GPU_VRAM_AVAILABLE_SYSTEM_MB, MSR_AGG_NO},
			{MSR_GPU_ENERGY_SYSTEM_JOULES, MSR_AGG_NO},
			{MSR_GIT_IS_REPO, MSR_AGG_NO},
			{MSR_GIT_HASH, MSR_AGG_NO},
			{MSR_GIT_LAST_COMMIT_HASH, MSR_AGG_NO},
			{MSR_GIT_BRANCH, MSR_AGG_NO},
			{MSR_GIT_BRANCH_UPSTREAM, MSR_AGG_NO},
			{MSR_GIT_TAGS, MSR_AGG_NO},
			{MSR_GIT_REMOTE_ORIGIN, MSR_AGG_NO},
			{MSR_GIT_UNCOMMITTED_CHANGES, MSR_AGG_NO},
			{MSR_GIT_UNPUSHED_CHANGES, MSR_AGG_NO},
			{MSR_GIT_UNCHECKED_FILES, MSR_AGG_NO},
			msrNullConf
	};
	msrSetLogCallback(logcallback);

	// Print information about the system (e.g., OS Information, HW Specs, ...)
	if (msrFetchInfo(providers, &result) != MSR_SUCCESS)
		abort();
	printResult(result, NULL);
	msrResultFree(result);

	// Track metadata
	if (msrStartMeasure(providers, 100, &measure) != MSR_SUCCESS)
		abort();
	{
		thrd_sleep(&(struct timespec){.tv_sec = 1}, NULL);
		char* data = calloc(24 * 1000 * 1000, 1);	  // allocate 24 MB
		for (size_t i = 0; i < 24 * 1000 * 1000; ++i) // Access the data so it is not optimized away
			data[i] = 1;
		thrd_sleep(&(struct timespec){.tv_sec = 1}, NULL);
		free(data);
		//fib(45);
	}
	if (msrStopMeasure(measure, &result) != MSR_SUCCESS)
		abort();
	printResult(result, NULL);
	msrResultFree(result);
	return 0;
}