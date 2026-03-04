

# Group dataprovider



[**Modules**](modules.md) **>** [**dataprovider**](group__dataprovider.md)



[More...](#detailed-description)
















## Classes

| Type | Name |
| ---: | :--- |
| struct | [**tirexDataProvider\_st**](structtirexDataProvider__st.md) <br>_Contains information about a data provider._  |


## Public Types

| Type | Name |
| ---: | :--- |
| typedef struct [**tirexDataProvider\_st**](structtirexDataProvider__st.md) | [**tirexDataProvider**](#typedef-tirexdataprovider)  <br>_Contains information about a data provider._  |




















## Public Functions

| Type | Name |
| ---: | :--- |
|  TIREX\_TRACKER\_EXPORT size\_t | [**tirexDataProviderGetAll**](#function-tirexdataprovidergetall) ([**tirexDataProvider**](tirex__tracker_8h.md#typedef-tirexdataprovider) \* providers, size\_t bufsize) <br>_Populates the given buffer with all available providers (or at most_ `bufsize` _if_`buf` _is not large enugh) and returns the total number of available providers._ |




























## Detailed Description


Methods and definitions necessary to fetch information about the underlying data sources. Dataproviders are the underlying mechanism for collecting the requested metadata and are abstracted away by the API. The following definitions can be used to get more information (e.g., to report detailed version information). 


    
## Public Types Documentation




### typedef tirexDataProvider 

_Contains information about a data provider._ 
```C++
typedef struct tirexDataProvider_st tirexDataProvider;
```



Data providers are modules tasked with collecting associated metrics about the program. 


        

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

------------------------------


