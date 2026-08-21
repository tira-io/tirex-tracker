# C / C++ API Reference

The C API is declared in a single header: `tirex_tracker.h`. Include it in your C or C++ project and link against the `tirex_tracker` shared library.

For setup and usage examples, see the [C/C++ getting started guide](../getting-started/c.md).

---

## Core API functions

| Function | Description |
|---|---|
| `tirexStartTracking(conf, handle)` | Start a tracking session as configured by `conf`. |
| `tirexStopTracking(handle, result)` | Stop tracking and collect results into `*result`. |
| `tirexFetchInfo(conf, result)` | Query static hardware/process info without continuous tracking. |
| `tirexResultEntryGetByIndex(result, i, entry)` | Get the i-th result entry. |
| `tirexResultEntryNum(result, n)` | Get the number of result entries. |
| `tirexResultFree(result)` | Free a result object (and any owned temporary files). |
| `tirexDataProviderGetAll(buf, bufsize)` | List all available data providers. |
| `tirexMeasureInfoGet(measure, info)` | Get description and example for a specific measure. |
| `tirexSetLogCallback(callback)` | Register a log callback. |
| `tirexSetAbortCallback(callback)` | Register an abort callback. |
| `tirexSetAbortLevel(level)` | Set the log level at which the abort callback is invoked. |

---

## Key types

### `tirexMeasureConf`

Configuration for a single measure. Combine into a null-terminated array:

```c
tirexMeasureConf conf[] = {
    {TIREX_TIME_ELAPSED_WALL_CLOCK_MS, TIREX_AGG_NO},
    {TIREX_CPU_USED_PROCESS_PERCENT,   TIREX_AGG_MAX},
    tirexNullConf   // sentinel — required
};
```

### `tirexTrackingConf`

Configures a tracking session (`tirexStartTracking`) or one-shot info fetch (`tirexFetchInfo`): which measures to
collect, which process (and optionally its descendants) to target, and how often to poll:

```c
typedef struct tirexTrackingConf_st {
    const tirexMeasureConf* measures;  // tirexNullConf-terminated array of measures
    int64_t pid;                       // TIREX_PID_SELF (default) or an explicit target PID
    bool trackSubprocesses;            // if true, also track pid's descendants
    size_t pollIntervalMs;             // polling interval; ignored by tirexFetchInfo
} tirexTrackingConf;
```

A zero-initialized `tirexTrackingConf` tracks the calling process alone,`TIREX_PID_SELF`, is defined as `0`, which is
never a real process ID (reserved for kernel/scheduler bookkeeping on POSIX and the System Idle Process on Windows), so
this is safe by construction:

```c
tirexTrackingConf conf = {.measures = measures, .pollIntervalMs = 100};
```

To target a different, already-known process (e.g. a spawned subprocess), set `pid` explicitly. Setting
`trackSubprocesses = true` additionally discovers and aggregates that process's own descendants at each poll, for both
`TIREX_CPU_USED_PROCESS_PERCENT` and `TIREX_RAM_USED_PROCESS_KB`:

```c
tirexTrackingConf conf = {
    .measures = measures,
    .pid = child_pid,
    .trackSubprocesses = true,
    .pollIntervalMs = 100,
};
```

CPU time is additive across a process tree on every platform, so tree-mode CPU is exact. RAM is not simply additive;
summing raw resident memory would double-count pages shared between processes (shared libraries, `fork()`-inherited
copy-on-write pages) — so each platform instead reports its own best-available approximation:

| Platform | Technique | Known imprecision |
|---|---|---|
| Linux | Sum of each process's PSS (`/proc/<pid>/smaps_rollup`) | May slightly undercount pages also shared with processes *outside* the tracked tree |
| macOS | Sum of each process's `ri_phys_footprint` (`proc_pid_rusage`) | Not a fair-share scheme — can overcount memory the tree's own members share with each other |
| Windows | Sum of each process's `PrivateUsage` (`PROCESS_MEMORY_COUNTERS_EX`) | Excludes all shared memory, so it undercounts memory shared *within* the tree; tracks committed virtual memory rather than resident physical memory |

Some measures depend on which process is targeted — `TIREX_INVOCATION` and the git- and devcontainer-related measures
are resolved relative to `conf.pid`'s working directory, for both `tirexFetchInfo` and `tirexStartTracking`.

### `tirexMeasure` enum

All ~51 measure constants are prefixed with `TIREX_`. See [Tracked Measures](../guides/measures.md) for descriptions and examples.

### `tirexAggregation` flags

```c
TIREX_AGG_NO    // no aggregation (returns full time series)
TIREX_AGG_MAX   // maximum value
TIREX_AGG_MIN   // minimum value
TIREX_AGG_MEAN  // arithmetic mean
```

Flags can be OR-combined: `TIREX_AGG_MAX | TIREX_AGG_MEAN`.

### `tirexResultType` enum

Indicates how to interpret a `tirexResultEntry.value` string:

```c
TIREX_STRING    // plain UTF-8 string
TIREX_INTEGER   // parse as integer
TIREX_FLOATING  // parse as floating-point number
TIREX_BOOLEAN   // "0" or "1"
TIREX_STRING_LIST   // JSON array of strings
TIREX_INTEGER_LIST  // JSON array of integers
```

### `tirexLogLevel` enum

```c
TIREX_LOG_TRACE
TIREX_LOG_DEBUG
TIREX_LOG_INFO
TIREX_LOG_WARN
TIREX_LOG_ERROR
TIREX_LOG_CRITICAL
```

---

## IR extension

When the library is compiled with `TIREX_TRACKER_EXTENSION_IR ON`, the additional header `irtracker.h` provides:

```c
tirexError tirexResultExportIrMetadata(
    const tirexResult* info,    // from tirexFetchInfo()
    const tirexResult* result,  // from tirexStopTracking()
    const char*        filepath
);
```

See the [IR Metadata guide](../guides/ir-metadata.md) for the output format.

---

## Generated reference

The full API reference — all functions, types, enums, and macros extracted from Doxygen comments in the source headers — is available in the generated C API documentation linked in the navigation.
