# The TIREx-Tracker C API

```c
#include <measure.h>

int main(int argc, char* argv[]) {
     TODO
}
```

# Installation
## CMake (FetchContent)
```cmake
include(FetchContent)

# Use GIT_TAG to request the tag (or branch) you would like
FetchContent_Declare(measure GIT_REPOSITORY https://github.com/tira-io/tirex-tracker.git GIT_TAG v0.0.1)
FetchContent_MakeAvailable(measure)
target_link_libraries(<yourtarget> measure::measureapi)
```