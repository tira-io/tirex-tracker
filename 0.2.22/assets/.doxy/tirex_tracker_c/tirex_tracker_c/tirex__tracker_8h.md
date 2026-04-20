

# File tirex\_tracker.h



[**FileList**](files.md) **>** [**c**](dir_1784a01aa976a8c78ef5dfc3737bcac8.md) **>** [**include**](dir_2d10db7395ecfee73f7722e70cabff64.md) **>** [**tirex\_tracker.h**](tirex__tracker_8h.md)

[Go to the source code of this file](tirex__tracker_8h_source.md)

_Contains tirex tracker's external C API._ 

* `#include <stdbool.h>`
* `#include <stddef.h>`
* `#include <stdint.h>`
* `#include <tirex_tracker_export.h>`















## Classes

| Type | Name |
| ---: | :--- |
| struct | [**tirexDataProvider\_st**](structtirexDataProvider__st.md) <br>_Contains information about a data provider._  |
| struct | [**tirexMeasureConf\_st**](structtirexMeasureConf__st.md) <br> |
| struct | [**tirexMeasureInfo\_st**](structtirexMeasureInfo__st.md) <br> |
| struct | [**tirexResultEntry\_st**](structtirexResultEntry__st.md) <br>_Holds a result entry, i.e., a key-value pair of a name (string) and a value (tirexResult)._  |


## Public Types

