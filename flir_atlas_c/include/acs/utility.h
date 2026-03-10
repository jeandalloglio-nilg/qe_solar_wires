/** @file
 * @brief Utility API
 * @author Teledyne FLIR
 * @copyright Copyright 2023: Teledyne FLIR
 */

#ifndef ACS_UTILITY_H
#define ACS_UTILITY_H

#include "common.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /** @struct ACS_Future
     * @brief EXPERIMENTAL! Thread-safe "channel" for passing and waiting for values between threads.
     */
    typedef struct ACS_Future_ ACS_Future;

    /** @brief Create a future object.
     * @relatesalso ACS_Future
     */
    ACS_API ACS_OWN(ACS_Future*, ACS_Future_free) ACS_Future_alloc(void);

    /** @brief Destroy a future object.
     * @relatesalso ACS_Future
     */
    ACS_API void ACS_Future_free(ACS_Future*);

    /** @brief Read the value, blocking until it's received (or an error is set).
     * @note May only be read once.
     * @relatesalso ACS_Future
     */
    ACS_API void* ACS_Future_get(ACS_Future*);


    /** @brief Set the value.
     * @note May only be set once, and both @ref ACS_Future_setValue and ACS_Future_setError are considered setting the future.
     * @relatesalso ACS_Future
     */
    ACS_API void ACS_Future_setValue(ACS_Future*, void*);

    /** @brief Set an error.
     * @note May only be set once, and both @ref ACS_Future_setValue and ACS_Future_setError are considered setting the future.
     * @relatesalso ACS_Future
     */
    ACS_API void ACS_Future_setError(ACS_Future*, ACS_Error);


#ifdef __cplusplus
}
#endif


#endif // ACS_UTILITY_H
