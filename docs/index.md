# TIREx Tracker

**Automatic resource and metadata tracking for information retrieval experiments.**

[![CI](https://img.shields.io/github/actions/workflow/status/tira-io/tirex-tracker/ci.yml?branch=master&style=flat-square)](https://github.com/tira-io/tirex-tracker/actions/workflows/ci.yml)
[![PyPi](https://img.shields.io/pypi/v/tirex-tracker?style=flat-square)](https://pypi.org/project/tirex-tracker/)
[![Maven](https://img.shields.io/maven-central/v/io.tira/tirex-tracker?style=flat-square)](https://central.sonatype.com/artifact/io.tira/tirex-tracker)
[![License](https://img.shields.io/github/license/tira-io/tirex-tracker?style=flat-square)](https://github.com/tira-io/tirex-tracker/blob/master/LICENSE)

---

The reproducibility and transparency of retrieval experiments depend on properly recording the experimental setup and conditions. Manually curating such metadata is tedious, error-prone, and inconsistent. TIREx Tracker automates this: wrap your experiment in a single call, and get a structured record of hardware, software, timing, energy, and Git state — without changing your code.

## What it tracks

TIREx Tracker collects over 50 measurements across six categories:

| Category | Examples |
|---|---|
| **OS** | OS name and version, kernel version |
| **Time** | Wall-clock, user-mode, and system-mode elapsed time |
| **CPU** | Model, cores, frequency, utilization (process & system), energy |
| **RAM** | Usage (process & system), available, DRAM energy |
| **GPU** | Model, VRAM, utilization (process & system), GPU energy (NVIDIA) |
| **Git** | Commit hash, branch, tags, uncommitted/unpushed changes, code archive |

See the [full list of tracked measures](guides/measures.md) for details.

## How to use it

TIREx Tracker is available as a CLI tool and as a library for C/C++, Python, and Java/Kotlin.

=== "CLI"

    ```shell
    tirex-tracker "python train.py --epochs 10"
    ```

=== "Python"

    ```python
    from tirex_tracker import tracking

    with tracking() as results:
        train_model()

    print(results)
    ```

=== "Java"

    ```java
    var result = Tracker.track(() -> trainModel());
    System.out.println(result);
    ```

=== "Kotlin"

    ```kotlin
    val result = track { trainModel() }
    println(result)
    ```

=== "C"

    ```c
    #include <tirex_tracker.h>

    tirexMeasureConf conf[] = {
        {TIREX_TIME_ELAPSED_WALL_CLOCK_MS, TIREX_AGG_NO},
        {TIREX_CPU_USED_PROCESS_PERCENT,   TIREX_AGG_MAX},
        {TIREX_RAM_USED_PROCESS_KB,        TIREX_AGG_MAX},
        tirexNullMeasureConf
    };
    tirexTrackingHandle* handle;
    tirexStartTracking(conf, /*pollIntervalMs=*/100, &handle);

    run_experiment();

    tirexResult* result;
    tirexStopTracking(handle, &result);
    tirexResultFree(result);
    ```

## Getting started

Choose the guide for your language or use case:

<div class="grid cards" markdown>

- :material-console: **[CLI Tool](getting-started/cli.md)**

    Download a prebuilt binary and wrap any shell command.

- :material-language-python: **[Python](getting-started/python.md)**

    `pip install tirex-tracker` and use the context manager or decorator.

- :material-language-java: **[Java / Kotlin](getting-started/java.md)**

    Add a single Gradle or Maven dependency.

- :material-language-c: **[C / C++](getting-started/c.md)**

    Integrate via CMake `FetchContent` and link against the C API.

</div>

## Citation

If you use TIREx Tracker in your research, please cite:

```bibtex
@inproceedings{tirextracker2025,
    author    = {Hagen, Tim and Fr{\"o}be, Maik and Merker, Jan Heinrich and
                 Scells, Harrisen and Hagen, Matthias and Potthast, Martin},
    booktitle = {48th International ACM SIGIR Conference on Research and
                 Development in Information Retrieval (SIGIR 2025)},
    month     = jul,
    publisher = {ACM},
    title     = {{TIREx Tracker: The Information Retrieval Experiment Tracker}},
    year      = {2025}
}
```
