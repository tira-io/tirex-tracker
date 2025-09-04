#include <irtracker.h>
#include <tirex_tracker.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void logcallback(tirexLogLevel level, const char* component, const char* message) {
	static const char* levlToStr[] = {[TRACE] = "TRACE", [DEBUG] = "DEBUG", [INFO] = "INFO",
									  [WARN] = "WARN",	 [ERROR] = "ERROR", [CRITICAL] = "CRITICAL"};
	printf("[%s] [%s] %s\n", levlToStr[level], component, message);
}

int main(int argc, char* argv[]) {
	tirexMeasureHandle* measure;
	tirexResult *info, *result;

	const tirexMeasureConf providers[] = {
			{TIREX_OS_NAME, TIREX_AGG_NO},
			{TIREX_OS_KERNEL, TIREX_AGG_NO},
			{TIREX_TIME_START, TIREX_AGG_NO},
			{TIREX_TIME_STOP, TIREX_AGG_NO},
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
			{TIREX_VERSION_MEASURE, TIREX_AGG_NO},
			{TIREX_INVOCATION, TIREX_AGG_NO},
			{TIREX_DEVCONTAINER_CONF_PATHS, TIREX_AGG_NO},
			tirexNullConf
	};
	tirexSetLogCallback(logcallback);

	// Print information about the system (e.g., OS Information, HW Specs, ...)
	if (tirexFetchInfo(providers, &info) != TIREX_SUCCESS)
		abort();

	// Track metadata
	if (tirexStartTracking(providers, 100, &measure) != TIREX_SUCCESS)
		abort();
	{
		char* data = calloc(24 * 1000 * 1000, 1);	  // allocate 24 MB
		for (size_t i = 0; i < 24 * 1000 * 1000; ++i) // Access the data so it is not optimized away
			data[i] = 1;
		thrd_sleep(&(struct timespec){.tv_sec = 1}, NULL);
		free(data);
		//fib(45);
	}
	if (tirexStopTracking(measure, &result) != TIREX_SUCCESS)
		abort();
	printf("Writing results to ./test.ir_metadata\n");
	if (tirexResultExportIrMetadata(info, result, "./test.ir_metadata") != TIREX_SUCCESS)
		abort();
	tirexResultFree(info);
	tirexResultFree(result);
	return 0;
}