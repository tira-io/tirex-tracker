

# Group logging



[**Modules**](modules.md) **>** [**logging**](group__logging.md)



_Functions and constants for logging._ [More...](#detailed-description)


















## Public Types

| Type | Name |
| ---: | :--- |
| typedef void(\* | [**tirexLogCallback**](#typedef-tirexlogcallback)  <br>_Points to a function that takes in a log level, component, and message which should be logged._  |
| typedef enum [**tirexLogLevel\_enum**](tirex__tracker_8h.md#enum-tirexloglevel_enum) | [**tirexLogLevel**](#typedef-tirexloglevel)  <br>_Represents different levels of verbosity._  |
| enum  | [**tirexLogLevel\_enum**](#enum-tirexloglevel_enum)  <br>_Represents different levels of verbosity._  |




















## Public Functions

| Type | Name |
| ---: | :--- |
|  TIREX\_TRACKER\_EXPORT void | [**tirexSetLogCallback**](#function-tirexsetlogcallback) ([**tirexLogCallback**](tirex__tracker_8h.md#typedef-tirexlogcallback) callback) <br>_Set a callback that should be used for all future logs._  |




























## Detailed Description


Definitions and functions relevant for telling measure where to log information to. 


    
## Public Types Documentation




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
## Public Functions Documentation




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

------------------------------


