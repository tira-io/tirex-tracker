# Java / Kotlin API Reference

Full Javadoc for the JVM library is hosted at:

**[javadoc.io/doc/io.tira/tirex-tracker](https://javadoc.io/doc/io.tira/tirex-tracker)**

---

For installation and usage examples, see the [Java / Kotlin getting started guide](../getting-started/java.md).

---

## Quick reference

### Kotlin

| Function / Class | Description |
|---|---|
| `track(measures, pollIntervalMs) { ... }` | Track a lambda block and return a result map. |
| `tracking(measures, pollIntervalMs)` | Returns a `TrackingHandle` for use with `use { }`. |
| `startTracking(measures, pollIntervalMs)` | Start tracking manually; returns a `TrackingHandle`. |
| `stopTracking(handle)` | Stop tracking and return the result map. |
| `fetchInfo(measures)` | Query static hardware info without tracking. |
| `setLogCallback { level, component, message -> }` | Register a log callback. |
| `providerInfos` | Returns a list of all available data providers. |
| `measureInfos` | Returns a list of all available measures with metadata. |

### Java (`Tracker` class)

| Method | Description |
|---|---|
| `Tracker.track(measures, pollIntervalMs, callable)` | Track a `Callable` or lambda. |
| `new Tracked(measures, pollIntervalMs)` | Try-with-resources handle. |
| `Tracker.startTracking(measures, pollIntervalMs)` | Start tracking manually. |
| `Tracker.stopTracking(handle)` | Stop tracking and return results. |
| `Tracker.fetchInfo(measures)` | Query static hardware info. |
| `Tracker.setLogCallback(callback)` | Register a log callback. |
| `Tracker.getProviderInfos()` | List available data providers. |
| `Tracker.getMeasureInfos()` | List available measures with metadata. |

---

## Key classes

### `Measure` enum

All ~51 platform measures (matching the C API) plus Java-specific measures:

```kotlin
Measure.TIME_ELAPSED_WALL_CLOCK_MS
Measure.CPU_USED_PROCESS_PERCENT
Measure.RAM_USED_PROCESS_KB
Measure.GPU_ENERGY_SYSTEM_JOULES
Measure.GIT_HASH
// ...
Measure.JAVA_VERSION       // JVM-only
Measure.JAVA_VM_NAME       // JVM-only
```

### `ResultEntry` data class

```kotlin
data class ResultEntry(
    val source: Measure,
    val value:  String,      // always a string; parse based on type
    val type:   ResultType,
)
```

### `Aggregation` enum

```kotlin
Aggregation.NO    // full time series
Aggregation.MAX
Aggregation.MIN
Aggregation.MEAN
```

Aggregation values can be combined using the `or` infix operator in Kotlin:
`Aggregation.MAX or Aggregation.MEAN`.

### `ProviderInfo` data class

```kotlin
data class ProviderInfo(
    val name:        String,
    val description: String,
    val version:     String?,
)
```

### `MeasureInfo` data class

```kotlin
data class MeasureInfo(
    val description: String,
    val dataType:    ResultType,
    val example:     String,
)
```

---

## Export to IR Metadata

=== "Kotlin"

    ```kotlin
    import io.tira.tirex.tracker.*

    val info   = fetchInfo(allMeasures)
    val result = track { runExperiment() }

    exportIrMetadata(info, result, "ir_metadata.yml")
    ```

=== "Java"

    ```java
    var info   = Tracker.fetchInfo(allMeasures);
    var result = Tracker.track(() -> runExperiment());

    Tracker.exportIrMetadata(info, result, "ir_metadata.yml");
    ```

See the [IR Metadata guide](../guides/ir-metadata.md) for the output format.
