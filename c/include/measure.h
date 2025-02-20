/**
 * @file measure.h
 * @brief Contains measure's external C API
 * 
 */

#ifndef MEASURE_H
#define MEASURE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(_WINDOWS)
#if !defined(MEASUREAPI_STATIC_IMPORT)
#if defined(MEASUREAPI_LIB_EXPORT)
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

/**
 * @defgroup error Error handling
 * @details
 * @{
 */
/**
 * @brief Categorizes different classes of error.
 */
typedef enum msrError_enum { MSR_SUCCESS = 0, MSR_INVALID_ARGUMENT = 1 } msrError;
/** @} */ // end of error

/**
 * @defgroup measure Measuring
 * @brief
 * @details
 * @{
 */
/**
 * @brief Identifiers for all available measurements.
 * @details Different types (EnvInfo, Time Series, Measure)
 */
typedef enum msrMeasure_enum {
	MSR_OS_NAME,   /**< Retrieve the name of the operating system (EnvInfo). */
	MSR_OS_KERNEL, /**< Retrieve the version of the kernel (EnvInfo). */

	MSR_TIME_ELAPSED_WALL_CLOCK_MS, /**< Measure the "real" (wall clock) time in milliseconds that elapsed between msrStartMeasure and msrStopMeasure (Measurement). */
	MSR_TIME_ELAPSED_USER_MS, /**< Measure the time in milliseconds that the program spend in user mode between msrStartMeasure and msrStopMeasure (Measurement). */
	MSR_TIME_ELAPSED_SYSTEM_MS, /**< Measure the time in milliseconds that the program spend in kernel mode between msrStartMeasure and msrStopMeasure (Measurement). */

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
	MSR_CPU_CACHES,
	MSR_CPU_VIRTUALIZATION,

	MSR_RAM_USED_PROCESS_KB,
	MSR_RAM_USED_SYSTEM_MB,
	MSR_RAM_AVAILABLE_SYSTEM_MB,
	MSR_RAM_ENERGY_SYSTEM_JOULES,

	MSR_GPU_SUPPORTED, /**< Boolean value (0 or 1) representing whether GPU measurements are supported or not. */
	MSR_GPU_MODEL_NAME,
	MSR_GPU_NUM_CORES,
	MSR_GPU_USED_PROCESS_PERCENT,
	MSR_GPU_USED_SYSTEM_PERCENT,
	MSR_GPU_VRAM_USED_PROCESS_MB,
	MSR_GPU_VRAM_USED_SYSTEM_MB,
	MSR_GPU_VRAM_AVAILABLE_SYSTEM_MB,
	MSR_GPU_ENERGY_SYSTEM_JOULES,

	MSR_GIT_IS_REPO,
	MSR_GIT_HASH,
	MSR_GIT_LAST_COMMIT_HASH,
	MSR_GIT_BRANCH,
	MSR_GIT_BRANCH_UPSTREAM,
	MSR_GIT_TAGS,
	MSR_GIT_REMOTE_ORIGIN,
	MSR_GIT_UNCOMMITTED_CHANGES,
	MSR_GIT_UNPUSHED_CHANGES,
	MSR_GIT_UNCHECKED_FILES,

	MSR_MEASURE_COUNT, /**< The total number of supported measures. */
	/**
	 * @brief Represents an invalid measurement.
	 * 
	 * @see msrNullConf
	 */
	MSR_MEASURE_INVALID = -1
} msrMeasure;

#define MSR_AGG_NO (1 << 1)	  /**< Perform no aggregation. */
#define MSR_AGG_MAX (1 << 2)  /**< For each aggregation intervall, store the maximum value. */
#define MSR_AGG_MIN (1 << 3)  /**< For each aggregation intervall, store the minimum value. */
#define MSR_AGG_MEAN (1 << 4) /**< For each aggregation intervall, store the average value. */

typedef uint8_t msrAggregateFn;

/**
 * @brief Holds a handle to an ongoing measurement task.
 */
typedef struct msrMeasureHandle_st msrMeasureHandle;
/** @} */ // end of measure

/**
 * @defgroup msrresult Result
 * @details
 * @{
 */
