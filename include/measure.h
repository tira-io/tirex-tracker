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

#ifdef __cplusplus
extern "C" {
#endif

typedef enum msrError_enum { MSR_SUCCESS = 0, MSR_INVALID_ARGUMENT } msrError;

typedef enum msrResultType_enum { MSR_STRING = 0, MSR_INTEGER = 1, MSR_FLOATING = 2 } msrResultType;

typedef enum msrMeasure_enum {
	MSR_OS_NAME,
	MSR_OS_KERNEL,

	MSR_TIME_ELAPSED_WALL_CLOCK_MS,
	MSR_TIME_ELAPSED_USER_MS,
	MSR_TIME_ELAPSED_SYSTEM_MS,

	MSR_CPU_USED_PROCESS_PERCENT,
	MSR_CPU_USED_SYSTEM_PERCENT,
	MSR_CPU_AVAILABLE_SYSTEM_CORES,
	MSR_CPU_ENERGY_SYSTEM_JOULES,
	MSR_CPU_FEATURES,
	MSR_CPU_FREQUENCY_MHZ,
	MSR_CPU_FREQUENCY_MIN_MHZ,
	MSR_CPU_FREQUENCY_MAX_MHZ,
	MSR_CPU_VENDOR_ID,
	MSR_CPU_BYTE_ORDER,
	MSR_CPU_ARCHITECTURE,
	MSR_CPU_MODEL_NAME,
	MSR_CPU_CORES_PER_SOCKET,
	MSR_CPU_THREADS_PER_CORE,
	MSR_CPU_CACHES_L1_KB,
	MSR_CPU_CACHES_L2_KB,
	MSR_CPU_CACHES_L3_KB,
	MSR_CPU_VIRTUALIZATION,
	MSR_CPU_BOGO_MIPS,

	MSR_RAM_USED_PROCESS_KB,
	MSR_RAM_USED_SYSTEM_MB,
	MSR_RAM_AVAILABLE_SYSTEM_MB,
	MSR_RAM_ENERGY_SYSTEM_JOULES,

	MSR_GPU_SUPPORTED,
	MSR_GPU_USED_PROCESS_PERCENT,
	MSR_GPU_USED_SYSTEM_PERCENT,
	MSR_GPU_AVAILABLE_SYTEM_CORES,
	MSR_GPU_VRAM_USED_PROCESS_MB,
	MSR_GPU_VRAM_USED_SYSTEM_MB,
	MSR_GPU_VRAM_AVAILABLE_SYSTEM_MB,
	MSR_GPU_ENERGY_SYSTEM_JOULES,

	MSR_GIT_IS_REPO,
	MSR_GIT_HASH,
	MSR_GIT_LAST_COMMIT_HASH,
	MSR_GIT_BRANCH,
	MSR_GIT_TAGS,
	MSR_GIT_REMOTE_ORIGIN,
	MSR_GIT_UNCOMMITTED_CHANGES,
	MSR_GIT_UNPUSHED_CHANGES,
	MSR_GIT_UNCHECKED_FILES,

	MSR_MEASURE_COUNT,
	MSR_MEASURE_INVALID = -1
} msrMeasure;

typedef enum msrAggregateFn_enum {
	MSR_AGG_NO,	 /**< Perform no aggregation. */
	MSR_AGG_MAX, /**< For each aggregation intervall, store the maximum value. */
	MSR_AGG_MIN, /**< For each aggregation intervall, store the minimum value. */
	MSR_AGG_MEAN /**< For each aggregation intervall, store the average value. */
} msrAggregateFn;

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

typedef struct msrMeasureConf_st {
	msrMeasure source;
	msrAggregateFn aggregate;
} msrMeasureConf;

static const msrMeasureConf msrNullConf = {.source = MSR_MEASURE_INVALID};

/**
 * @brief Holds a result entry, i.e., a key-value pair of a name (string) and a value (msrResult).
 */
typedef struct msrResultEntry_st {
	msrMeasure source;
	const void* value;
	msrResultType type;
} msrResultEntry;

/**
 * @brief 
 * 
 */
MSR_EXPORT msrError msrFetchInfo(const msrMeasureConf* measures, msrResult** result);

/**
 * @brief Initializes the providers set in the configuration and starts measuring.
 * 
 * @param config the configuration to use to measure
 * @return a handle to the running measurement.
 * @see msrStopMeasure(msrMeasureHandle*, char**)
 */
MSR_EXPORT msrError msrStartMeasure(const msrMeasureConf* measures, size_t pollIntervalMs, msrMeasureHandle** handle);

/**
 * @brief Stops the measurement and deinitializes the data providers.
 * @details This function **must** be called **exactly once** for each measurement job.
 * 
 * @param measure The handle of the running measurement that should be stopped.
 * @return a handle to the result tree of the measurement. Must be freed by the caller using msrResultFree(msrResult*)
 * @see msrStartMeasure(msrConfig)
 */
MSR_EXPORT msrError msrStopMeasure(msrMeasureHandle* handle, msrResult** result);

MSR_EXPORT msrError msrResultEntryGetByIndex(const msrResult* result, size_t index, msrResultEntry* entry);

/**
 * @brief Returns the number of entries contained in the result set.
 * 
 * @param result 
 * @param num 
 * @return MSR_EXPORT 
 */
MSR_EXPORT msrError msrResultEntryNum(const msrResult* result, size_t* num);

/**
 * @brief Deinitializes and frees the result pointed at by \p result.
 * 
 * @param result The result to be destructed.
 */
MSR_EXPORT void msrResultFree(msrResult* result);

/**
 * @defgroup logging Definitions and functions relevant for telling measure where to log information to.
 * @{
 */
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
/** @} */ // end of logging

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
MSR_EXPORT size_t msrDataProviderGetAll(msrDataProvider* providers, size_t bufsize);
/** @} */ // end of dataprovider

/**
 * @defgroup measureinfo
 */
typedef struct msrMeasureInfo_st {
	const char* description;
	unsigned versions;
	msrResultType datatype;
	const char* example;
} msrMeasureInfo;

MSR_EXPORT msrError msrMeasureInfoGet(msrMeasure measure, const msrMeasureInfo** info);

/** @} */ // end of measureinfo
#ifdef __cplusplus
}
#endif

#endif