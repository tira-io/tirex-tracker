# C / C++

## Integration via CMake

The recommended way to consume the TIREx Tracker in a C/C++ project is via CMake's `FetchContent`:

```cmake
include(FetchContent)

FetchContent_Declare(
    tirex_tracker
    GIT_REPOSITORY https://github.com/tira-io/tirex-tracker.git
    GIT_TAG        0.0.11   # replace with the desired version tag
)
FetchContent_MakeAvailable(tirex_tracker)

target_link_libraries(<your_target> tirex_tracker::tirex_tracker)
```

Then include the header in your source files:

```c
#include <tirex_tracker.h>
```

## Minimal example

```c
#include <tirex_tracker.h>

int main() {
    /* 1. Configure which measures to track.
     *    The array must be terminated with tirexNullConf. */
    tirexMeasureConf conf[] = {
        {TIREX_TIME_ELAPSED_WALL_CLOCK_MS, TIREX_AGG_NO},
        {TIREX_CPU_USED_PROCESS_PERCENT,   TIREX_AGG_MAX},
        {TIREX_RAM_USED_PROCESS_KB,        TIREX_AGG_MAX},
        tirexNullConf   /* sentinel — do not omit */
    };

    /* 2. Start tracking. Poll every 100 ms for time-series measures. */
    tirexTrackingConf trackingConf = {.measures = conf, .pollIntervalMs = 100};
    tirexMeasureHandle* handle;
    tirexStartTracking(trackingConf, &handle);

    /* 3. Run your experiment. */
    run_experiment();

    /* 4. Stop tracking and collect results. */
    tirexResult* result;
    tirexStopTracking(handle, &result);

    /* 5. Inspect results. */
    size_t n;
    tirexResultEntryNum(result, &n);
    for (size_t i = 0; i < n; i++) {
        tirexResultEntry entry;
        tirexResultEntryGetByIndex(result, i, &entry);
        /* entry.source  → tirexMeasure enum value
         * entry.value   → null-terminated string (always)
         * entry.type    → tirexResultType for parsing */
    }

    /* 6. Free the result object (important — owns temporary files). */
    tirexResultFree(result);
    return 0;
}
```

## Querying system info (without tracking)

Use `tirexFetchInfo` to query static hardware information once, without starting a continuous monitoring loop:

```c
tirexMeasureConf info_conf[] = {
    {TIREX_CPU_MODEL_NAME,             TIREX_AGG_NO},
    {TIREX_CPU_AVAILABLE_SYSTEM_CORES, TIREX_AGG_NO},
    {TIREX_RAM_AVAILABLE_SYSTEM_MB,    TIREX_AGG_NO},
    {TIREX_OS_NAME,                    TIREX_AGG_NO},
    tirexNullConf
};

tirexResult* info;
tirexFetchInfo((tirexTrackingConf){.measures = info_conf}, &info);
/* ... inspect info ... */
tirexResultFree(info);
```

## Tracking a subprocess

By default, `tirexTrackingConf.pid` is `TIREX_PID_SELF`, so tracking targets the calling process like in the examples
above. To track a different process instead, spawn it first (so you have its PID) and pass that PID explicitly. Setting
`trackSubprocesses` additionally aggregates whatever that process itself goes on to spawn:

```c
#include <tirex_tracker.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t child = fork();
    if (child == 0) {
        execlp("some-command", "some-command", NULL);
        _exit(127); /* execlp only returns on failure */
    }

    tirexMeasureConf conf[] = {
        {TIREX_CPU_USED_PROCESS_PERCENT, TIREX_AGG_MAX},
        {TIREX_RAM_USED_PROCESS_KB,      TIREX_AGG_MAX},
        tirexNullConf
    };
    tirexTrackingConf trackingConf = {
        .measures = conf,
        .pid = child,               /* track the child, not ourselves */
        .trackSubprocesses = true,  /* also aggregate anything it spawns */
        .pollIntervalMs = 100,
    };
    tirexMeasureHandle* handle;
    tirexStartTracking(trackingConf, &handle);

    int status;
    waitpid(child, &status, 0);

    tirexResult* result;
    tirexStopTracking(handle, &result);
    /* ... inspect result ... */
    tirexResultFree(result);
    return 0;
}
```

RAM aggregation across a process tree is an approximation whose accuracy depends on the platform: Linux sums each
process's PSS (proportional set size, from `/proc/<pid>/smaps_rollup`), so pages shared with processes *outside* the
tracked tree are still divided by their global mapper count. macOS sums each process's `ri_phys_footprint` (via
`proc_pid_rusage`), a per-process estimate with no cross-process dedup. Windows sums each process's `PrivateUsage`
(`PROCESS_MEMORY_COUNTERS_EX`). CPU aggregation has no such caveat — CPU time is always additive across a tree on every
platform. See [Tracked Measures](../guides/measures.md) for details.

## Listing available measures at runtime

```c
#include <tirex_tracker.h>
#include <stdio.h>

int main() {
    /* Get provider information. */
    size_t num_providers = tirexDataProviderGetAll(NULL, 0);
    tirexDataProvider* providers = malloc(num_providers * sizeof(tirexDataProvider));
    tirexDataProviderGetAll(providers, num_providers);

    for (size_t i = 0; i < num_providers; i++) {
        printf("Provider: %s\n  %s\n", providers[i].name, providers[i].description);
    }
    free(providers);

    /* Get info for a specific measure. */
    const tirexMeasureInfo* info;
    tirexMeasureInfoGet(TIREX_CPU_USED_PROCESS_PERCENT, &info);
    printf("Description: %s\nExample: %s\n", info->description, info->example);
    return 0;
}
```

## Logging

Register a callback to receive log messages from the tracker:

```c
#include <tirex_tracker.h>
#include <stdio.h>

void my_log(tirexLogLevel level, const char* component, const char* message) {
    printf("[%d][%s] %s\n", level, component, message);
}

/* Call before any other tirex function. */
tirexSetLogCallback(my_log);
```

## IR Metadata export

To export results in the [`ir_metadata`](../guides/ir-metadata.md) format, include the IR extension and call `tirexResultExportIrMetadata`:

```c
#include <tirex_tracker.h>
#include <irtracker.h>

tirexResult* info   = /* fetch system info */;
tirexResult* result = /* tracking result */;

tirexResultExportIrMetadata(info, result, "ir_metadata.yml");

tirexResultFree(result);
tirexResultFree(info);
```

Enable the IR extension in your `CMakeLists.txt`:

```cmake
set(TIREX_TRACKER_EXTENSION_IR ON)
FetchContent_MakeAvailable(tirex_tracker)
```

## C++ convenience

The C API is fully usable from C++ code. Additionally, consider using a simple RAII wrapper to avoid forgetting `tirexResultFree`:

```cpp
#include <tirex_tracker.h>
#include <memory>

struct TirexResultDeleter {
    void operator()(tirexResult* r) const { tirexResultFree(r); }
};
using TirexResultPtr = std::unique_ptr<tirexResult, TirexResultDeleter>;

// Usage:
TirexResultPtr result;
{
    tirexResult* raw = nullptr;
    tirexStopTracking(handle, &raw);
    result.reset(raw);
}
// result is automatically freed when it goes out of scope.
```

## API reference

See the [C/C++ API reference](../api/c.md) for the complete function and type documentation generated from Doxygen comments.
