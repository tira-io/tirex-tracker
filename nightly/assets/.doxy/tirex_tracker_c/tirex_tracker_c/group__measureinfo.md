

# Group measureinfo



[**Modules**](modules.md) **>** [**measureinfo**](group__measureinfo.md)




















## Classes

| Type | Name |
| ---: | :--- |
| struct | [**tirexMeasureInfo\_st**](structtirexMeasureInfo__st.md) <br> |


## Public Types

| Type | Name |
| ---: | :--- |
| typedef struct [**tirexMeasureInfo\_st**](structtirexMeasureInfo__st.md) | [**tirexMeasureInfo**](#typedef-tirexmeasureinfo)  <br> |




















## Public Functions

| Type | Name |
| ---: | :--- |
|  TIREX\_TRACKER\_EXPORT [**tirexError**](tirex__tracker_8h.md#typedef-tirexerror) | [**tirexMeasureInfoGet**](#function-tirexmeasureinfoget) ([**tirexMeasure**](tirex__tracker_8h.md#typedef-tirexmeasure) measure, const [**tirexMeasureInfo**](tirex__tracker_8h.md#typedef-tirexmeasureinfo) \*\* info) <br> |




























## Public Types Documentation




### typedef tirexMeasureInfo 

```C++
typedef struct tirexMeasureInfo_st tirexMeasureInfo;
```




<hr>
## Public Functions Documentation




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

------------------------------


