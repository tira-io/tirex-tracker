<img width="100%" src="assets/banner.jpeg" alt="TIREx tracker banner image">
<h1 align="center">TIREx Tracker</h1>
<p align="center">Automatic resource and metadata tracking for information retrieval experiments.</p>
<div align="center">

[![CI](https://img.shields.io/github/actions/workflow/status/tira-io/tirex-tracker/ci.yml?branch=master&style=flat-square)](https://github.com/tira-io/tirex-tracker/actions/workflows/ci.yml)
[![Maintenance](https://img.shields.io/maintenance/yes/2025?style=flat-square)](https://github.com/tira-io/tirex-tracker/graphs/contributors)
[![Code coverage](https://img.shields.io/codecov/c/github/tira-io/tirex-tracker?style=flat-square)](https://codecov.io/github/tira-io/tirex-tracker/)
\
[![Release](https://img.shields.io/github/v/tag/tira-io/tirex-tracker?style=flat-square&label=library)](https://github.com/tira-io/tirex-tracker/releases/)
[![Ubuntu](https://img.shields.io/badge/ubuntu-20.04_%7C_22.04_%7C_24.04-blue?style=flat-square)](https://github.com/tira-io/tirex-tracker/releases/)
[![macOS](https://img.shields.io/badge/macos-13_%7C_14-blue?style=flat-square)](https://github.com/tira-io/tirex-tracker/releases/)
[![Windows](https://img.shields.io/badge/windows-2019_%7C_2022_%7C_2025-blue?style=flat-square)](https://github.com/tira-io/tirex-tracker/releases/)
\
[![PyPi](https://img.shields.io/pypi/v/tirex-tracker?style=flat-square)](https://pypi.org/project/tirex-tracker/)
[![Python](https://img.shields.io/pypi/pyversions/tirex-tracker?style=flat-square)](https://pypi.org/project/tirex-tracker/)
[![Downloads](https://img.shields.io/pypi/dm/tirex-tracker?style=flat-square)](https://pypi.org/project/tirex-tracker/)
\
[![Maven](https://img.shields.io/maven-central/v/io.tira/tirex-tracker?style=flat-square)](https://central.sonatype.com/artifact/io.tira/tirex-tracker)
[![Java](https://img.shields.io/badge/java-8_%7C_11_%7C_17_%7C_21-blue?style=flat-square)](https://github.com/tira-io/tirex-tracker/packages/)
\
[![Issues](https://img.shields.io/github/issues/tira-io/tirex-tracker?style=flat-square)](https://github.com/tira-io/tirex-tracker/issues)
[![Commit activity](https://img.shields.io/github/commit-activity/m/tira-io/tirex-tracker?style=flat-square)](https://github.com/tira-io/tirex-tracker/commits)
[![License](https://img.shields.io/github/license/tira-io/tirex-tracker?style=flat-square)](LICENSE)

[CLI](#command-line-tool)&emsp;•&emsp;[C/C++ API](#cc-api)&emsp;•&emsp;[Python API](#python-api)&emsp;•&emsp;[Java/Kotlin API](#javakotlinjvm-api)&emsp;•&emsp;[Citation](#citation)

</div>

---

The TIREx tracker is a command line tool and API to automatically track resource usage, hardware specifications, and [other metadata](#tracked-measures) when running information retrieval experiments.
It can be used either from the [command line](#command-line-tool), from [C/C++ code](#cc-api), in [Python applications](#python-api), or in [Java/Kotlin applications](#javakotlinjvm-api).

## Command Line Tool

Download a prebuilt binary for your hardware architecture and operating system from the [latest release](https://github.com/tira-io/tirex-tracker/releases/latest).

After downloading, run `tirex-tracker --help` to get a full description of all supported command line arguments.

In most cases, you would use the TIREx tracker CLI like this:

```shell
tirex-tracker "<command>"
```

This will measure all hardware metrics and metadata while executing the given shell command `<command>`.

## C/C++ API

To use the TIREx tracker in your C/C++ projects, first include this repository to your [CMake](https://cmake.org/) file like this:

```cmake
include(FetchContent)
# Use GIT_TAG to request the tag (or branch) you would like
FetchContent_Declare(tirex_tracker GIT_REPOSITORY https://github.com/tira-io/tirex-tracker.git GIT_TAG 0.0.11)
FetchContent_MakeAvailable(tirex_tracker)
target_link_libraries(<yourtarget> tirex_tracker::tirex_tracker)
```

This will link the TIREx tracker C API to your binaries.
Take a look at the [examples](c/examples/) to see how our C API can be used.

A minimal example would is shown below:

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

The TIREx tracker will automatically track the specified metrics and metadata for everything that is run between `tirexStartTracking` and `tirexStopTracking`.

You can customize the measures to track by adjusting the `conf` array in the example above.

<!-- TODO: ir_metadata export instructions. -->

## Python API

First, install the TIREx tracker Python package from [PyPI](https://pypi.org/project/tirex-tracker/):

```shell
pip install tirex-tracker
```

Now, you can track the hardware metrics and metadata of your Python code by using the [context manager](https://geeksforgeeks.org/context-manager-in-python/):

```python
from tirex_tracker import tracking

with tracking() as results:
    # Do something...

print(results)
```

Alternatively, you can track the hardware metrics and metadata of a Python function by using the [function decorator](https://geeksforgeeks.org/decorators-in-python/):

```python
from tirex_tracker import tracked

@tracked
def do_something():
    # Do something...

do_something()

print(do_something.results)
```

If you cannot use either the context manager or the function decorator from above, you can manually start and stop the tracking:

```python
from tirex_tracker import start_tracking, stop_tracking

handle = start_tracking()
try:
    # Do something...
finally:
    results = stop_tracking(handle)

print(results)
```

<!-- TODO: Explain parameters. -->

<!-- TODO: ir_metadata export instructions. -->

## Java/Kotlin/JVM API

The Java/Kotlin API can be installed via Gradle or Maven from the [Maven Central Repository](https://central.sonatype.com/artifact/io.tira/tirex-tracker).
After installing the package, you can use the TIREx tracker JVM API in your [Java](#java-usage) or [Kotlin](#kotlin-usage) projects.

<details><summary>Alternative: GitHub Packages</summary>

Alternatively to the Maven Central Repository, the TIREx tracker JVM API is also published to [GitHub Packages](https://github.com/tira-io/tirex-tracker/packages/).
To use GitHub Packages, you must first authenticate ([Maven instructions](https://docs.github.com/en/packages/working-with-a-github-packages-registry/working-with-the-apache-maven-registry#authenticating-to-github-packages), [Gradle instructions](https://docs.github.com/en/packages/working-with-a-github-packages-registry/working-with-the-gradle-registry#authenticating-to-github-packages)).

</details>

### Gradle Dependency

For Gradle, add the following line to the `dependencies` block of your `build.gradle` (or `build.gradle.kts`) file:

```gradle
dependencies {
    implementation("io.tira:tirex-tracker:x.x.x")
}
```

### Maven Dependency

For Maven projects, add these lines to your `pom.xml` file:

```xml
<dependency>
  <groupId>io.tira</groupId>
  <artifactId>tirex-tracker</artifactId>
  <version>x.x.x</version>
</dependency>
```

Replace the version placeholder (`x.x.x`) with the [latest available version tag](https://central.sonatype.com/artifact/io.tira/tirex-tracker).

You can now use the TIREx tracker JVM API in your [Java](#java-usage) or [Kotlin](#kotlin-usage) projects.

### Java Usage

For pure Java projects, the easiest way is to use the `track` function and pass your code to be tracked as a lambda like this:

```java
package io.tira.tirex.tracker.example;

import io.tira.tirex.tracker.*;

void main() {
    var result = Tracker.track(() -> {
        // Do something...
    });

    System.out.println(result);
}
```

Alternatively, use the [try-with-resources syntax](https://baeldung.com/java-try-with-resources) like this:

```java
Tracked tracked = new Tracked();
try (tracked) {
    // Do something...
}
    
System.out.println(tracked.result);
```

<!-- TODO: Explain parameters. -->

### Kotlin Usage

In Kotlin projects, the `track` function takes an [inline block](https://kotlinlang.org/docs/inline-functions.html) so that your code can be tracked like this:

```kotlin
package io.tira.tirex.tracker.example

import io.tira.tirex.tracker.*

fun main() {
  val result = track {
    // Do something...
  }

  println(result)
}
```

<!-- TODO: Explain parameters. -->

<!-- TODO: ir_metadata export instructions. -->

## Tracked Measures

<!-- TODO: Shortly list the tracked measures or groups from the paper. -->

### `ir_metadata` Extension

The resources, hardware specifications, and metadata tracked by the TIREx tracker can easily be exported to an [`ir_metadata`](https://ir-metadata.org/) file.
Not all of our extensive metadata fits well into the current `ir_metadata` specification [version 0.1](https://ir-metadata.org/metadata/overview/). We therefore extend the specification and propose `ir_metadata` 0.2-beta: 

<!-- TODO: List all added fields compared to the `ir_metadata` 0.1 specification. -->

## Contributing

We happily accept [pull requests](https://github.com/tira-io/tirex-tracker/compare), [feature requests](https://github.com/tira-io/tirex-tracker/issues/new/choose), and [bug reports](https://github.com/tira-io/tirex-tracker/issues/new/choose) to the TIREx tracker!
To get started for contributing to the development, first clone this repository:

```shell
git clone https://github.com/tira-io/tirex-tracker.git
cd tirex-tracker
```

The further steps will depend on which part of the TIREx tracker's API you work on: the [C API](#c-development), the [Python API](#python-development), or the [JVM API](#jvm-development).

### C development

> [!TIP]
> If you are not familiar with CMake, we highly recommend using the [CMake VSCode extension](https://github.com/microsoft/vscode-cmake-tools/blob/main/docs/how-to.md#configure-a-project).

We use the meta-build tool [CMake](https://cmake.org/) to configure the project. Different build targets are used so that you can select what to build.
[Doxygen](https://doxygen.nl/) is used to generate the documentation for the C API.
If you haven't installed CMake and/or Doxygen, please install it as described [here for CMake](https://cmake.org/download/) and [here for Doxygen](https://doxygen.nl/download.html)

First, configure CMake enable desired build targets by running the following. Here, we use [GCC](https://gcc.gnu.org/) but any **C++20 compliant** compiler also works.

```shell
cmake -S c/ -B c/build/ \
  -D CMAKE_BUILD_TYPE=Release \
  -D CMAKE_C_COMPILER=gcc-13 \
  -D CMAKE_CXX_COMPILER=g++-13 \
  -D BUILD_SHARED_LIBS=YES \
  -D TIREX_TRACKER_BUILD_DOCS=YES \
  -D TIREX_TRACKER_BUILD_DEB=YES \
  -D TIREX_TRACKER_BUILD_EXAMPLES=YES
```

(Hint: If you do not want to generate the documentation and have not installed Doxygen, you can disable it by setting `TIREX_TRACKER_BUILD_DOCS=NO`.)

To build the library, run
```shell
cmake --build c/build/ --config Release --target tirex_tracker_full
```
Under Linux, this will compile the C API into a statically linked library at `c/build/extensions/libtirex_tracker_full.so`.

The supported targets are<br/>
Target                   | Type          | Description
-------------------------|---------------|-----------------------------------------------------------------------------------------------
**tirex_tracker_full**   | Library       | A shared library containing `tirex_tracker` and all extensions.
**measureext_ir**        | Library       | A shared library containing only the IR extension. `tirex_tracker´ must be loaded separately.
**tirex_tracker**        | Library       | The `tirex_tracker´ shared library.
**tirex_tracker_static** | Library       | The `tirex_tracker´ static library.
**measure**              | Executable    | The measure command.
**01_tracking**          | Executable    | Example 01: demonstrating basic tracking
**02_list_measures**     | Executable    | Example 02: demonstrating how to fetch meta information through the API.
**04_ir_extension**      | Executable    | Example 04: demonstrating the IR extension (`measureext_ir`).
**tirex_tracker_docs**   | Documentation | Builds the documentation; Is only available if Doxygen is installed.
**package**              | Package       | Builds a debian package.


That means, to build the Debian package, run:

```shell
cmake --build c/build/ --config Release --target package
```

You will find the compiled Debian package file at `c/build/tirex-tracker-*-Linux.deb` (where `*` is the version).

#### CMake Options
Option                         | Description                                                                                         | Default
-------------------------------|-----------------------------------------------------------------------------------------------------|---------
`TIREX_TRACKER_ONLY_DOCS`      | Build only documentation -- this disables tests and others                                          | OFF
`TIREX_TRACKER_BUILD_EXAMPLES` | Build the examples                                                                                  | OFF
`TIREX_TRACKER_BUILD_DEB`      | Build debian package                                                                                | OFF
`TIREX_TRACKER_BUILD_DOCS`     | Build the documentation                                                                             | OFF
`TIREX_TRACKER_BUILD_CMD_DEB`  | Build the debian package for the measure command (`requires TIREX_TRACKER_BUILD_EXAMPLES` to be ON) | OFF

<!-- TODO: C test instructions. -->

### Python development

When developing (parts of) the Python API, first create a virtual environment and then install all necessary developer tooling by running:

```shell
python3 -m venv venv/
source venv/bin/activate
pip install python/[tests]
```

Once the Python developer tools are set up like that, you can check the Python code, static typing, and security, and run all tests with:

```shell
ruff check python/  # Code format and LINT
mypy python/        # Static typing
bandit -c python/pyproject.toml -r python/  # Security
pytest python/      # Unit tests
```

<!-- TODO: Build Python documentation from docstrings? -->

### JVM Development

The JVM wrapper API uses [Gradle](https://gradle.org/) for build and test.
Consistent builds are ensured by using a [Gradle wrapper](https://docs.gradle.org/current/userguide/gradle_wrapper_basics.html).
To build the classes, run:

```shell
jvm/gradlew --project-dir jvm/ build
```

Tests and checks for Java and Kotlin usage can be run by:

```shell
jvm/gradlew --project-dir jvm/ check
```

<!-- TODO: How to build the Javadoc? -->

## Citation

If you find our work useful and reference or use it in a paper, please cite us.

```bibtex
@inproceedings{tirextracker2025,
    author = {Hagen, Tim and Fr{\"o}be, Maik and Merker, Jan Heinrich and Scells, Harrisen and Hagen, Matthias and Potthast, Martin},
    booktitle = {48th International ACM SIGIR Conference on Research and Development in Information Retrieval (SIGIR 2025)},
    month = jul,
    publisher = {ACM},
    title = {{TIREx Tracker: The Information Retrieval Experiment Tracker}},
    year = {2025}
}
```

You can also use the [`CITATION.cff`](./CITATION.cff) to generate a citation in other formats.

## License

The TIREx tracker code is licensed under the [MIT License](./LICENSE).
If you use the TIREx tracker in your experiments, we would appreciate you [citing](#citation) our paper.

## Abstract

> The reproducibility and transparency of retrieval experiments heavily depends on properly provided information on the experimental setup and conditions. But as manually curating such experiment metadata can be tedious, error-prone, and inconsistent, metadata should be systematically collected in an automatic way—similar to the collection of Python and git-specific settings in the `ir_metadata` reference implementation. To enable a platform-independent automatic metadata collection following the `ir_metadata` specification, we introduce the TIREx tracker: a tool realized via a lightweight C binary, pre-compiled with all dependencies for all major platforms to track hardware configurations, usage of power/CPUs/RAM/GPUs, and experiment/system versions. The TIREx tracker seamlessly integrates into Python, Java, or C/C++ workflows and can be easily incorporated in run submissions of shared tasks, which we showcase for the TIRA/TIREx platform. Code, binaries, and documentation are publicly available at <https://github.com/tira-io/tirex-tracker>.
