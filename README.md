<img width="100%" src="assets/banner.jpeg">
<h1 align="center">measure</h1>
<center>
<p align="center">
    <h3 align="center">Measuring what really matters</h3>
</p>
<p align="center">
    <a><img alt="GPL 2.0 License" src="https://img.shields.io/github/license/tira-io/measure.svg" style="filter: none;"/></a>
    <a><img alt="Current Release" src="https://img.shields.io/github/release/tira-io/measure.svg" style="filter: none;"/></a>
    <br>
    <a href="#installation">Installation</a> &nbsp;|&nbsp;
    <a href="#command">Command</a> &nbsp;|&nbsp;
    <a href="#api">API</a> &nbsp;|&nbsp;
    <a href="https://github.com/tira-io/measure/tree/main/examples">Examples</a> &nbsp;|&nbsp;
    <a href="#citation">Citation</a>
</p>
</center>



# Installation
## Measure Command
Check out our [latest release](https://github.com/tira-io/measure/releases/latest) to find a plethora of prebuilt binaries for various architectures (AMD64, ARM64) and operating systems (Windows, Linux, MacOS). Simply downloading what fits you should work since everything is compiled into a single file. 

## Measure API
### CMake (FetchContent)
```cmake
include(FetchContent)

# Use GIT_TAG to request the tag (or branch) you would like
FetchContent_Declare(measure GIT_REPOSITORY https://github.com/tira-io/measure.git GIT_TAG v0.0.1)
FetchContent_MakeAvailable(measure)
target_link_libraries(<yourtarget> measure::measureapi)
```



# Usage
Generally, `measure` offers two ways of measuring programs: a [commandline interface](#command) and a [C API](#api) that can be called from and integrated into other languages easily. Please also have a look at the [examples](./examples) to see how it can be used.

## Command
Type `measurecmd --help` to get a full description of the supported commandline arguments. Generally, you only need to call
```
measurecmd "<command>"
```
to measure everything, you may want to know about the shell command `<command>`.

## API
- [C Bindings](c/)
- [Python Bindings](python/)
- [JVM Bindings (e.g., Java and Kotlin)](jvm/)
```c
#include <measureapi.h>

int main(int argc, char* argv[]) {
    const char* provider[] = {"git", "system", "gpu", "energy", NULL};
    mapiConfig config = {.provider = provider, .monitor = true, .pollIntervallMs = 100};

    mapiMeasure* measure = mapiStartMeasure(config);
    /** Measure your code here **/

    mapiResult* result = mapiStopMeasure(measure);
    /** The collected data can be traversed here **/
    
    mapiResultFree(result);
}
```



# Datasources

Timeseries datatypes are denoted `T...`, where `T` is the datatype of each entry. Similarly, arrays are denoted as `[T]`.

## Git

| Name                                   | Type      | Description                                                                                          |   Status    |
|----------------------------------------|-----------|------------------------------------------------------------------------------------------------------|-------------|
| `git` &gt; `isrepo`                    | Boolean   | Checks if the current working directory is (part of) a Git repository                                | Implemented |
| `git` &gt; `tag`                       | String    | Retrieves the tag or branch name that is checked out                                                 | Implemented |
| `git` &gt; `last commit`               | String    | Holds the hash identifying the latest commit                                                         | Implemented |
| `git` &gt; `remote` &gt; `origin`      | String    | Contains the URL to the `origin` remote if such a remote exists                                      | Implemented |
| `git` &gt; `up to date`                | Boolean   | True if no changes to files in the repository are uncommitted                                        | Implemented |
| `git` &gt; `untracked files`           | Boolean   | True if files exist that are not ignored by the `.gitignore` and are not checked into the repository | Implemented |
| `git` &gt; `hash`                      | String    | The hash of all files checked into the repository                                                    | Planned     |

## System

| Name                                   | Type      | Description                                                                                          |   Status    |
|----------------------------------------|-----------|------------------------------------------------------------------------------------------------------|-------------|
| `elapsed time` &gt; `wallclock (ms)`   | Float     | The wallclock ("real") time the program took to run                                                  | Implemented |
| `elapsed time` &gt; `user (ms)`        | Float     | The total time the program spent in user mode                                                        | Implemented |
| `elapsed time` &gt; `system (ms)`      | Float     | The total time the program spent in system mode                                                      | Implemented |
| `resources` &gt; `RAM Used (KB)`       | UInt...   | The time series of RAM utilization                                                                   | Implemented |
| `resources` &gt; `Max RAM Used (KB)`   | UInt      | The most amount of RAM the program used at any one point                                             | Implemented |
| `resources` &gt; `CPU Utilization (%)` | UInt8...  | The time series of **process'** CPU utilization of the                                               | Planned     |
| `system` &gt; `num cores`              | UInt      | The total number of CPU cores installed in the system                                                | Implemented |
| `system` &gt; `RAM (MB)`               | UInt      | The total amount of RAM installed in the system                                                      | Implemented |
| `system` &gt; `RAM Used (MB)`          | UInt...   | The time series of entire **system's** RAM utilization                                               | Planned     |
| `system` &gt; `CPU Utilization (%)`    | UInt8...  | The time series of entire **system's** CPU utilization                                               | Planned     |

## Energy

| Name                                   | Type      | Description                                                                                          |   Status    |
|----------------------------------------|-----------|------------------------------------------------------------------------------------------------------|-------------|
| `energy` &gt; `CPU`                    | Float...  |                                                                                                      | Planned     |
| `energy` &gt; `GPU`                    | Float...  |                                                                                                      | Planned     |
| `energy` &gt; `DRAM`                   | Float...  |                                                                                                      | Planned     |

## GPU

| Name                                       | Type      | Description                                                                                          |   Status    |
|--------------------------------------------|-----------|------------------------------------------------------------------------------------------------------|-------------|
| `resources` &gt; `VRAM Used (MB)`          | [UInt]... |                                                                                                      | Planned     |
| `resources` &gt; `Max VRAM Used (MB)`      | [UInt]    |                                                                                                      | Planned     |
| `resources` &gt; `GPU Utilization (%)`     | [UInt8]...|                                                                                                      | Planned     |
| `resources` &gt; `Max GPU Utilization (%)` | [UInt8]   |                                                                                                      | Planned     |
| `system` &gt; `VRAM (MB)`                  | [UInt]    |                                                                                                      | Implemented |
| `system` &gt; `Max VRAM Used (MB)`         | [UInt]    |                                                                                                      | Implemented |
| `system` &gt; `VRAM Used (MB)`             | [UInt]... |                                                                                                      | Planned     |

# Citation
`TODO`