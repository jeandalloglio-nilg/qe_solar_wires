/** @file
 * @brief ACS Measurement Marker API
 * @author Teledyne FLIR 
 * @copyright Copyright 2023: Teledyne FLIR 
 */

#ifndef ACS_MEASUREMENT_MARKER_H
#define ACS_MEASUREMENT_MARKER_H

#include <acs/common.h>
#include <acs/thermal_value.h>


#ifdef __cplusplus
extern "C"
{
#endif
    
    /** @struct ACS_MeasurementMarker
        @brief Represents a certain mathematical aspect, with operations such as min, max and average, of applicable measurement types. */
    typedef struct ACS_MeasurementMarker_ ACS_MeasurementMarker;

    /** @brief Get the maximum temperature value.
     *
     *  @return         A thermal value object containing the maximum temperature value.
     *  @relatesalso    ACS_MeasurementMarker
     */
    ACS_API ACS_ThermalValue ACS_MeasurementMarker_getMaxValue(const ACS_MeasurementMarker* marker);

    /** @brief Get the minimum temperature value.
     *
     *  @return         A thermal value object containing the minimum temperature value.
     *  @relatesalso    ACS_MeasurementMarker
     */
    ACS_API ACS_ThermalValue ACS_MeasurementMarker_getMinValue(const ACS_MeasurementMarker* marker);

    /** @brief Get the average temperature value.
     *
     *  @return         A thermal value object containing the maximum temperature value.
     *  @relatesalso    ACS_MeasurementMarker
     */
    ACS_API ACS_ThermalValue ACS_MeasurementMarker_getAvgValue(const ACS_MeasurementMarker* marker);

    /** @brief Get the marker flag for the maximum temperature point/location within the area.
     *
     *  @return         If true then the markers is visible/enabled, false if not.
     *  @relatesalso    ACS_MeasurementMarker
     */
    ACS_API bool ACS_MeasurementMarker_getMaxMarker(const ACS_MeasurementMarker* marker);

    /** @brief Enable/disable the marker flag for the minimum temperature point/location within the area.
     *  @relatesalso    ACS_MeasurementMarker
     */
    ACS_API void ACS_MeasurementMarker_setMinMarker(ACS_MeasurementMarker* marker, bool value);

    /** @brief Get the marker flag for the minimum temperature point/location within the area.
     *
     *  @return         If true then the markers is visible/enabled, false if not.
     *  @relatesalso    ACS_MeasurementMarker
     */
    ACS_API bool ACS_MeasurementMarker_getMinMarker(const ACS_MeasurementMarker* marker);

    /** @brief Enable/disable the marker flag for the maximum temperature point/location within the area.
     *  @relatesalso    ACS_MeasurementMarker
     */
    ACS_API void ACS_MeasurementMarker_setMaxMarker(ACS_MeasurementMarker* marker, bool value);

    /** @brief Get the position of the maximum temperature within the area.
     *
     *  @return         A Point with the (x,y)-coordinates. Error if point is (-1,-1).
     *  @relatesalso    ACS_MeasurementMarker
     */
    ACS_API ACS_Point ACS_MeasurementMarker_getMaxMarkerPosition(const ACS_MeasurementMarker* marker);

    /** @brief Get the position of the minimum temperature within the area.
     *
     *  @return         A Point with the (x,y)-coordinates. Error if point is (-1,-1).
     *  @relatesalso    ACS_MeasurementMarker
     */
    ACS_API ACS_Point ACS_MeasurementMarker_getMinMarkerPosition(const ACS_MeasurementMarker* marker);

    /** @brief Get the status if average calculations are enabled.
     *
     *  @return         Status of average calculation.
     *  @relatesalso    ACS_MeasurementMarker
     */
    ACS_API bool ACS_MeasurementMarker_getAvgCalc(const ACS_MeasurementMarker* marker);

    /** @brief Enable average calculations.*/
    ACS_API void ACS_MeasurementMarker_setAvgCalc(ACS_MeasurementMarker* marker, bool enable);

    /** @brief Get the status if min calculations are enabled.
     *
     *  @return         Status of min calculation.
     *  @relatesalso    ACS_MeasurementMarker
     */
    ACS_API bool ACS_MeasurementMarker_getMinCalc(const ACS_MeasurementMarker* marker);

    /** @brief Enable min calculations.
     *  @relatesalso    ACS_MeasurementMarker
     */
    ACS_API void ACS_MeasurementMarker_setMinCalc(ACS_MeasurementMarker* marker, bool enable);

    /** @brief Get the status if max calculations are enabled.
     *
     *  @return         Status of max calculation.
     *  @relatesalso    ACS_MeasurementMarker
     */
    ACS_API bool ACS_MeasurementMarker_getMaxCalc(const ACS_MeasurementMarker* marker);

    /** @brief Enable max calculations.
     *  @relatesalso    ACS_MeasurementMarker
     */
    ACS_API void ACS_MeasurementMarker_setMaxCalc(ACS_MeasurementMarker* marker, bool enable);



#ifdef __cplusplus
}
#endif

#endif // ACS_MEASUREMENT_MARKER_H
