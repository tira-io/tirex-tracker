

# Group error



[**Modules**](modules.md) **>** [**error**](group__error.md)



_Functions and constants for error handling._ 


















## Public Types

| Type | Name |
| ---: | :--- |
| typedef void(\* | [**tirexAbortCallback**](#typedef-tirexabortcallback)  <br> |
| typedef enum [**tirexError\_enum**](tirex__tracker_8h.md#enum-tirexerror_enum) | [**tirexError**](#typedef-tirexerror)  <br>_Categorizes different classes of error._  |
| enum  | [**tirexError\_enum**](#enum-tirexerror_enum)  <br>_Categorizes different classes of error._  |




















## Public Functions

| Type | Name |
| ---: | :--- |
|  TIREX\_TRACKER\_EXPORT void | [**tirexSetAbortCallback**](#function-tirexsetabortcallback) ([**tirexAbortCallback**](tirex__tracker_8h.md#typedef-tirexabortcallback) callback) <br>_Set a callback that should be used in case the program needs to abort._  |
|  TIREX\_TRACKER\_EXPORT void | [**tirexSetAbortLevel**](#function-tirexsetabortlevel) ([**tirexLogLevel**](tirex__tracker_8h.md#typedef-tirexloglevel) level) <br> |




























## Public Types Documentation




### typedef tirexAbortCallback 

```C++
typedef void(* tirexAbortCallback) (const char *message);
```




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
## Public Functions Documentation




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

------------------------------