| Type | Name |
| ---: | :--- |
| typedef void(\* | [**tirexAbortCallback**](#typedef-tirexabortcallback)  <br> |
| typedef uint8\_t | [**tirexAggregateFn**](#typedef-tirexaggregatefn)  <br> |
| typedef struct [**tirexDataProvider\_st**](structtirexDataProvider__st.md) | [**tirexDataProvider**](#typedef-tirexdataprovider)  <br>_Contains information about a data provider._  |
| typedef enum [**tirexError\_enum**](tirex__tracker_8h.md#enum-tirexerror_enum) | [**tirexError**](#typedef-tirexerror)  <br>_Categorizes different classes of error._  |
| enum  | [**tirexError\_enum**](#enum-tirexerror_enum)  <br>_Categorizes different classes of error._  |
| typedef void(\* | [**tirexLogCallback**](#typedef-tirexlogcallback)  <br>_Points to a function that takes in a log level, component, and message which should be logged._  |
| typedef enum [**tirexLogLevel\_enum**](tirex__tracker_8h.md#enum-tirexloglevel_enum) | [**tirexLogLevel**](#typedef-tirexloglevel)  <br>_Represents different levels of verbosity._  |
| enum  | [**tirexLogLevel\_enum**](#enum-tirexloglevel_enum)  <br>_Represents different levels of verbosity._  |
| typedef enum [**tirexMeasure\_enum**](tirex__tracker_8h.md#enum-tirexmeasure_enum) | [**tirexMeasure**](#typedef-tirexmeasure)  <br>_Identifiers for all available measurements._  |
| typedef struct [**tirexMeasureConf\_st**](structtirexMeasureConf__st.md) | [**tirexMeasureConf**](#typedef-tirexmeasureconf)  <br> |
| typedef struct tirexMeasureHandle\_st | [**tirexMeasureHandle**](#typedef-tirexmeasurehandle)  <br>_Holds a handle to an ongoing measurement task._  |
| typedef struct [**tirexMeasureInfo\_st**](structtirexMeasureInfo__st.md) | [**tirexMeasureInfo**](#typedef-tirexmeasureinfo)  <br> |
| enum  | [**tirexMeasure\_enum**](#enum-tirexmeasure_enum)  <br>_Identifiers for all available measurements._  |
| typedef struct tirexResult\_st | [**tirexResult**](#typedef-tirexresult)  <br>_Holds a handle to the results of a measurement._  |
| typedef struct [**tirexResultEntry\_st**](structtirexResultEntry__st.md) | [**tirexResultEntry**](#typedef-tirexresultentry)  <br>_Holds a result entry, i.e., a key-value pair of a name (string) and a value (tirexResult)._  |
| typedef enum [**tirexResultType\_enum**](tirex__tracker_8h.md#enum-tirexresulttype_enum) | [**tirexResultType**](#typedef-tirexresulttype)  <br> |
| enum  | [**tirexResultType\_enum**](#enum-tirexresulttype_enum)  <br> |






## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  const [**tirexMeasureConf**](tirex__tracker_8h.md#typedef-tirexmeasureconf) | [**tirexNullConf**](#variable-tirexnullconf)   = `{.source = TIREX\_MEASURE\_INVALID}`<br>_Represents an invalid measurement configuration._  |














## Public Functions

| Type | Name |
| ---: | :--- |
|  TIREX\_TRACKER\_EXPORT size\_t | [**tirexDataProviderGetAll**](#function-tirexdataprovidergetall) ([**tirexDataProvider**](tirex__tracker_8h.md#typedef-tirexdataprovider) \* providers, size\_t bufsize) <br>_Populates the given buffer with all available providers (or at most_ `bufsize` _if_`buf` _is not large enugh) and returns the total number of available providers._ |
|  TIREX\_TRACKER\_EXPORT [**tirexError**](tirex__tracker_8h.md#typedef-tirexerror) | [**tirexFetchInfo**](#function-tirexfetchinfo) (const [**tirexMeasureConf**](tirex__tracker_8h.md#typedef-tirexmeasureconf) \* measures, [**tirexResult**](tirex__tracker_8h.md#typedef-tirexresult) \*\* result) <br>_Fetches the system information from the measures requested in_ `measures` _._ |
|  TIREX\_TRACKER\_EXPORT [**tirexError**](tirex__tracker_8h.md#typedef-tirexerror) | [**tirexMeasureInfoGet**](#function-tirexmeasureinfoget) ([**tirexMeasure**](tirex__tracker_8h.md#typedef-tirexmeasure) measure, const [**tirexMeasureInfo**](tirex__tracker_8h.md#typedef-tirexmeasureinfo) \*\* info) <br> |
|  TIREX\_TRACKER\_EXPORT [**tirexError**](tirex__tracker_8h.md#typedef-tirexerror) | [**tirexResultEntryGetByIndex**](#function-tirexresultentrygetbyindex) (const [**tirexResult**](tirex__tracker_8h.md#typedef-tirexresult) \* result, size\_t index, [**tirexResultEntry**](tirex__tracker_8h.md#typedef-tirexresultentry) \* entry) <br> |
|  TIREX\_TRACKER\_EXPORT [**tirexError**](tirex__tracker_8h.md#typedef-tirexerror) | [**tirexResultEntryNum**](#function-tirexresultentrynum) (const [**tirexResult**](tirex__tracker_8h.md#typedef-tirexresult) \* result, size\_t \* num) <br>_Returns the number of entries contained in the result set._  |
|  TIREX\_TRACKER\_EXPORT void | [**tirexResultFree**](#function-tirexresultfree) ([**tirexResult**](tirex__tracker_8h.md#typedef-tirexresult) \* result) <br>_Deinitializes and frees the result pointed at by_ `result` _._ |
|  TIREX\_TRACKER\_EXPORT void | [**tirexSetAbortCallback**](#function-tirexsetabortcallback) ([**tirexAbortCallback**](tirex__tracker_8h.md#typedef-tirexabortcallback) callback) <br>_Set a callback that should be used in case the program needs to abort._  |
|  TIREX\_TRACKER\_EXPORT void | [**tirexSetAbortLevel**](#function-tirexsetabortlevel) ([**tirexLogLevel**](tirex__tracker_8h.md#typedef-tirexloglevel) level) <br> |
|  TIREX\_TRACKER\_EXPORT void | [**tirexSetLogCallback**](#function-tirexsetlogcallback) ([**tirexLogCallback**](tirex__tracker_8h.md#typedef-tirexlogcallback) callback) <br>_Set a callback that should be used for all future logs._  |
|  TIREX\_TRACKER\_EXPORT [**tirexError**](tirex__tracker_8h.md#typedef-tirexerror) | [**tirexStartTracking**](#function-tirexstarttracking) (const [**tirexMeasureConf**](tirex__tracker_8h.md#typedef-tirexmeasureconf) \* measures, size\_t pollIntervalMs, [**tirexMeasureHandle**](tirex__tracker_8h.md#typedef-tirexmeasurehandle) \*\* handle) <br>_Initializes the providers set in the configuration and starts measuring._  |
|  TIREX\_TRACKER\_EXPORT [**tirexError**](tirex__tracker_8h.md#typedef-tirexerror) | [**tirexStopTracking**](#function-tirexstoptracking) ([**tirexMeasureHandle**](tirex__tracker_8h.md#typedef-tirexmeasurehandle) \* handle, [**tirexResult**](tirex__tracker_8h.md#typedef-tirexresult) \*\* result) <br>_Stops the measurement and deinitializes the data providers._  |



























## Macros

| Type | Name |
| ---: | :--- |
| define  | [**TIREX\_AGG\_MAX**](tirex__tracker_8h.md#define-tirex_agg_max)  `(1 &lt;&lt; 2)`<br> |
| define  | [**TIREX\_AGG\_MEAN**](tirex__tracker_8h.md#define-tirex_agg_mean)  `(1 &lt;&lt; 4)`<br> |
| define  | [**TIREX\_AGG\_MIN**](tirex__tracker_8h.md#define-tirex_agg_min)  `(1 &lt;&lt; 3)`<br> |
| define  | [**TIREX\_AGG\_NO**](tirex__tracker_8h.md#define-tirex_agg_no)  `(1 &lt;&lt; 1)`<br> |
| define  | [**static\_assert**](tirex__tracker_8h.md#define-static_assert) (cond, msg) <br> |

## Public Types Documentation




### typedef tirexAbortCallback 

```C++
typedef void(* tirexAbortCallback) (const char *message);
```




<hr>



### typedef tirexAggregateFn 

```C++
typedef uint8_t tirexAggregateFn;
```




<hr>



### typedef tirexDataProvider 

_Contains information about a data provider._ 
```C++
typedef struct tirexDataProvider_st tirexDataProvider;
```



Data providers are modules tasked with collecting associated metrics about the program. 


        

<hr>



### typedef tirexError 

_Categorizes different classes of error._ 
```C++
typedef enum tirexError_enum tirexError;
```




<hr>



### enum tirexError\_enum 

_Categorizes different classes of error._ 
```C++
enum tirexError_enum {
    TIREX_SUCCESS = 0,
    TIREX_INVALID_ARGUMENT = 1
};
```




<hr>



### typedef tirexLogCallback 

_Points to a function that takes in a log level, component, and message which should be logged._ 
```C++
typedef void(* tirexLogCallback) (tirexLogLevel level, const char *component, const char *message);
```



The log callback can handle the call as it likes (including ignoring it for example because logging is set to a lower verbosity or turned of). The `component` may be used to identify, where the log is comming from. 


        

<hr>



### typedef tirexLogLevel 

_Represents different levels of verbosity._ 
```C++
typedef enum tirexLogLevel_enum tirexLogLevel;
```



The enum values are sorted by verbosity. That is, for example, if logs should only be logged `level` `INFO` or higher, then `level >= INFO` can be used to check what should be logged. 


        

<hr>



### enum tirexLogLevel\_enum 

_Represents different levels of verbosity._ 
```C++
enum tirexLogLevel_enum {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    CRITICAL
};
```



The enum values are sorted by verbosity. That is, for example, if logs should only be logged `level` `INFO` or higher, then `level >= INFO` can be used to check what should be logged. 


        

<hr>



### typedef tirexMeasure 

_Identifiers for all available measurements._ 
```C++
typedef enum tirexMeasure_enum tirexMeasure;
```



Different types (EnvInfo, Time Series, Measure) 


        

<hr>



### typedef tirexMeasureConf 

```C++
typedef struct tirexMeasureConf_st tirexMeasureConf;
```




<hr>



### typedef tirexMeasureHandle 

_Holds a handle to an ongoing measurement task._ 
```C++
typedef struct tirexMeasureHandle_st tirexMeasureHandle;
```




<hr>



### typedef tirexMeasureInfo 

```C++
typedef struct tirexMeasureInfo_st tirexMeasureInfo;
```




<hr>



### enum tirexMeasure\_enum 

_Identifiers for all available measurements._ 
```C++
enum tirexMeasure_enum {
    TIREX_OS_NAME = 0,
    TIREX_OS_KERNEL = 1,
    TIREX_TIME_ELAPSED_WALL_CLOCK_MS = 2,
    TIREX_TIME_ELAPSED_USER_MS = 3,
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
    TIREX_TIME_START = 44,
    TIREX_TIME_STOP = 45,
    TIREX_GIT_ROOT = 46,
    TIREX_GIT_ARCHIVE_PATH = 47,
    TIREX_VERSION_MEASURE = 48,
    TIREX_INVOCATION = 49,
    TIREX_DEVCONTAINER_CONF_PATHS = 50,
    TIREX_MEASURE_COUNT,
    TIREX_MEASURE_INVALID = -1
};
```



Different types (EnvInfo, Time Series, Measure) 


        

<hr>



### typedef tirexResult 

_Holds a handle to the results of a measurement._ 
```C++
typedef struct tirexResult_st tirexResult;
```



The measurement result is structed as a tree of categories, subcategories and (at the leaves) values.




**See also:** tirexStopTracking(tirexMeasureHandle\*, char\*\*) 



        

<hr>



### typedef tirexResultEntry 

_Holds a result entry, i.e., a key-value pair of a name (string) and a value (tirexResult)._ 
```C++
typedef struct tirexResultEntry_st tirexResultEntry;
```




<hr>



### typedef tirexResultType 

```C++
typedef enum tirexResultType_enum tirexResultType;
```



Note: Currently unused 


        

<hr>



### enum tirexResultType\_enum 

```C++
enum tirexResultType_enum {
    TIREX_STRING = 0,
    TIREX_INTEGER = 1,
    TIREX_FLOATING = 2,
    TIREX_BOOLEAN = 3,
    TIREX_WSTRING = 4,
    TIREX_LIST = 1 << 7,
    TIREX_STRING_LIST = TIREX_STRING | TIREX_LIST,
    TIREX_INTEGER_LIST = TIREX_INTEGER | TIREX_LIST,
    TIREX_FLOATING_LIST = TIREX_FLOATING | TIREX_LIST,
    TIREX_BOOLEAN_LIST = TIREX_BOOLEAN | TIREX_LIST,
    TIREX_WSTRING_LIST = TIREX_WSTRING | TIREX_LIST
};
```



Note: Currently unused 


        

<hr>
## Public Static Attributes Documentation




### variable tirexNullConf 

_Represents an invalid measurement configuration._ 
```C++
const tirexMeasureConf tirexNullConf;
```



The null configuration should be used as a sentinel value to mark the end of the configuration in tirexStartTracking. 


        

<hr>
## Public Functions Documentation




### function tirexDataProviderGetAll 

_Populates the given buffer with all available providers (or at most_ `bufsize` _if_`buf` _is not large enugh) and returns the total number of available providers._
```C++
TIREX_TRACKER_EXPORT size_t tirexDataProviderGetAll (
    tirexDataProvider * providers,
    size_t bufsize
) 
```



If `buf` is `NULL` , `bufsize` is ignored and only the number of available providers are returned. A typical call may look as follows: 
```C++
size_t num = tirexDataProviderGetAll(NULL, 0);
struct tirexDataProvider* buf = (struct tirexDataProvider*)calloc(num, sizeof(struct tirexDataProvider));
tirexDataProviderGetAll(buf, num);
// ...
free(buf);
```





**Parameters:**


* `providers` 
* `bufsize` 



**Returns:**

The number of available data providers. 





        

<hr>



### function tirexFetchInfo 

_Fetches the system information from the measures requested in_ `measures` _._
```C++
TIREX_TRACKER_EXPORT tirexError tirexFetchInfo (
    const tirexMeasureConf * measures,
    tirexResult ** result
) 
```





**Parameters:**


* `measures` 
* `result` 



**Returns:**

TIREX\_SUCCESS on success or an error code. 





        

<hr>



### function tirexMeasureInfoGet 

```C++
TIREX_TRACKER_EXPORT tirexError tirexMeasureInfoGet (
    tirexMeasure measure,
    const tirexMeasureInfo ** info
) 
```





**Parameters:**


* `measure` 
* `info` 



**Returns:**

TIREX\_SUCCESS on success or an error code. 





        

<hr>



### function tirexResultEntryGetByIndex 

```C++
TIREX_TRACKER_EXPORT tirexError tirexResultEntryGetByIndex (
    const tirexResult * result,
    size_t index,
    tirexResultEntry * entry
) 
```





**Parameters:**


* `result` 
* `index` 
* `entry` 



**Returns:**

TIREX\_SUCCESS on success or an error code. 





        

<hr>



### function tirexResultEntryNum 

_Returns the number of entries contained in the result set._ 
```C++
TIREX_TRACKER_EXPORT tirexError tirexResultEntryNum (
    const tirexResult * result,
    size_t * num
) 
```





**Parameters:**


* `result` 
* `num` 



**Returns:**

TIREX\_SUCCESS on success or an error code. 





        

<hr>



### function tirexResultFree 

_Deinitializes and frees the result pointed at by_ `result` _._
```C++
TIREX_TRACKER_EXPORT void tirexResultFree (
    tirexResult * result
) 
```





**Parameters:**


* `result` The result to be destructed. 




        

<hr>



### function tirexSetAbortCallback 

_Set a callback that should be used in case the program needs to abort._ 
```C++
TIREX_TRACKER_EXPORT void tirexSetAbortCallback (
    tirexAbortCallback callback
) 
```



Per default, [`abort()`](https://en.cppreference.com/w/c/program/abort.html) is used.




**Parameters:**


* `callback` 




        

<hr>



### function tirexSetAbortLevel 

```C++
TIREX_TRACKER_EXPORT void tirexSetAbortLevel (
    tirexLogLevel level
) 
```




<hr>



### function tirexSetLogCallback 

_Set a callback that should be used for all future logs._ 
```C++
TIREX_TRACKER_EXPORT void tirexSetLogCallback (
    tirexLogCallback callback
) 
```



Per default, nothing is logged and passing NULL for `callback` disables logging.




**Parameters:**


* `callback` Points to a function that takes in a log level, component, and message which should be logged. Pass NULL to disable logging. 




        

<hr>



### function tirexStartTracking 

_Initializes the providers set in the configuration and starts measuring._ 
```C++
TIREX_TRACKER_EXPORT tirexError tirexStartTracking (
    const tirexMeasureConf * measures,
    size_t pollIntervalMs,
    tirexMeasureHandle ** handle
) 
```





**Parameters:**


* `measures` 
* `pollIntervalMs` 
* `handle` a handle to the running measurement. 



**Returns:**

TIREX\_SUCCESS on success or an error code.




**See also:** [**tirexStopTracking**](tirex__tracker_8h.md#function-tirexstoptracking) 



        

<hr>



### function tirexStopTracking 

_Stops the measurement and deinitializes the data providers._ 
```C++
TIREX_TRACKER_EXPORT tirexError tirexStopTracking (
    tirexMeasureHandle * handle,
    tirexResult ** result
) 
```



This function **must** be called **exactly once** for each measurement job.




**Parameters:**


* `handle` The handle of the running measurement that should be stopped. 
* `result` a handle to the result tree of the measurement. Must be freed by the caller using [**tirexResultFree(tirexResult\*)**](tirex__tracker_8h.md#function-tirexresultfree) 



**Returns:**

TIREX\_SUCCESS on success or an error code.




**See also:** [**tirexStartTracking**](tirex__tracker_8h.md#function-tirexstarttracking) 



        

<hr>
## Macro Definition Documentation





### define TIREX\_AGG\_MAX 

```C++
#define TIREX_AGG_MAX `(1 << 2)`
```



For each aggregation intervall, store the maximum value. 


        

<hr>



### define TIREX\_AGG\_MEAN 

```C++
#define TIREX_AGG_MEAN `(1 << 4)`
```



For each aggregation intervall, store the average value. 


        

<hr>



### define TIREX\_AGG\_MIN 

```C++
#define TIREX_AGG_MIN `(1 << 3)`
```



For each aggregation intervall, store the minimum value. 


        

<hr>



### define TIREX\_AGG\_NO 

```C++
#define TIREX_AGG_NO `(1 << 1)`
```



Perform no aggregation. 


        

<hr>



### define static\_assert 

```C++
#define static_assert (
    cond,
    msg
) 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `c/include/tirex_tracker.h`

