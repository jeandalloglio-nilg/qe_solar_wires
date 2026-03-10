/** @file
 * @brief ACS Measurement Rectangle API
 * @author Teledyne FLIR 
 * @copyright Copyright 2023: Teledyne FLIR 
 */

#ifndef ACS_MEASUREMENT_RECTANGLE_H
#define ACS_MEASUREMENT_RECTANGLE_H

#include <acs/common.h>
#include <acs/measurement_area.h>
#include <acs/measurement_marker.h>
#include <acs/measurement_shape.h>


#ifdef __cplusplus
extern "C"
{
#endif

    /** @struct ACS_MeasurementRectangle
        @brief Represents a rectangle shaped measurement area.
        @see   Single rectangle:
        @see   ACS_Measurements_addRectangle
        @see   ACS_Measurements_findRectangle
        @see   ACS_Measurements_moveRectangle
        @see   ACS_Measurements_removeRectangle
        @see   Multiple rectangles:
        @see   ACS_Measurements_getAllRectangles
        @see   ACS_ListMeasurementRectangle
    */
    typedef struct ACS_MeasurementRectangle_ ACS_MeasurementRectangle;
    
    /** @brief Set the position (2D coordinate) of the rectangle.
     *
     *  @param rectangle    The line object.
     *  @param x            The x-coordinate.
     *  @param y            The y-coordinate.
     *  @param width        The width of the rectangle.
     *  @param height       The height of the rectangle.
     *
     *  @remarks        Coordinates and size arguments must be positive values and express a rectangle which does not exceed the boundaries of the image.
     *  @relatesalso    ACS_MeasurementRectangle
     */
    ACS_API void ACS_MeasurementRectangle_setRectangle(ACS_MeasurementRectangle* rectangle, int x, int y, int width, int height);

    /** @brief Get the position of the rectangle.
     *
     *  @return         A Point with (x,y)-coordinates. Error if point is (-1,-1).
     *  @relatesalso    ACS_MeasurementRectangle
     */
    ACS_API ACS_Point ACS_MeasurementRectangle_getPosition(const ACS_MeasurementRectangle* rectangle);

    /** @brief Get the area aspect of the measurement.
     *  @relatesalso    ACS_MeasurementRectangle
     */
    ACS_API const ACS_MeasurementArea* ACS_MeasurementRectangle_asMeasurementArea(const ACS_MeasurementRectangle* rectangle);

    /** @brief Get the marker aspect of the measurement.
     *  @relatesalso    ACS_MeasurementRectangle
     */
    ACS_API const ACS_MeasurementMarker* ACS_MeasurementRectangle_asMeasurementMarker(const ACS_MeasurementRectangle* rectangle);

    /** @brief Get the shape aspect of the measurement.
     *  @relatesalso    ACS_MeasurementRectangle
     */
    ACS_API const ACS_MeasurementShape* ACS_MeasurementRectangle_asMeasurementShape(const ACS_MeasurementRectangle* rectangle);



#ifdef __cplusplus
}
#endif

#endif // ACS_MEASUREMENT_RECTANGLE_H
