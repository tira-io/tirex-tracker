# IR Metadata Extension

TIREx Tracker can export results in the [`ir_metadata`](https://ir-metadata.org/) format — a YAML-based standard for documenting information retrieval experiments.

The current `ir_metadata` specification (version 0.1) does not cover all the hardware and software metadata that TIREx Tracker collects. TIREx Tracker therefore proposes an extended format (**`ir_metadata` 0.2-beta**) that adds fields for resource usage, energy consumption, and software environment.

## Exporting results

=== "CLI"

    ```shell
    tirex-tracker --format irmetadata -o ir_metadata.yml "python train.py"
    ```

=== "Python"

    ```python
    from tirex_tracker import ExportFormat, tracking

    with tracking(
        export_format=ExportFormat.IR_METADATA,
        export_file_path="ir_metadata.yml"
    ) as results:
        train_model()
    ```

=== "Java"

    ```java
    // Export after stopping
    Tracker.exportIrMetadata(info, result, "ir_metadata.yml");
    ```

=== "Kotlin"

    ```kotlin
    exportIrMetadata(info, result, "ir_metadata.yml")
    ```

=== "C"

    Link against the IR extension (`TIREX_TRACKER_EXTENSION_IR ON`) and call:

    ```c
    #include <irtracker.h>

    tirexResultExportIrMetadata(info, result, "ir_metadata.yml");
    ```

## Output structure

The exported YAML file follows the `ir_metadata` 0.2-beta format. Below is an example for a typical experiment:

```yaml
# ir_metadata 0.2-beta
schema version: 0.2
implementation:
  executable:
    cmd:
    - python3
    - script.py
  python:
    interactive: true
    modules: 3.12.12
    packages:
    - ...
  source:
    is repo: 0
    lang: null
platform:
  hardware:
    cpu:
      architecture: x86_64
      byte order: Little Endian
      caches:
        l1d: 32 KiB
        l1i: 32 KiB
        l2: 256 KiB
        l3: 56320 KiB
      features: rdtsc rdtscp fxsave xsave fpu mmx mmx_plus prefetchw daz sse sse2
        sse3 ssse3 sse4_1 sse4_2 avx fma3 f16c avx2 hle rtm xtest cmov cmpxchg8b cmpxchg16b
        movbe lahf_sahf lzcnt popcnt bmi bmi2 adx aes pclmulqdq rdrand rdseed
      frequency:
        avg: 0
        max: 0
        min: 0
        timeseries:
          timestamps:
          - 1ms
          - 101ms
          values:
          - 0
          - 0
      frequency max: 0 MHz
      frequency min: 0 MHz
      model: Intel Xeon 2.20GHz
      number of cores: 1
      threads per core: 2
      vendor id: Intel Corporation
      virtualization: null
    gpu: null
    ram: 13605 MB
  operating system:
    distribution: Ubuntu 22.04.5 LTS
    kernel: Linux 6.6.113+ x86_64
  software:
    tirex-tracker: 0.2.20
resources:
  cpu:
    used process:
      avg: 0
      max: 100
      min: 0
      timeseries:
        timestamps:
        - 1ms
        - 101ms
        values:
        - 0
        - 100
    used system:
      avg: 0
      max: 100
      min: 0
      timeseries:
        timestamps:
        - 1ms
        - 101ms
        values:
        - 100
        - 58
  ram:
    used process:
      avg: 0
      max: 156983
      min: 0
      timeseries:
        timestamps:
        - 1ms
        - 101ms
        values:
        - 156983
        - 156983
    used system:
      avg: 0
      max: 4290
      min: 0
      timeseries:
        timestamps:
        - 1ms
        - 101ms
        values:
        - 4290
        - 4285
  runtime:
    start time: 2026-03-05T16:09:14.397568991+0000
    stop time: 2026-03-05T16:09:14.568676993+0000
    system: 0 ms
    user: 170 ms
    wallclock: 171 ms
```

## Custom metadata

You can attach arbitrary key-value pairs to the export using `register_metadata` (Python) or the equivalent in your language:

=== "Python"

    ```python
    from tirex_tracker import register_metadata, tracking, ExportFormat

    register_metadata({"dataset": "msmarco-v2-passage", "model": "bm25", "retrieval_depth": 1000})

    with tracking(export_format=ExportFormat.IR_METADATA,
                  export_file_path="ir_metadata.yml") as results:
        run_retrieval()
    ```

Custom metadata appears under a `metadata` key in the output file.

## Including files

Use `register_file` to include additional files (e.g., a config file or results file) in the export:

=== "Python"

    ```python
    from tirex_tracker import register_file, tracking, ExportFormat

    register_file("config.yaml")
    register_file("run.results")

    with tracking(export_format=ExportFormat.IR_METADATA,
                  export_file_path="ir_metadata.yml") as results:
        run_retrieval()
    ```

The registered files will be bundled alongside the `ir_metadata.yml` in the output.

## C Extension API

The IR export functionality is implemented as an optional extension to the core C library. It is controlled by the `TIREX_TRACKER_EXTENSION_IR` CMake option (default: `ON` when building the shared library for distribution).

The extension exposes a single function:

```c
tirexError tirexResultExportIrMetadata(
    const tirexResult* info,    // result from tirexFetchInfo()
    const tirexResult* result,  // result from tirexStopTracking()
    const char*        filepath // output file path
);
```

Both `info` and `result` may be `NULL` — passing `NULL` for `info` omits system hardware information, passing `NULL` for `result` omits runtime resource measurements.
