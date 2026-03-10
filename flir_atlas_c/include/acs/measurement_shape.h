/** @file
 * @brief ACS Measurement Shape API
 * @author Teledyne FLIR 
 * @copyright Copyright 2023: Teledyne FLIR 
 */

#ifndef ACS_MEASUREMENT_SHAPE_H
#define ACS_MEASUREMENT_SHAPE_H

#include <acs/common.h>


#ifdef __cplusplus
extern "C"
{
#endif

    /** @struct ACS_MeasurementShape
        @brief Represents the shape aspect of measurements.*/
    typedef struct ACS_MeasurementShape_ ACS_MeasurementShape;

    /** @brief Get the id number of the shape, should be between 1-8.
     *  @relatesalso ACS_MeasurementShape
     */
    ACS_API int ACS_MeasurementShape_getId(const ACS_MeasurementShape* shape);

    /** @brief Get the label for this shape.
     *  @relatesalso ACS_MeasurementShape
     */
    ACS_API ACS_String* ACS_MeasurementShape_getLabel(const ACS_MeasurementShape* shape);
    
    /** @brief Set new label on this shape.
     *
     *  @param shape    The shape that will get new label.
     *  @param label    The new label.
     *
     *  @remarks        Will fail if the label length is larger than max allowed length.
     *  @relatesalso    ACS_MeasurementShape
     */
    ACS_API void ACS_MeasurementShape_setLabel(ACS_MeasurementShape* shape, const char* label);



#ifdef __cplusplus
}
#endif

#endif // ACS_MEASUREMENT_SHAPE_H
