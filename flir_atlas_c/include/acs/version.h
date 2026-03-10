/** @file
 * @brief Atlas C SDK Version API
 * @author Teledyne FLIR
 * @copyright Copyright 2023: Teledyne FLIR
 */

#ifndef ACS_VERSION_H
#define ACS_VERSION_H

#include "common.h"

#define ACS_VERSION_MAJOR 2
#define ACS_VERSION_MINOR 12
#define ACS_VERSION_PATCH 0
#define ACS_VERSION_STRING "2.12.0"
#define ACS_COMMIT_HASH "57849ba0ad6b85cdb763f535f08829225eb82845"


#ifdef __cplusplus
extern "C" {
#endif

/** @brief Run-time variant of ACS_VERSION_MAJOR. */
ACS_API int ACS_Version_getMajor();
/** @brief Run-time variant of ACS_VERSION_MINOR. */
ACS_API int ACS_Version_getMinor();
/** @brief Run-time variant of ACS_VERSION_PATCH. */
ACS_API int ACS_Version_getPatch();

/** @brief Run-time variant of ACS_VERSION_STRING. */
ACS_API const char* ACS_Version_getString();

/** @brief Run-time variant of ACS_COMMIT_HASH. */
ACS_API const char* ACS_Version_getCommitHash();

#ifdef __cplusplus
}
#endif


#endif // ACS_VERSION_H
