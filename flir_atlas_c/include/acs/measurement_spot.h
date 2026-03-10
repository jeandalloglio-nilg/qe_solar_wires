/** @file
 * @brief ACS Measurement Spot API
 * @author Teledyne FLIR 
 * @copyright Copyright 2023: Teledyne FLIR 
 */

#ifndef ACS_MEASUREMENT_SPOT_H
#define ACS_MEASUREMENT_SPOT_H

#include <acs/common.h>
#include <acs/measurement_shape.h>
#include <acs/thermal_value.h>


#ifdef __cplusplus
extern "C"
{
#endif
    
    /** @struct ACS_MeasurementSpot
        @brief Represents a single point (spot) measurement.
        @see   Single spot:
        @see   ACS_Measurements_addSpot
        @see   ACS_Measurements_findSpot
        @see   ACS_Measurements_moveSpot
        @see   ACS_Measurements_removeSpot
        @see   Multiple spots:
        @see   ACS_Measurements_getAllSpots
        @see   ACS_ListMeasurementSpot
    */
    typedef struct ACS_MeasurementSpot_ ACS_MeasurementSpot;
    
    /** @brief Set the position (2D coordinate) of the spot
     *
     *  @param spot The spot that will get new position.
     *  @param x    The x-coordinate.
     *  @param y    The y-coordinate.
     *
     *  @remarks        Coordinates must be positive and within the bounds of the image.
     *  @relatesalso    ACS_MeasurementSpot
     */
    ACS_API void ACS_MeasurementSpot_setSpot(ACS_MeasurementSpot* spot, int x, int y);

    /** @brief Get the thermal value (i.e. temperature) of the spot.
     *
     *  @relatesalso    ACS_MeasurementSpot
     */
    ACS_API ACS_ThermalValue ACS_MeasurementSpot_getValue(const ACS_MeasurementSpot* spot);

    /** @brief Get the position of the spot.
     *
     *  @return         A Point with (x,y)-coordinates. Error if point is (-1,-1).
     *  @relatesalso    ACS_MeasurementSpot
     */
    ACS_API ACS_Point ACS_MeasurementSpot_getPosition(const ACS_MeasurementSpot* spot);

    /** @brief Get the shape aspect of the measurement.
     *  @relatesalso    ACS_MeasurementSpot
     */
    ACS_API const ACS_MeasurementShape* ACS_MeasurementSpot_asMeasurementShape(const ACS_MeasurementSpot* spot);



#ifdef __cplusplus
}
#endif

#endif // ACS_MEASUREMENT_SPOT_H
