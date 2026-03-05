# Adding a New Measure

This guide walks through adding a brand-new measurement to TIREx Tracker. There are two scenarios:

- **Adding to an existing provider** — the new measure fits naturally into an existing `StatsProvider` (e.g., adding a new CPU metric to `SystemStats`).
- **Creating a new provider** — the new measure requires a new data source (e.g., a new hardware sensor or external tool).

Read the [Architecture](architecture.md) page first if you haven't already.

---

## Step 1: Add the enum constant to the C header

Open [c/include/tirex_tracker.h](https://github.com/tira-io/tirex-tracker/blob/master/c/include/tirex_tracker.h) and add your measure to the `tirexMeasure` enum **before** `TIREX_MEASURE_INVALID`:

```c
typedef enum tirexMeasure {
    // ... existing measures ...

    TIREX_MY_NEW_MEASURE = 51,   // ← add here, with the next available integer

    TIREX_MEASURE_INVALID,
    TIREX_MEASURE_COUNT = TIREX_MEASURE_INVALID,
} tirexMeasure;
```

!!! warning "Enum values are part of the ABI"
    Never change the integer value of an existing measure. Always append new measures before `TIREX_MEASURE_INVALID`. Changing existing values would break binary compatibility with previously compiled code.

---

## Step 2: Add metadata in `measureinfo.cpp`

Open [c/src/measureinfo.cpp](https://github.com/tira-io/tirex-tracker/blob/master/c/src/measureinfo.cpp) and add an entry to the `measureInfos` array. The array is positionally indexed — the entry at index `N` describes measure `N`:

```cpp
static const tirexMeasureInfo measureInfos[] {
    // ... existing entries ...

    /* [TIREX_MY_NEW_MEASURE] = */
    {
        .description = "A one-sentence description of what this measures.",
        .datatype    = tirexResultType::TIREX_STRING,
        .example     = "example value",
    },
};
static_assert((sizeof(measureInfos) / sizeof(*measureInfos)) == TIREX_MEASURE_COUNT);
```

The `static_assert` at the bottom of the file will catch any mismatch between the enum size and the array size at compile time.

---

## Step 3a: Add to an existing provider

If your measure belongs to an existing provider (e.g., `SystemStats`), add it to the provider's `measures` static set and collect it in `getStats()` or `getInfo()`.

In the provider's header (e.g., `systemstats.hpp`), add the constant to the `measures` set:

```cpp
// systemstats.cpp (static initialization)
const std::set<tirexMeasure> SystemStats::measures = {
    // ... existing measures ...
    TIREX_MY_NEW_MEASURE,
};
```

Then collect the value in `getStats()` (for dynamic/runtime measures) or `getInfo()` (for static measures):

```cpp
Stats SystemStats::getInfo() {
    return makeFilteredStats(enabled,
        // ... existing pairs ...
        std::pair{TIREX_MY_NEW_MEASURE, std::string{"my value"}},
    );
}
```

Use `makeFilteredStats` to automatically filter to only the measures in `enabled` (i.e., those the caller actually requested).

For **dynamic** measures collected during `step()`, add a `TimeSeries` member to the class and push values in `step()`:

```cpp
// In the header, add a TimeSeries member:
tirex::TimeSeries<unsigned> myMetric =
    ts::store<unsigned>() | ts::Limit(300, TIREX_AGG_MAX) |
    ts::Batched(100ms, TIREX_AGG_MAX, 300);

// In step():
void SystemStats::step() {
    // ... existing collection ...
    unsigned value = /* read my metric */;
    if (enabled.contains(TIREX_MY_NEW_MEASURE))
        myMetric.push(value);
}

// In getStats():
Stats SystemStats::getStats() {
    return makeFilteredStats(enabled,
        // ... existing pairs ...
        std::pair{TIREX_MY_NEW_MEASURE, std::cref(myMetric)},
    );
}
```

---

## Step 3b: Create a new provider

If your measure requires a new data source, create a new `StatsProvider` subclass.

### Header (`c/src/measure/stats/myprovider.hpp`)

```cpp
#pragma once
#include "provider.hpp"

namespace tirex {
    class MyProvider final : public StatsProvider {
    public:
        std::set<tirexMeasure> providedMeasures() noexcept override;
        void start() override;
        void stop()  override;
        void step()  override;
        Stats getStats() override;
        Stats getInfo()  override;

        static constexpr const char* description = "Description of MyProvider.";
        static const char*            version;
        static const std::set<tirexMeasure> measures;
    };
} // namespace tirex
```

### Implementation (`c/src/measure/stats/myprovider.cpp`)

```cpp
#include "myprovider.hpp"

const std::set<tirexMeasure> tirex::MyProvider::measures = {
    TIREX_MY_NEW_MEASURE,
};

const char* tirex::MyProvider::version = "MyProvider 1.0";

std::set<tirexMeasure> tirex::MyProvider::providedMeasures() noexcept {
    return measures;
}

void tirex::MyProvider::start() {
    // initialize data source
}

void tirex::MyProvider::stop() {
    // finalize / close data source
}

void tirex::MyProvider::step() {
    if (enabled.contains(TIREX_MY_NEW_MEASURE)) {
        // poll value
    }
}

tirex::Stats tirex::MyProvider::getStats() {
    return makeFilteredStats(enabled,
        std::pair{TIREX_MY_NEW_MEASURE, std::string{"collected value"}},
    );
}
```

### Register in the provider registry (`c/src/measure/stats/provider.cpp`)

```cpp
const std::map<std::string, ProviderEntry> tirex::providers = {
    // ... existing entries ...
    {"myprovider", {
        .constructor = []() -> std::unique_ptr<StatsProvider> {
            return std::make_unique<MyProvider>();
        },
        .measures    = MyProvider::measures,
        .version     = MyProvider::version,
        .description = MyProvider::description,
    }},
};
```

### Add to the CMake source list (`c/src/CMakeLists.txt`)

```cmake
target_sources(tirex_tracker PRIVATE
    # ... existing sources ...
    measure/stats/myprovider.cpp
)
```

---

## Step 4: Expose in the Python wrapper

Open [python/tirex_tracker/_utils/constants.py](https://github.com/tira-io/tirex-tracker/blob/master/python/tirex_tracker/_utils/constants.py) and add the new enum member to `Measure`, using the same integer value as in the C header:

```python
class Measure(IntEnum):
    # ... existing members ...
    MY_NEW_MEASURE = 51
```

If the measure is **Python-only** (not provided by the C library), add it to `_PYTHON_MEASURES` in [python/tirex_tracker/_utils/library.py](https://github.com/tira-io/tirex-tracker/blob/master/python/tirex_tracker/_utils/library.py) instead:

```python
_PYTHON_MEASURES: Mapping[Measure, MeasureInfo] = {
    # ... existing entries ...
    Measure.MY_NEW_MEASURE: MeasureInfo(
        description="A one-sentence description.",
        data_type=ResultType.STRING,
        example='"example value"',
    ),
}
```

---

## Step 5: Expose in the JVM wrapper

Open [jvm/library/src/main/kotlin/io/tira/tirex/tracker/Tracker.kt](https://github.com/tira-io/tirex-tracker/blob/master/jvm/library/src/main/kotlin/io/tira/tirex/tracker/Tracker.kt) and add the constant to the `Measure` enum:

```kotlin
enum class Measure(val value: Int) {
    // ... existing entries ...
    MY_NEW_MEASURE(51),
}
```

---

## Step 6: Write a test

Add a test in [c/tests/tracker.cpp](https://github.com/tira-io/tirex-tracker/blob/master/c/tests/tracker.cpp) (using [Catch2](https://github.com/catchorg/Catch2)) to verify that the new measure is returned correctly:

```cpp
TEST_CASE("TIREX_MY_NEW_MEASURE is collected", "[tracker]") {
    tirexMeasureConf conf[] = {
        {TIREX_MY_NEW_MEASURE, TIREX_AGG_NO},
        tirexNullMeasureConf
    };
    tirexResult* result = nullptr;
    tirexTrackingHandle* handle = nullptr;
    REQUIRE(tirexStartTracking(conf, 100, &handle) == TIREX_SUCCESS);
    REQUIRE(tirexStopTracking(handle, &result) == TIREX_SUCCESS);

    size_t n = 0;
    tirexResultEntryNum(result, &n);
    REQUIRE(n == 1);

    tirexResultEntry entry;
    tirexResultEntryGetByIndex(result, 0, &entry);
    REQUIRE(entry.source == TIREX_MY_NEW_MEASURE);
    REQUIRE(entry.value != nullptr);

    tirexResultFree(result);
}
```

---

## Checklist

- [ ] Enum constant added to `tirex_tracker.h` (before `TIREX_MEASURE_INVALID`)
- [ ] Metadata entry added to `measureinfo.cpp` at the correct position
- [ ] Provider collects and returns the value (`getInfo()` or `getStats()`)
- [ ] Python `Measure` enum updated
- [ ] JVM `Measure` enum updated
- [ ] Test written and passing
- [ ] Doxygen comment added to the enum constant in the header