/**
 * @brief 
 */
typedef enum msrResultType_enum { MSR_STRING = 0, MSR_INTEGER = 1, MSR_FLOATING = 2 } msrResultType;

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
	msrMeasure source;
	const void* value;
	msrResultType type;
} msrResultEntry;

/**
 * @brief 
 * 
 * @param[in] result
 * @param[in] index
 * @param[out] entry
 * @return MSR_SUCCESS on success or an error code. 
 */
MSR_EXPORT msrError msrResultEntryGetByIndex(const msrResult* result, size_t index, msrResultEntry* entry);

/**
 * @brief Returns the number of entries contained in the result set.
 * 
 * @param result 
 * @param num 
 * @return MSR_SUCCESS on success or an error code. 
 */
MSR_EXPORT msrError msrResultEntryNum(const msrResult* result, size_t* num);

/**
 * @brief Deinitializes and frees the result pointed at by \p result.
 * 
 * @param result The result to be destructed.
 */
MSR_EXPORT void msrResultFree(msrResult* result);
/** @} */ // end of msrresult

/**
 * @brief 
 */
typedef struct msrMeasureConf_st {
	msrMeasure source;		  /**< @details The measurement to be configured. */
	msrAggregateFn aggregate; /**< @details The type of aggregation to use. */
} msrMeasureConf;

/**
 * @brief Represents an invalid measurement configuration.
 * @details The null configuration should be used as a sentinel value to mark the end of the configuration in
 * msrStartMeasure.
 */
static const msrMeasureConf msrNullConf = {.source = MSR_MEASURE_INVALID};

/**
 * @brief Fetches the system information from the measures requested in \p measures.
 * 
 * @param[in] measures 
 * @param[out] result 
 * @return MSR_SUCCESS on success or an error code. 
 */
MSR_EXPORT msrError msrFetchInfo(const msrMeasureConf* measures, msrResult** result);

/**
 * @ingroup measure
 */
/**
 * @brief Initializes the providers set in the configuration and starts measuring.
 * 
 * @param measures 
 * @param pollIntervalMs 
 * @param[out] handle a handle to the running measurement.
 * @return MSR_SUCCESS on success or an error code. 
 * 
 * @see msrStopMeasure
 */
MSR_EXPORT msrError msrStartMeasure(const msrMeasureConf* measures, size_t pollIntervalMs, msrMeasureHandle** handle);

/**
 * @brief Stops the measurement and deinitializes the data providers.
 * @details This function **must** be called **exactly once** for each measurement job.
 * 
 * @param measure The handle of the running measurement that should be stopped.
 * @return a handle to the result tree of the measurement. Must be freed by the caller using msrResultFree(msrResult*)
 * @see msrStartMeasure
 */
MSR_EXPORT msrError msrStopMeasure(msrMeasureHandle* handle, msrResult** result);
/** @} */ // end of measure

/**
 * @defgroup logging Logging
 * @details Definitions and functions relevant for telling measure where to log information to.
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
 * @defgroup dataprovider Data Providers
 * @details Methods and definitions necessary to fetch information about the underlying data sources.
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
 * @param[in] providers 
 * @param[in] bufsize 
 * @return The number of available data providers.
 */
MSR_EXPORT size_t msrDataProviderGetAll(msrDataProvider* providers, size_t bufsize);
/** @} */ // end of dataprovider

/**
 * @defgroup measureinfo Measure Info
 * @details
 * @{
 */
/**
 * @brief
 * @details
 */
typedef struct msrMeasureInfo_st {
	const char* description; /**< @brief A human legible description of the measurement. **/
	msrResultType datatype;	 /**< @brief The datatype of the measurment **/
	const char* example;	 /**< @brief An example value of what a value may look like. **/
} msrMeasureInfo;

/**
 * @brief 
 * 
 * @param[in] measure 
 * @param[out] info 
 * @return MSR_SUCCESS on success or an error code. 
 */
MSR_EXPORT msrError msrMeasureInfoGet(msrMeasure measure, const msrMeasureInfo** info);
/** @} */ // end of measureinfo
#ifdef __cplusplus
}
#endif

#endif