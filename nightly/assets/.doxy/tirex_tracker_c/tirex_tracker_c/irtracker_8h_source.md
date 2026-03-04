

# File irtracker.h

[**File List**](files.md) **>** [**c**](dir_1784a01aa976a8c78ef5dfc3737bcac8.md) **>** [**extensions**](dir_38a876d14b5516144c7bcccda62f37ac.md) **>** [**irtracker.h**](irtracker_8h.md)

[Go to the documentation of this file](irtracker_8h.md)


```C++

#ifndef TIREX_TRACKEREXT_IRTRACKER_H
#define TIREX_TRACKEREXT_IRTRACKER_H

#include <tirex_tracker.h>

#ifdef __cplusplus
extern "C" {
#endif
TIREX_TRACKER_EXPORT tirexError
tirexResultExportIrMetadata(const tirexResult* info, const tirexResult* result, const char* filepath);
 // end of irtracking
#ifdef __cplusplus
}
#endif
#endif
```


