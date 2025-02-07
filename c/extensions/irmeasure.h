#ifndef MEASURE_EXT_IRMEASURE_H
#define MEASURE_EXT_IRMEASURE_H

#include <measure.h>

#ifdef __cplusplus
extern "C" {
#endif

MSR_EXPORT msrError msrResultExportIrMetadata(const msrResult* result, const char* filepath);

#ifdef __cplusplus
}
#endif
#endif