

# Group measure



[**Modules**](modules.md) **>** [**measure**](group__measure.md)






















## Public Types

| Type | Name |
| ---: | :--- |
| typedef uint8\_t | [**tirexAggregateFn**](#typedef-tirexaggregatefn)  <br> |
| typedef enum [**tirexMeasure\_enum**](tirex__tracker_8h.md#enum-tirexmeasure_enum) | [**tirexMeasure**](#typedef-tirexmeasure)  <br>_Identifiers for all available measurements._  |
| typedef struct tirexMeasureHandle\_st | [**tirexMeasureHandle**](#typedef-tirexmeasurehandle)  <br>_Holds a handle to an ongoing measurement task._  |
| enum  | [**tirexMeasure\_enum**](#enum-tirexmeasure_enum)  <br>_Identifiers for all available measurements._  |




















## Public Functions

| Type | Name |
| ---: | :--- |
|  TIREX\_TRACKER\_EXPORT [**tirexError**](tirex__tracker_8h.md#typedef-tirexerror) | [**tirexStartTracking**](#function-tirexstarttracking) (const [**tirexMeasureConf**](tirex__tracker_8h.md#typedef-tirexmeasureconf) \* measures, size\_t pollIntervalMs, [**tirexMeasureHandle**](tirex__tracker_8h.md#typedef-tirexmeasurehandle) \*\* handle) <br>_Initializes the providers set in the configuration and starts measuring._  |
|  TIREX\_TRACKER\_EXPORT [**tirexError**](tirex__tracker_8h.md#typedef-tirexerror) | [**tirexStopTracking**](#function-tirexstoptracking) ([**tirexMeasureHandle**](tirex__tracker_8h.md#typedef-tirexmeasurehandle) \* handle, [**tirexResult**](tirex__tracker_8h.md#typedef-tirexresult) \*\* result) <br>_Stops the measurement and deinitializes the data providers._  |



























## Macros

| Type | Name |
| ---: | :--- |
| define  | [**TIREX\_AGG\_MAX**](tirex__tracker_8h.md#define-tirex_agg_max)  `(1 &lt;&lt; 2)`<br> |
| define  | [**TIREX\_AGG\_MEAN**](tirex__tracker_8h.md#define-tirex_agg_mean)  `(1 &lt;&lt; 4)`<br> |
| define  | [**TIREX\_AGG\_MIN**](tirex__tracker_8h.md#define-tirex_agg_min)  `(1 &lt;&lt; 3)`<br> |
| define  | [**TIREX\_AGG\_NO**](tirex__tracker_8h.md#define-tirex_agg_no)  `(1 &lt;&lt; 1)`<br> |

## Public Types Documentation




### typedef tirexAggregateFn 

```C++
typedef uint8_t tirexAggregateFn;
```




<hr>



### typedef tirexMeasure 

_Identifiers for all available measurements._ 
```C++
typedef enum tirexMeasure_enum tirexMeasure;
```



Different types (EnvInfo, Time Series, Measure) 


        

<hr>



### typedef tirexMeasureHandle 

_Holds a handle to an ongoing measurement task._ 
```C++
typedef struct tirexMeasureHandle_st tirexMeasureHandle;
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
## Public Functions Documentation




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

------------------------------


