/**
 * @file irtracker.h
 * @brief Contains the ir extension for tirex tracker
 */

#ifndef TIREX_TRACKEREXT_IRTRACKER_H
#define TIREX_TRACKEREXT_IRTRACKER_H

#include <tirex_tracker.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @defgroup irtracking Information Retrieval
 * @brief
 * @details
 * @{
 */
/**
 * @brief
 * 
 * @param info 
 * @param result 
 * @param filepath 
 * @return TIREX_TRACKER_EXPORT 
 */
TIREX_TRACKER_EXPORT tirexError
tirexResultExportIrMetadata(const tirexResult* info, const tirexResult* result, const char* filepath);

/** @} */ // end of irtracking
#ifdef __cplusplus
}
#endif
#endif