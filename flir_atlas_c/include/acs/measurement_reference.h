/** @file
 * @brief ACS Measurement Reference API
 * @author Teledyne FLIR 
 * @copyright Copyright 2023: Teledyne FLIR 
 */

#ifndef ACS_MEASUREMENT_REFERENCE_H
#define ACS_MEASUREMENT_REFERENCE_H

#include <acs/common.h>
#include <acs/measurement_shape.h>
#include <acs/thermal_value.h>


#ifdef __cplusplus
extern "C"
{
#endif

    /** @struct ACS_MeasurementReference
        @brief Represents a reference temperature to a measurement.
        @see   Single reference:
        @see   ACS_Measurements_addReference
        @see   ACS_Measurements_findReference
        @see   ACS_Measurements_removeReference
        @see   ACS_Measurements_setReferenceValue
        @see   Multiple references:
        @see   ACS_Measurements_getAllReferences
        @see   ACS_ListMeasurementReference
    */
    typedef struct ACS_MeasurementReference_ ACS_MeasurementReference;

    /** @brief Get the thermal value (i.e. temperature) of the reference measurement.
     *  @relatesalso    ACS_MeasurementReference
     */
    ACS_API ACS_ThermalValue ACS_MeasurementReference_getValue(const ACS_MeasurementReference* reference);

    /** @brief Set the thermal value (i.e. temperature) of the reference measurement.
     *  @relatesalso    ACS_MeasurementReference
     */
    ACS_API void ACS_MeasurementReference_setValue(ACS_MeasurementReference* reference, ACS_ThermalValue value);

    /** @brief Get the shape aspect of the measurement.
     *  @relatesalso    ACS_MeasurementReference
     */
    ACS_API const ACS_MeasurementShape* ACS_MeasurementReference_asMeasurementShape(const ACS_MeasurementReference* reference);



#ifdef __cplusplus
}
#endif

#endif // ACS_MEASUREMENT_REFERENCE_H
