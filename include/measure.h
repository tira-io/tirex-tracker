/**
 * @brief Contains measure's external C API
 */

#ifndef MEASURE_H
#define MEASURE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined _WINDOWS
#if !defined MEASUREAPI_STATIC_IMPORT
#if defined MEASUREAPI_LIB_EXPORT
#define MSR_EXPORT __declspec(dllexport)
#else
#define MSR_EXPORT __declspec(dllimport)
#endif
#else
#define MSR_EXPORT
#endif
#else
#define MSR_EXPORT
#endif

#define MSR_TIME_ELAPSED_WALL_CLOCK_MS ((uint64_t)1 << 0)
#define MSR_TIME_ELAPSED_USER_MS ((uint64_t)1 << 1)
#define MSR_TIME_ELAPSED_SYSTEM_MS ((uint64_t)1 << 2)
#define MSR_TIME_ALL (MSR_TIME_ELAPSED_WALL_CLOCK_MS | MSR_TIME_ELAPSED_USER_MS | MSR_TIME_ELAPSED_SYSTEM_MS)

#define MSR_CPU_USED_PROCESS_PERCENT ((uint64_t)1 << 3)
#define MSR_CPU_USED_SYSTEM_PERCENT ((uint64_t)1 << 4)
#define MSR_CPU_AVAILABLE_SYSTEM_CORES ((uint64_t)1 << 5)
#define MSR_CPU_ENERGY_SYSTEM_JOULES ((uint64_t)1 << 6)
#define MSR_CPU_ALL                                                                                                    \
	(MSR_CPU_USED_PROCESS_PERCENT | MSR_CPU_USED_SYSTEM_PERCENT | MSR_CPU_AVAILABLE_SYSTEM_CORES |                     \
	 MSR_CPU_ENERGY_SYSTEM_JOULES)

#define MSR_RAM_USED_PROCESS_KB ((uint64_t)1 << 7)
#define MSR_RAM_USED_SYSTEM_MB ((uint64_t)1 << 8)
#define MSR_RAM_AVAILABLE_SYSTEM_MB ((uint64_t)1 << 9)
#define MSR_RAM_ENERGY_SYSTEM_JOULES ((uint64_t)1 << 10)
#define MSR_RAM_ALL                                                                                                    \
	(MSR_RAM_USED_PROCESS_KB | MSR_RAM_USED_SYSTEM_MB | MSR_RAM_AVAILABLE_SYSTEM_MB | MSR_RAM_ENERGY_SYSTEM_JOULES)

#define MSR_GPU_USED_PROCESS_PERCENT ((uint64_t)1 << 11)
#define MSR_GPU_USED_SYSTEM_PERCENT ((uint64_t)1 << 12)
#define MSR_GPU_AVAILABLE_SYTEM_CORES ((uint64_t)1 << 13)
#define MSR_GPU_VRAM_USED_PROCESS_MB ((uint64_t)1 << 14)
#define MSR_GPU_VRAM_USED_SYSTEM_MB ((uint64_t)1 << 15)
#define MSR_GPU_VRAM_AVAILABLE_SYSTEM_MB ((uint64_t)1 << 16)
#define MSR_GPU_ALL                                                                                                    \
	(MSR_GPU_USED_PROCESS_PERCENT | MSR_GPU_USED_SYSTEM_PERCENT | MSR_GPU_AVAILABLE_SYTEM_CORES |                      \
	 MSR_GPU_VRAM_USED_PROCESS_MB | MSR_GPU_VRAM_USED_SYSTEM_MB | MSR_GPU_VRAM_AVAILABLE_SYSTEM_MB)

#define MSR_GIT_IS_REPO ((uint64_t)1 << 17)
#define MSR_GIT_HASH ((uint64_t)1 << 18)
#define MSR_GIT_LAST_COMMIT_HASH ((uint64_t)1 << 19)
#define MSR_GIT_BRANCH ((uint64_t)1 << 20)
#define MSR_GIT_TAGS ((uint64_t)1 << 21)
#define MSR_GIT_REMOTE_ORIGIN ((uint64_t)1 << 22)
#define MSR_GIT_UNCOMMITTED_CHANGES ((uint64_t)1 << 23)
#define MSR_GIT_UNPUSHED_CHANGES ((uint64_t)1 << 24)
#define MSR_GIT_UNCHECKED_FILES ((uint64_t)1 << 25)
#define MSR_GIT_ALL                                                                                                    \
	(MSR_GIT_IS_REPO | MSR_GIT_HASH | MSR_GIT_LAST_COMMIT_HASH | MSR_GIT_BRANCH | MSR_GIT_TAGS |                       \
	 MSR_GIT_REMOTE_ORIGIN | MSR_GIT_UNCOMMITTED_CHANGES | MSR_GIT_UNPUSHED_CHANGES | MSR_GIT_UNCHECKED_FILES)

