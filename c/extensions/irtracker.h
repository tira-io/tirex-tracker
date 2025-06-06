#ifndef TIREX_TRACKEREXT_IRTRACKER_H
#define TIREX_TRACKEREXT_IRTRACKER_H

#include <tirex_tracker.h>

#ifdef __cplusplus
extern "C" {
#endif

TIREX_TRACKER_EXPORT tirexError
tirexResultExportIrMetadata(const tirexResult* info, const tirexResult* result, const char* filepath);

#ifdef __cplusplus
}
#endif
#endif