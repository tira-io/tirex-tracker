<!-- TODO: Add badges. -->

<img width="100%" src="assets/banner.jpeg">
<h1 align="center">TIREx tracker</h1>
<center>
<p align="center">
    <h3 align="center">Automatic resource and metadata tracking for IR experiments.</h3>
</p>
<p align="center">
    <a><img alt="GPL 2.0 License" src="https://img.shields.io/github/license/tira-io/measure.svg" style="filter: none;"/></a>
    <a><img alt="Current Release" src="https://img.shields.io/github/release/tira-io/measure.svg" style="filter: none;"/></a>
    <br>
    <a href="#installation">Installation</a> &nbsp;|&nbsp;
    <a href="#command">Command</a> &nbsp;|&nbsp;
    <a href="#api">API</a> &nbsp;|&nbsp;
    <a href="https://github.com/tira-io/tirex-tracker/tree/main/examples">Examples</a> &nbsp;|&nbsp;
    <a href="#citation">Citation</a>
</p>
</center>

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
target_link_libraries(<yourtarget> tirex_tracker::measureapi)
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

<!-- TODO: Installation instructions with Gradle. -->
<!-- TODO: Installation instructions with Maven. -->

<!-- TODO: Usage instructions with Kotlin. -->
<!-- TODO: Usage instructions with Java. -->

```kotlin
package io.tira.tirex.tracker.example;
import io.tira.tirex.tracker.*;
fun main() {
  val result = track {
    // Do something...
  }
  println(result)
}
```

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

<!-- TODO: Explain parameters. -->

<!-- TODO: ir_metadata export instructions. -->

## Tracked Measures

<!-- TODO: Shortly list the tracked measures or groups from the paper. -->

## Contributing

<!-- TODO: Developer installation instructions and tests usage for C, Python, and JVM. -->

```shell
pytest
```

## Citation

<!-- TODO: Add citation (written out and as BibTeX). -->

## License

<!-- TODO: Add license note. -->

## Abstract

<!-- TODO: Add abstract of the paper. -->