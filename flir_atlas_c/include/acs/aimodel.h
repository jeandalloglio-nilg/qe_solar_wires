/** @file
 * @brief ACS AI Model API
 * @author Teledyne FLIR
 * @copyright Copyright 2023: Teledyne FLIR
 */

#ifndef ACS_AIMODEL_H
#define ACS_AIMODEL_H

#include "common.h"

#ifndef __cplusplus
# include <stdbool.h>
#endif


#ifdef __cplusplus
extern "C"
{
#endif

    /** @struct ACS_AIModel
     *  @brief AI model
     *  @warning    EXPERIMENTAL! The API isn't officially supported and will likely change in the near future without notice.
     */
    typedef struct ACS_AIModel_ ACS_AIModel;

    /** @brief Construct AI model
     *  @relates    ACS_AIModel
     */
    ACS_API ACS_OWN(ACS_AIModel*, ACS_AIModel_free) ACS_AIModel_alloc(void);

    /** @brief Destruct AI model
     *  @relates    ACS_AIModel
     */
    ACS_API void ACS_AIModel_free(const ACS_AIModel* model);

    /** @brief Load AI model
     *  @relates    ACS_AIModel
     */
    ACS_API void ACS_AIModel_loadModel(ACS_AIModel* model, const char* path);

#ifdef __cplusplus
}
#endif


#endif // ACS_AIMODEL_H