#ifdef __cplusplus
extern "C" {
#endif

typedef enum msrError_enum { MSR_SUCCESS = 0 } msrError;

typedef enum msrResultType_enum { MSR_STRING = 0, MSR_INTEGER = 1, MSR_FLOATING = 2 } msrResultType;

typedef uint64_t msrMeasureFlag;

/**
 * @brief Represents different levels of verbosity.
 * @details The enum values are sorted by verbosity. That is, for example, if logs should only be logged \c level
 * \c INFO or higher, then `level >= INFO` can be used to check what should be logged.
 */
typedef enum msrLogLevel_enum { TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL } msrLogLevel;

/**
 * @brief Points to a function that takes in a log level, component, and message which should be logged.
 * @details The log callback can handle the call as it likes (including ignoring it for example because logging is set
 * to a lower verbosity or turned of). The \p component may be used to identify, where the log is comming from.
 */
typedef void (*msrLogCallback)(msrLogLevel level, const char* component, const char* message);

/**
 * @brief Set a callback that should be used for all future logs.
 * @details The default logger does not do anything.
 * 
 * @param callback Points to a function that takes in a log level, component, and message which should be logged. 
 */
MSR_EXPORT void msrSetLogCallback(msrLogCallback callback);

/**
 * @brief Holds a handle to an ongoing measurement task.
 */
typedef struct msrMeasureHandle_st msrMeasureHandle;

/**
 * @brief Holds a handle to the results of a measurement.
 * @details The measurement result is structed as a tree of categories, subcategories and (at the leaves) values.
 * 
 * @see msrStopMeasure(msrMeasureHandle*, char**)
 */
typedef struct msrResult_st msrResult;

/**
 * @brief Holds a result entry, i.e., a key-value pair of a name (string) and a value (msrResult).
 */
typedef struct msrResultEntry_st {
	msrMeasureFlag source;
	const void* value;
	msrResultType type;
} msrResultEntry;

/**
 * @brief 
 * 
 */
MSR_EXPORT msrError msrFetchEnvInfo(msrMeasureFlag measures, msrResult** result);

/**
 * @brief Initializes the providers set in the configuration and starts measuring.
 * 
 * @param config the configuration to use to measure
 * @return a handle to the running measurement.
 * @see msrStopMeasure(msrMeasureHandle*, char**)
 */
MSR_EXPORT msrError msrStartMeasure(msrMeasureFlag measures, size_t pollIntervalMs, msrMeasureHandle** handle);

/**
 * @brief Stops the measurement and deinitializes the data providers.
 * @details This function **must** be called **exactly once** for each measurement job.
 * 
 * @param measure The handle of the running measurement that should be stopped.
 * @return a handle to the result tree of the measurement. Must be freed by the caller using msrResultFree(msrResult*)
 * @see msrStartMeasure(msrConfig)
 */
MSR_EXPORT msrError msrStopMeasure(msrMeasureHandle* handle, msrResult** result);

inline msrError msrResultEntryGetByIndex(const msrResult* result, size_t index, msrResultEntry* entry);

inline msrError msrResultEntryNum(const msrResult* result, size_t* num);

/**
 * @brief Frees the given result, its children, and all associated values.
 * 
 * @param result The result to be freed.
 */
MSR_EXPORT void msrResultFree(msrResult* result);

/**
 * @defgroup dataprovider Methods and definitions necessary to fetch information about the underlying data sources.
 * Dataproviders are the underlying mechanism for collecting the requested metadata and are abstracted away by the API.
 * The following definitions can be used to get more information (e.g., to report detailed version information).
 * @{
 */

/**
 * @brief Contains information about a data provider.
 * @details Data providers are modules tasked with collecting associated metrics about the program.
 */
typedef struct msrDataProvider_st {
	const char* name;		 /**< The human legible name of the data provider **/
	const char* description; /**< The humand legible description of the data provider **/
	const char* version; /**< A textual representation of the provider's version. May contain linefeeds. May be NULL **/
} msrDataProvider;

/**
 * @brief Populates the given buffer with all available providers (or at most \p bufsize if \p buf is not large enugh)
 * and returns the total number of available providers.
 * @details If \p buf is \c NULL , \p bufsize is ignored and only the number of available providers are returned. A
 * typical call may look as follows:
 * ```c
 * size_t num = msrDataProviderGetAll(NULL, 0);
 * struct msrDataProvider* buf = (struct msrDataProvider*)calloc(num, sizeof(struct msrDataProvider));
 * msrDataProviderGetAll(buf, num);
 * // ...
 * free(buf);
 * ```
 * 
 * @param providers 
 * @param bufsize 
 * @return The number of available data providers.
 * @see msrDataProviderGetCount
 */
MSR_EXPORT size_t msrDataProviderGetAll(msrMeasureFlag* flag);
/** @} */ // end of dataprovider
#ifdef __cplusplus
}
#endif

#endif