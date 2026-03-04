# IR Metadata Extension

TIREx Tracker can export results in the [`ir_metadata`](https://ir-metadata.org/) format — a YAML-based standard for documenting information retrieval experiments.

The current `ir_metadata` specification (version 0.1) does not cover all the hardware and software metadata that TIREx Tracker collects. TIREx Tracker therefore proposes an extended format (**`ir_metadata` 0.2-beta**) that adds fields for resource usage, energy consumption, and software environment.

## Exporting results

=== "CLI"

    ```shell
    tirex-tracker --output ir_metadata.yml "python train.py"
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
tag: 0.2-beta

system:
  os: "Fedora Linux 41 (Workstation Edition)"
  kernel: "Linux 6.12.8-200.fc41.x86_64 x86_64"
  cpu:
    model: "Intel(R) Core(TM) i7-10750H CPU @ 2.60GHz"
    cores: 12
    architecture: "x86_64"
  ram:
    total_mb: 32888
  gpu:
    model: "NVIDIA GeForce RTX 2060"
    vram_mb: 6144

run:
  start: "2025-04-17T08:00:50.996+0000"
  stop:  "2025-04-17T08:00:55.666+0000"
  wall_clock_ms: 4670
  user_time_ms: 3200
  system_time_ms: 89

resources:
  cpu_percent_max: 117
  ram_kb_max: 21630
  gpu_percent_max: 95
  gpu_vram_mb_max: 1557

energy:
  cpu_joules: 2970137
  dram_joules: 297013
  gpu_joules: 1234567

software:
  tracker_version: "0.0.11"
  invocation: "python train.py --epochs 10"

git:
  is_repo: true
  hash: "aa5fba7feff8605c3b253b46fc86d7ac1732a586"
  last_commit: "ff52eaf7c0291edbba93c87917e555c720267740"
  branch: "main"
  remote_origin: "git@github.com:org/experiment.git"
  uncommitted_changes: false
  unpushed_changes: false
```

## Custom metadata

You can attach arbitrary key-value pairs to the export using `register_metadata` (Python) or the equivalent in your language:

=== "Python"

    ```python
    from tirex_tracker import register_metadata, tracking, ExportFormat

    register_metadata("dataset", "msmarco-v2-passage")
    register_metadata("model", "bm25")
    register_metadata("retrieval_depth", 1000)

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
