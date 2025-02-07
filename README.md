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
Generally, `measure` offers two ways of measuring programs: a [commandline interface](#command) and a [C-API](#api) that can be called from and integrated into other languages easily. Please also have a look at the [examples](./examples) to see how it can be used.

## Command
Type `measurecmd --help` to get a full description of the supported commandline arguments. Generally, you only need to call
```
measurecmd "<command>"
```
to measure everything, you may want to know about the shell command `<command>`.

## API
```c
#include <measure.h>

int main(int argc, char* argv[]) {
    TODO
}
```



# Datasources

Timeseries datatypes are denoted `T...`, where `T` is the datatype of each entry. Similarly, arrays are denoted as `[T]`.

`TODO`

# Citation
`TODO`