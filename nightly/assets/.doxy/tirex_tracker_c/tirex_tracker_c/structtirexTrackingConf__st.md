

# Struct tirexTrackingConf\_st



[**ClassList**](annotated.md) **>** [**tirexTrackingConf\_st**](structtirexTrackingConf__st.md)



_Configures a tracking session: which measures to collect, which process (and optionally its descendants) to measure, and how often to poll._ [More...](#detailed-description)

* `#include <tirex_tracker.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  const [**tirexMeasureConf**](tirex__tracker_8h.md#typedef-tirexmeasureconf) \* | [**measures**](#variable-measures)  <br>_tirexNullConf-terminated array of measures to track._  |
|  int64\_t | [**pid**](#variable-pid)  <br>_TIREX\_PID\_SELF (default) to track the calling process, or an explicit target process ID._  |
|  size\_t | [**pollIntervalMs**](#variable-pollintervalms)  <br>_The interval, in milliseconds, at which to poll for updated stats. Ignored by tirexFetchInfo, which does not poll._  |
|  bool | [**trackSubprocesses**](#variable-tracksubprocesses)  <br>_If true, also discover and aggregate the descendants of_ `pid` _over the tracking session, instead of measuring_`pid` _alone._ |












































## Detailed Description


For a minimal config, use`tirexTrackingConf{.measures = ..., .pollIntervalMs = 100}` to track the current process without its children. 


    
## Public Attributes Documentation




### variable measures 

_tirexNullConf-terminated array of measures to track._ 
```C++
const tirexMeasureConf* tirexTrackingConf_st::measures;
```




<hr>



### variable pid 

_TIREX\_PID\_SELF (default) to track the calling process, or an explicit target process ID._ 
```C++
int64_t tirexTrackingConf_st::pid;
```




<hr>



### variable pollIntervalMs 

_The interval, in milliseconds, at which to poll for updated stats. Ignored by tirexFetchInfo, which does not poll._ 
```C++
size_t tirexTrackingConf_st::pollIntervalMs;
```




<hr>



### variable trackSubprocesses 

_If true, also discover and aggregate the descendants of_ `pid` _over the tracking session, instead of measuring_`pid` _alone._
```C++
bool tirexTrackingConf_st::trackSubprocesses;
```



Applies to both TIREX\_CPU\_USED\_PROCESS\_PERCENT and TIREX\_RAM\_USED\_PROCESS\_KB, summed across `pid` and every descendant process discovered at each poll. CPU time is simply additive (never shared between processes), so that figure is exact. RAM cannot simply be summed (that would double-count pages shared between processes, e.g. shared libraries or fork()-inherited copy-on-write pages), so each platform instead uses its own best-available deduplicated approximation: PSS on Linux (divides each shared page's cost by its global mapper count; may slightly undercount pages also shared with processes outside the tracked tree), ri\_phys\_footprint on macOS (Apple's own per-process accounting, but not a fair-share scheme; can overcount memory the tree's own members share with each other), and PrivateUsage on Windows (excludes all shared memory entirely, so it cannot double-count but does undercount by any memory shared within the tree; also tracks committed virtual memory rather than strictly resident physical memory, unlike the single-process figure). 


        

<hr>

------------------------------
The documentation for this class was generated from the following file `c/include/tirex_tracker.h`

