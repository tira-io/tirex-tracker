# Aggregation

Some measures — primarily CPU usage, RAM usage, GPU utilization, and CPU frequency — are **dynamic**: they change during the lifetime of the tracked process. TIREx Tracker samples these values at a configurable poll interval and stores them in an internal time series.

When tracking stops, each time-series measure is reduced to a summary according to an **aggregation function**.

## Aggregation functions

| Constant (C) | Python | JVM | Description |
|---|---|---|---|
| `TIREX_AGG_NO` | `Aggregation.NO` | `Aggregation.NO` | No aggregation; the full time series is included in the result. |
| `TIREX_AGG_MAX` | `Aggregation.MAX` | `Aggregation.MAX` | Maximum value observed during tracking. |
| `TIREX_AGG_MIN` | `Aggregation.MIN` | `Aggregation.MIN` | Minimum value observed during tracking. |
| `TIREX_AGG_MEAN` | `Aggregation.MEAN` | `Aggregation.MEAN` | Mean (average) value over the tracking period. |

Aggregation functions are **bit flags** and can be combined: requesting `MAX | MIN` returns both the maximum and the minimum for the same measure.

## Result format

Dynamic measures always return a JSON object, regardless of which aggregation functions are selected:

```json
{
  "max": 117,
  "min": 0,
  "avg": 55,
  "timeseries": {
    "timestamps": ["100ms", "200ms", "300ms"],
    "values":     [23,      117,      89]
  }
}
```

Fields are omitted when they are not requested:

- If you request only `TIREX_AGG_MAX`, the result contains only `"max"`.
- If you request `TIREX_AGG_NO`, the result contains only `"timeseries"`.
- The time series is always stored internally and is always available in the raw result, but may be filtered by the library depending on the configuration.

## Static measures and aggregation

Static measures (hardware specs, Git state, timestamps) are collected once and are not affected by aggregation. Pass `TIREX_AGG_NO` for these:

```c
tirexMeasureConf conf[] = {
    {TIREX_OS_NAME,                    TIREX_AGG_NO},  // static — aggregation ignored
    {TIREX_CPU_USED_PROCESS_PERCENT,   TIREX_AGG_MAX}, // dynamic — take peak value
    {TIREX_RAM_USED_PROCESS_KB,        TIREX_AGG_MAX | TIREX_AGG_MEAN}, // both peak and average
    tirexNullMeasureConf
};
```

## Configuring aggregation

=== "C"

    ```c
    tirexMeasureConf conf[] = {
        {TIREX_CPU_USED_PROCESS_PERCENT, TIREX_AGG_MAX},
        {TIREX_RAM_USED_PROCESS_KB,      TIREX_AGG_MAX | TIREX_AGG_MEAN},
        {TIREX_CPU_FREQUENCY_MHZ,        TIREX_AGG_NO}, // full time series
        tirexNullMeasureConf
    };
    ```

## Poll interval and time series length

The poll interval controls how frequently dynamic measures are sampled:

- A **shorter interval** (e.g., 50 ms) gives finer-grained time series and better peak detection, at the cost of slightly more overhead.
- A **longer interval** (e.g., 1000 ms) reduces overhead and is appropriate for long-running experiments where per-second granularity is sufficient.

The time series is capped internally at 300 data points per measure. When the cap is reached, older data points are batched (using the configured aggregation function) to make room for new ones.

Default poll interval: **100 ms**.
