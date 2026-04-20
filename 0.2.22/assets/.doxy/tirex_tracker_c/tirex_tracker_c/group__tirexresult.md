

# Group tirexresult



[**Modules**](modules.md) **>** [**tirexresult**](group__tirexresult.md)




















## Classes

| Type | Name |
| ---: | :--- |
| struct | [**tirexResultEntry\_st**](structtirexResultEntry__st.md) <br>_Holds a result entry, i.e., a key-value pair of a name (string) and a value (tirexResult)._  |


## Public Types

| Type | Name |
| ---: | :--- |
| typedef struct tirexResult\_st | [**tirexResult**](#typedef-tirexresult)  <br>_Holds a handle to the results of a measurement._  |
| typedef struct [**tirexResultEntry\_st**](structtirexResultEntry__st.md) | [**tirexResultEntry**](#typedef-tirexresultentry)  <br>_Holds a result entry, i.e., a key-value pair of a name (string) and a value (tirexResult)._  |
| typedef enum [**tirexResultType\_enum**](tirex__tracker_8h.md#enum-tirexresulttype_enum) | [**tirexResultType**](#typedef-tirexresulttype)  <br> |
| enum  | [**tirexResultType\_enum**](#enum-tirexresulttype_enum)  <br> |




















## Public Functions

| Type | Name |
| ---: | :--- |
|  TIREX\_TRACKER\_EXPORT [**tirexError**](tirex__tracker_8h.md#typedef-tirexerror) | [**tirexResultEntryGetByIndex**](#function-tirexresultentrygetbyindex) (const [**tirexResult**](tirex__tracker_8h.md#typedef-tirexresult) \* result, size\_t index, [**tirexResultEntry**](tirex__tracker_8h.md#typedef-tirexresultentry) \* entry) <br> |
|  TIREX\_TRACKER\_EXPORT [**tirexError**](tirex__tracker_8h.md#typedef-tirexerror) | [**tirexResultEntryNum**](#function-tirexresultentrynum) (const [**tirexResult**](tirex__tracker_8h.md#typedef-tirexresult) \* result, size\_t \* num) <br>_Returns the number of entries contained in the result set._  |
|  TIREX\_TRACKER\_EXPORT void | [**tirexResultFree**](#function-tirexresultfree) ([**tirexResult**](tirex__tracker_8h.md#typedef-tirexresult) \* result) <br>_Deinitializes and frees the result pointed at by_ `result` _._ |



























## Macros

| Type | Name |
| ---: | :--- |
| define  | [**static\_assert**](tirex__tracker_8h.md#define-static_assert) (cond, msg) <br> |

## Public Types Documentation




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
## Public Functions Documentation




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
## Macro Definition Documentation





### define static\_assert 

```C++
#define static_assert (
    cond,
    msg
) 
```




<hr>

------------------------------


