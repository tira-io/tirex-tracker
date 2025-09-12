# TIREx Tracker &mdash; C API

To use the TIREx tracker in your C/C++ projects, first include this repository to your [CMake](https://cmake.org/) file as follows

<div class="tabbed">

- <b class="tab-title">CPM (Recommended)</b>
    If you use [CPM](https://github.com/cpm-cmake/CPM.cmake), you can add TIREx Tracker to your build as follows:
    ```cmake
    CPMAddPackage(URI "gh:tira-io/tirex-tracker#0.2.14")
    target_link_libraries(<yourtarget> tirex_tracker::tirex_tracker)
    ```
    ... and if you don't use CPM, you can of course use FetchContent but may want to have a look at CPM ;)
- <b class="tab-title">FetchContent</b>
    ```cmake
    include(FetchContent)
    # Use GIT_TAG to request the tag (or branch) you would like
    FetchContent_Declare(tirex_tracker GIT_REPOSITORY https://github.com/tira-io/tirex-tracker.git GIT_TAG 0.2.14)
    FetchContent_MakeAvailable(tirex_tracker)
    target_link_libraries(<yourtarget> tirex_tracker::tirex_tracker)
    ```
- <b class="tab-title">Manual</b>
    While we do not recommend this approach for general development, you may want to build the library separately and link it manually. To do so, first checkout the repository and build it with CMake. You can now link as usual using the files `tirex_tracker_export.h` and `libtirex_tracker.so` / `libtirex_tracker.dll` / `libtirex_tracker.dylib` (for Linux / Windows / macOS respectively) from the build directory and [`tirex_tracker.h`](https://github.com/tira-io/tirex-tracker/blob/master/c/include/tirex_tracker.h). We provide precompiled binaries for the major platforms [here](https://github.com/tira-io/tirex-tracker/releases/latest).

</div>

Measuring with TIREx Tracker is meant to be simple. This means that it generally consists of 4 steps: starting the tracking with `tirexStartTracking`, stopping the tracking with `tirexStopTracking`, using the information stored in the `tirexResult` structure, and, after you are done, freeing the results with `tirexResultFree`.

```c
#include <tirex_tracker.h>

int main() {
    // Configure measures to track.
    tirexMeasureConf conf[] = { 
        {TIREX_TIME_ELAPSED_WALL_CLOCK_MS, TIREX_AGG_NO},
        // Further measures...
        tirexNullMeasureConf // sentinel value
    };
    size_t pollIntervalMs = 100;
    tirexTrackingHandle* handle;
    tirexStartTracking(conf, pollIntervalMs, &handle);

    // Do something...

    tirexResult* result;
    tirexStopTracking(handle, &result);
    // Analyze the results.
    tirexResultFree(result);
}
```

<!-- TODO: ir_metadata export instructions. -->
