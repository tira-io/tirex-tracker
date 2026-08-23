

# Struct tirexDataProvider\_st



[**ClassList**](annotated.md) **>** [**tirexDataProvider\_st**](structtirexDataProvider__st.md)



_Contains information about a data provider._ [More...](#detailed-description)

* `#include <tirex_tracker.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  const char \* | [**description**](#variable-description)  <br>_The humand legible description of the data provider._  |
|  const char \* | [**name**](#variable-name)  <br>_The human legible name of the data provider._  |
|  const char \* | [**version**](#variable-version)  <br>_A textual representation of the provider's version. May contain linefeeds. May be NULL._  |












































## Detailed Description


Data providers are modules tasked with collecting associated metrics about the program. 


    
## Public Attributes Documentation




### variable description 

_The humand legible description of the data provider._ 
```C++
const char* tirexDataProvider_st::description;
```




<hr>



### variable name 

_The human legible name of the data provider._ 
```C++
const char* tirexDataProvider_st::name;
```




<hr>



### variable version 

_A textual representation of the provider's version. May contain linefeeds. May be NULL._ 
```C++
const char* tirexDataProvider_st::version;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `c/include/tirex_tracker.h`

