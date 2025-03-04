/**
 * @file tirex_tracker.h
 * @brief Contains tirex tracker's external C API
 * 
 */

#ifndef TIREX_TRACKER_H
#define TIREX_TRACKER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(_WINDOWS)
#if !defined(TIREX_TRACKER_STATIC_IMPORT)
#if defined(TIREX_TRACKER_LIB_EXPORT)
#define TIREX_EXPORT __declspec(dllexport)
#else
#define TIREX_EXPORT __declspec(dllimport)
#endif
#else
#define TIREX_EXPORT
#endif
#else
#define TIREX_EXPORT
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
typedef enum tirexError_enum { TIREX_SUCCESS = 0, TIREX_INVALID_ARGUMENT = 1 } tirexError;
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
typedef enum tirexMeasure_enum {
	TIREX_OS_NAME = 0,	 /**< Retrieve the name of the operating system (EnvInfo). */
	TIREX_OS_KERNEL = 1, /**< Retrieve the version of the kernel (EnvInfo). */

	/**
	 * @brief Measure the "real" (wall clock) time in milliseconds that elapsed between tirexStartTracking and
	 * tirexStopTracking (Measurement).
	 */
	TIREX_TIME_ELAPSED_WALL_CLOCK_MS = 2,
	/**
	 * @brief Measure the time in milliseconds that the program spend in user mode between tirexStartTracking and
	 * tirexStopTracking (Measurement).
	 */
	TIREX_TIME_ELAPSED_USER_MS = 3,
	/**
	 * @brief Measure the time in milliseconds that the program spend in kernel mode between tirexStartTracking and
	 * tirexStopTracking (Measurement).
	 */
	TIREX_TIME_ELAPSED_SYSTEM_MS = 4,

	TIREX_CPU_USED_PROCESS_PERCENT = 5,
	TIREX_CPU_USED_SYSTEM_PERCENT = 6,
	TIREX_CPU_AVAILABLE_SYSTEM_CORES = 7,
	TIREX_CPU_ENERGY_SYSTEM_JOULES = 8,
	TIREX_CPU_FEATURES = 9,
	TIREX_CPU_FREQUENCY_MHZ = 10,
	TIREX_CPU_FREQUENCY_MIN_MHZ = 11,
	TIREX_CPU_FREQUENCY_MAX_MHZ = 12,
	TIREX_CPU_VENDOR_ID = 13,
	TIREX_CPU_BYTE_ORDER = 14,
	TIREX_CPU_ARCHITECTURE = 15,
	TIREX_CPU_MODEL_NAME = 16,
	TIREX_CPU_CORES_PER_SOCKET = 17,
	TIREX_CPU_THREADS_PER_CORE = 18,
	TIREX_CPU_CACHES = 19,
	TIREX_CPU_VIRTUALIZATION = 20,

	TIREX_RAM_USED_PROCESS_KB = 21,
	TIREX_RAM_USED_SYSTEM_MB = 22,
	TIREX_RAM_AVAILABLE_SYSTEM_MB = 23,
	TIREX_RAM_ENERGY_SYSTEM_JOULES = 24,

	/** @brief Boolean value (0 or 1) representing whether GPU measurements are supported or not. */
	TIREX_GPU_SUPPORTED = 25,
	TIREX_GPU_MODEL_NAME = 26,
	TIREX_GPU_NUM_CORES = 27,
	TIREX_GPU_USED_PROCESS_PERCENT = 28,
	TIREX_GPU_USED_SYSTEM_PERCENT = 29,
	TIREX_GPU_VRAM_USED_PROCESS_MB = 30,
	TIREX_GPU_VRAM_USED_SYSTEM_MB = 31,
	TIREX_GPU_VRAM_AVAILABLE_SYSTEM_MB = 32,
	TIREX_GPU_ENERGY_SYSTEM_JOULES = 33,

	TIREX_GIT_IS_REPO = 34,
	TIREX_GIT_HASH = 35,
	TIREX_GIT_LAST_COMMIT_HASH = 36,
	TIREX_GIT_BRANCH = 37,
	TIREX_GIT_BRANCH_UPSTREAM = 38,
	TIREX_GIT_TAGS = 39,
	TIREX_GIT_REMOTE_ORIGIN = 40,
	TIREX_GIT_UNCOMMITTED_CHANGES = 41,
	TIREX_GIT_UNPUSHED_CHANGES = 42,
	TIREX_GIT_UNCHECKED_FILES = 43,

	/**
	 * @brief The total number of supported measures.
	 * @details It can be assumed that every number in the range `[0, TIREX_MEASURE_COUNT]` is a valid enum value.
	 */
	TIREX_MEASURE_COUNT,
	/**
	 * @brief Represents an invalid measurement.
	 * 
	 * @see tirexNullConf
	 */
	TIREX_MEASURE_INVALID = -1
} tirexMeasure;

#define TIREX_AGG_NO (1 << 1)	/**< Perform no aggregation. */
#define TIREX_AGG_MAX (1 << 2)	/**< For each aggregation intervall, store the maximum value. */
#define TIREX_AGG_MIN (1 << 3)	/**< For each aggregation intervall, store the minimum value. */
#define TIREX_AGG_MEAN (1 << 4) /**< For each aggregation intervall, store the average value. */

typedef uint8_t tirexAggregateFn;

/**
 * @brief Holds a handle to an ongoing measurement task.
 */
typedef struct tirexMeasureHandle_st tirexMeasureHandle;
/** @} */ // end of measure

/**
 * @defgroup tirexresult Result
 * @details
 * @{
 */
/**
 * @brief 
 * @details Note: Currently unused
 */
typedef enum tirexResultType_enum { TIREX_STRING = 0, TIREX_INTEGER = 1, TIREX_FLOATING = 2 } tirexResultType;

