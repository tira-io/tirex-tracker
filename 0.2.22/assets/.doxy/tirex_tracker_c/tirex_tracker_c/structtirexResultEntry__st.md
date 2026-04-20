

# Struct tirexResultEntry\_st



[**ClassList**](annotated.md) **>** [**tirexResultEntry\_st**](structtirexResultEntry__st.md)



_Holds a result entry, i.e., a key-value pair of a name (string) and a value (tirexResult)._ 

* `#include <tirex_tracker.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  [**tirexMeasure**](tirex__tracker_8h.md#typedef-tirexmeasure) | [**source**](#variable-source)  <br>_The measure that the result is associated with._  |
|  [**tirexResultType**](tirex__tracker_8h.md#typedef-tirexresulttype) | [**type**](#variable-type)  <br>_The datatype of the stored result._  |
|  const void \* | [**value**](#variable-value)  <br>_The value of the result (the type depends on_ [_**tirexResultEntry\_st::type**_](structtirexResultEntry__st.md#variable-type) _)._ |












































## Public Attributes Documentation




### variable source 

_The measure that the result is associated with._ 
```C++
tirexMeasure tirexResultEntry_st::source;
```




<hr>



### variable type 

_The datatype of the stored result._ 
```C++
tirexResultType tirexResultEntry_st::type;
```




<hr>



### variable value 

_The value of the result (the type depends on_ [_**tirexResultEntry\_st::type**_](structtirexResultEntry__st.md#variable-type) _)._
```C++
const void* tirexResultEntry_st::value;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `c/include/tirex_tracker.h`

