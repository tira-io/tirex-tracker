# Architecture

This document describes the internal structure of the TIREx Tracker C library. Understanding it is a prerequisite for [adding new measures](adding-measures.md) or contributing to the core.

## Overview

```
┌─────────────────────────────────────────┐
│            Public C API                 │  tirex_tracker.h
│  tirexStartTracking / tirexStopTracking │
│  tirexFetchInfo                         │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│         Measure API (measureapi.cpp)    │
│  tirexMeasureHandle_st                  │
│  • providers: vector<StatsProvider>     │
│  • monitorThread (polls step())         │
│  • signal: promise<void> (stop signal)  │
└──────────────┬──────────────────────────┘
               │  initializes and drives
               ▼
┌──────────────────────────────────────────────────────────┐
│                 StatsProvider (provider.hpp)              │
│                                                          │
│  start() ──► [measure starts]                           │
│  step()  ──► [called every pollIntervalMs]              │
│  stop()  ──► [measure ends]                             │
│  getStats() → Stats  (after stop)                       │
│  getInfo()  → Stats  (static info, no tracking needed)  │
└──────────┬───────────────────────────────────────────────┘
           │  implemented by
           ▼
┌──────────────────────────────────────────────────────────┐
│  Concrete providers (c/src/measure/stats/)               │
│                                                          │
│  SystemStats     — OS, CPU, RAM, timing, invocation      │
│  EnergyStats     — CPU / DRAM / GPU energy via PCM       │
│  GitStats        — Git metadata via libgit2              │
│  NVMLStats       — NVIDIA GPU via NVML                   │
│  DevContainerStats — devcontainer.json detection         │
└──────────────────────────────────────────────────────────┘
```

## The `StatsProvider` base class

Every measurement source implements the `tirex::StatsProvider` abstract class ([provider.hpp](https://github.com/tira-io/tirex-tracker/blob/master/c/src/measure/stats/provider.hpp)):

```cpp
class StatsProvider {
protected:
    std::set<tirexMeasure> enabled; // which measures this instance should collect

public:
    virtual std::set<tirexMeasure> providedMeasures() noexcept = 0;

    virtual void start() {}   // called once before the experiment begins
    virtual void step()  {}   // called every pollIntervalMs (may be many times)
    virtual void stop()  {}   // called once after the experiment ends

    virtual Stats getStats() { return {}; } // returns results after stop()
    virtual Stats getInfo()  { return {}; } // returns static info (no tracking)
};
```

The `enabled` field is populated by `requestMeasures()`, which filters the full `providedMeasures()` set down to only the measures the caller actually requested. Providers should check `enabled.contains(TIREX_...)` before collecting data they don't need.

## Result type: `Stats` and `StatVal`

```cpp
using StatVal = std::variant<
    std::string,                                        // plain text / numeric string
    nlohmann::json,                                     // structured JSON (e.g., cache sizes)
    TmpFile,                                            // a temporary file path (e.g., git archive)
    std::reference_wrapper<const TimeSeries<unsigned>>  // time-series data
>;
using Stats = std::map<tirexMeasure, StatVal>;
```

`StatVal` is a discriminated union that covers all value types the providers need to return. When `tirexStopTracking` is called, `Stats` from all providers is merged and converted to the `tirexResult_st` opaque result structure.

`TmpFile` is an RAII wrapper that auto-deletes a temporary file when the `tirexResult` is freed — this is how the Git archive (`TIREX_GIT_ARCHIVE_PATH`) is cleaned up automatically.

## Provider registry

All available providers are declared in a static map in [provider.cpp](https://github.com/tira-io/tirex-tracker/blob/master/c/src/measure/stats/provider.cpp):

```cpp
const std::map<std::string, ProviderEntry> tirex::providers = {
    {"system",       {SystemStats::constructor,      SystemStats::measures,  ...}},
    {"energy",       {EnergyStats::constructor,      EnergyStats::measures,  ...}},
    {"git",          {GitStats::constructor,         GitStats::measures,     ...}},
    {"gpu",          {NVMLStats::constructor,        NVMLStats::measures,    ...}},
    {"devcontainer", {DevContainerStats::constructor, ...,                   ...}},
};
```

Each `ProviderEntry` contains:
- `constructor` — a factory function (`std::function<unique_ptr<StatsProvider>()>`)
- `measures` — a `set<tirexMeasure>` of all measures this provider _can_ supply (used to decide which providers to instantiate)
- `version` — a human-readable version string for the provider's data sources
- `description` — a short description for display

When tracking starts, `tirex::initProviders()` uses the requested measures to determine the minimal set of providers that needs to be instantiated: a provider is created only if at least one of its declared measures is requested.

## Threading model

```
Main thread                    Monitor thread
───────────────────────────    ──────────────────────────────────────────
tirexStartTracking()
  provider->start() (each)
  launch monitorThread ──────► loop:
                                   provider->step() (each)
                                   wait(pollIntervalMs)
                               until signal.future is ready
tirexStopTracking()
  signal.set_value() ────────► (future becomes ready → loop exits)
  monitorThread.join() ◄─────── thread exits
  provider->stop() (each, reverse order)
  collect getStats()
```

The stop signal uses a `std::promise<void>` / `std::future<void>` pair. The monitor thread calls `future.wait_for(interval)` in a loop; when the promise is fulfilled by `stop()`, the wait returns `future_status::ready` and the loop terminates.

## Platform-specific code

Each provider that has platform-dependent behaviour uses compile-time conditionals:

- **Linux:** reads `/proc/[pid]/stat`, `/proc/meminfo`, etc.
- **macOS:** uses `sysctl`, `mach/task.h`, and IOKit.
- **Windows:** uses `GetProcessMemoryInfo`, `GlobalMemoryStatusEx`, `FILETIME`.

Platform-specific implementations for `SystemStats` live in separate source files:

```
c/src/measure/stats/
├── systemstats.cpp          # platform-agnostic code
├── systemstats_linux.cpp    # Linux implementation
├── systemstats_macos.cpp    # macOS implementation
└── systemstats_windows.cpp  # Windows implementation
```

## From `Stats` to `tirexResult`

After all providers have been stopped and their `getStats()` called, the combined `Stats` map is converted to the public `tirexResult_st` structure by `createMsrResultFromStats()` ([measureresult.cpp](https://github.com/tira-io/tirex-tracker/blob/master/c/src/measureresult.cpp)).

Each `StatVal` variant is converted to a string representation:

| `StatVal` type | Conversion |
|---|---|
| `std::string` | Used directly |
| `nlohmann::json` | `json.dump()` → JSON string |
| `TmpFile` | Path string; file is owned by the `tirexResult` |
| `TimeSeries<unsigned>` | Serialized as a JSON object with `max`, `min`, `avg`, and `timeseries` fields |

The caller receives an opaque `tirexResult*` and accesses entries via `tirexResultEntryGetByIndex`. When done, `tirexResultFree` releases all memory and deletes any owned `TmpFile` files.

## IR extension

The IR metadata export lives in [c/extensions/](../../c/extensions/) as an optional component that is compiled in alongside the main library (controlled by `TIREX_TRACKER_EXTENSION_IR`). It reads both a `tirexResult` from `tirexFetchInfo` (static hardware info) and a `tirexResult` from `tirexStopTracking` (runtime measurements), and serialises them into the `ir_metadata` YAML format.