/**
 * @brief Holds a handle to the results of a measurement.
 * @details The measurement result is structed as a tree of categories, subcategories and (at the leaves) values.
 * 
 * @see tirexStopTracking(tirexMeasureHandle*, char**)
 */
typedef struct tirexResult_st tirexResult;

/**
 * @brief Holds a result entry, i.e., a key-value pair of a name (string) and a value (tirexResult).
 */
typedef struct tirexResultEntry_st {
	tirexMeasure source;
	const void* value;
	tirexResultType type;
} tirexResultEntry;

/**
 * @brief 
 * 
 * @param[in] result
 * @param[in] index
 * @param[out] entry
 * @return TIREX_SUCCESS on success or an error code. 
 */
TIREX_EXPORT tirexError tirexResultEntryGetByIndex(const tirexResult* result, size_t index, tirexResultEntry* entry);

/**
 * @brief Returns the number of entries contained in the result set.
 * 
 * @param result 
 * @param num 
 * @return TIREX_SUCCESS on success or an error code. 
 */
TIREX_EXPORT tirexError tirexResultEntryNum(const tirexResult* result, size_t* num);

/**
 * @brief Deinitializes and frees the result pointed at by \p result.
 * 
 * @param result The result to be destructed.
 */
TIREX_EXPORT void tirexResultFree(tirexResult* result);
/** @} */ // end of tirexresult

/**
 * @brief 
 */
typedef struct tirexMeasureConf_st {
	tirexMeasure source;		/**< @details The measurement to be configured. */
	tirexAggregateFn aggregate; /**< @details The type of aggregation to use. */
} tirexMeasureConf;

/**
 * @brief Represents an invalid measurement configuration.
 * @details The null configuration should be used as a sentinel value to mark the end of the configuration in
 * tirexStartTracking.
 */
static const tirexMeasureConf tirexNullConf = {.source = TIREX_MEASURE_INVALID};

/**
 * @brief Fetches the system information from the measures requested in \p measures.
 * 
 * @param[in] measures 
 * @param[out] result 
 * @return TIREX_SUCCESS on success or an error code. 
 */
TIREX_EXPORT tirexError tirexFetchInfo(const tirexMeasureConf* measures, tirexResult** result);

/**
 * @ingroup measure
 */
/**
 * @brief Initializes the providers set in the configuration and starts measuring.
 * 
 * @param measures 
 * @param pollIntervalMs 
 * @param[out] handle a handle to the running measurement.
 * @return TIREX_SUCCESS on success or an error code. 
 * 
 * @see tirexStopTracking
 */
TIREX_EXPORT tirexError
tirexStartTracking(const tirexMeasureConf* measures, size_t pollIntervalMs, tirexMeasureHandle** handle);

/**
 * @brief Stops the measurement and deinitializes the data providers.
 * @details This function **must** be called **exactly once** for each measurement job.
 * 
 * @param measure The handle of the running measurement that should be stopped.
 * @return a handle to the result tree of the measurement. Must be freed by the caller using tirexResultFree(tirexResult*)
 * @see tirexStartTracking
 */
TIREX_EXPORT tirexError tirexStopTracking(tirexMeasureHandle* handle, tirexResult** result);
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
typedef enum tirexLogLevel_enum { TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL } tirexLogLevel;

/**
 * @brief Points to a function that takes in a log level, component, and message which should be logged.
 * @details The log callback can handle the call as it likes (including ignoring it for example because logging is set
 * to a lower verbosity or turned of). The \p component may be used to identify, where the log is comming from.
 */
typedef void (*tirexLogCallback)(tirexLogLevel level, const char* component, const char* message);

/**
 * @brief Set a callback that should be used for all future logs.
 * @details The default logger does not do anything.
 * 
 * @param callback Points to a function that takes in a log level, component, and message which should be logged. 
 */
TIREX_EXPORT void tirexSetLogCallback(tirexLogCallback callback);
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
typedef struct tirexDataProvider_st {
	const char* name;		 /**< The human legible name of the data provider **/
	const char* description; /**< The humand legible description of the data provider **/
	const char* version; /**< A textual representation of the provider's version. May contain linefeeds. May be NULL **/
} tirexDataProvider;

/**
 * @brief Populates the given buffer with all available providers (or at most \p bufsize if \p buf is not large enugh)
 * and returns the total number of available providers.
 * @details If \p buf is \c NULL , \p bufsize is ignored and only the number of available providers are returned. A
 * typical call may look as follows:
 * ```c
 * size_t num = tirexDataProviderGetAll(NULL, 0);
 * struct tirexDataProvider* buf = (struct tirexDataProvider*)calloc(num, sizeof(struct tirexDataProvider));
 * tirexDataProviderGetAll(buf, num);
 * // ...
 * free(buf);
 * ```
 * 
 * @param[in] providers 
 * @param[in] bufsize 
 * @return The number of available data providers.
 */
TIREX_EXPORT size_t tirexDataProviderGetAll(tirexDataProvider* providers, size_t bufsize);
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
typedef struct tirexMeasureInfo_st {
	const char* description;  /**< @brief A human legible description of the measurement. **/
	tirexResultType datatype; /**< @brief The datatype of the measurment **/
	const char* example;	  /**< @brief An example value of what a value may look like. **/
} tirexMeasureInfo;

/**
 * @brief 
 * 
 * @param[in] measure 
 * @param[out] info 
 * @return TIREX_SUCCESS on success or an error code. 
 */
TIREX_EXPORT tirexError tirexMeasureInfoGet(tirexMeasure measure, const tirexMeasureInfo** info);
/** @} */ // end of measureinfo
#ifdef __cplusplus
}
#endif

#endif