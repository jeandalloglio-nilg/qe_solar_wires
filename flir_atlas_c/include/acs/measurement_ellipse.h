/** @file
 * @brief ACS Measurement Ellipse API
 * @author Teledyne FLIR 
 * @copyright Copyright 2025: Teledyne FLIR 
 */

#ifndef ACS_MEASUREMENT_ELLIPSE_H
#define ACS_MEASUREMENT_ELLIPSE_H

#include <acs/common.h>

#include <acs/measurement_area.h>
#include <acs/measurement_marker.h>
#include <acs/measurement_shape.h>


#ifdef __cplusplus
extern "C"
{
#endif

    /** @struct ACS_MeasurementEllipse
        @brief Represents an ellipse shaped measurement area.
        @see   Single ellipse:
        @see   ACS_Measurements_addEllipse
        @see   ACS_Measurements_findEllipse
        @see   ACS_Measurements_moveEllipse
        @see   ACS_Measurements_removeEllipse
        @see   Multiple ellipses:
        @see   ACS_Measurements_getAllEllipses
        @see   ACS_ListMeasurementEllipse
    */
    typedef struct ACS_MeasurementEllipse_ ACS_MeasurementEllipse;

    /** @brief Set the position (2D coordinate) and radii of the ellipse.
     *  The 2D coordinate is the center of the ellipse, NOT the top left corner.
     *
     *  @param ellipse   The ellipse object that will get a new position and size.
     *  @param x        The x-coordinate of the ellipse's center.
     *  @param y        The y-coordinate of the ellipse's center.
     *  @param radiusX   The horizontal radius of the ellipse. The width/2 - 1 of the ellipse.
     *  @param radiusY   The vertical radius of the ellipse. The height/2 - 1 of the ellipse.
     *
     *  @remarks        Coordinates and radius must be positive values and express a shape which does not exceed the boundaries of the image.
     *  @relatesalso    ACS_MeasurementEllipse
     */
    ACS_API void ACS_MeasurementEllipse_setEllipse(ACS_MeasurementEllipse* ellipse, int x, int y, int radiusX, int radiusY);

    /** @brief Get the position of the ellipse's center.
     *
     *  @param ellipse   The ellipse object that holds the position.
     *
     *  @return         A Point with the (x,y)-coordinates. Error if point is (-1,-1).
     *  @relatesalso    ACS_MeasurementEllipse
     */
    ACS_API ACS_Point ACS_MeasurementEllipse_getPosition(const ACS_MeasurementEllipse* ellipse);

    /** @brief Get the horizontal radius of the ellipse (x-axis).
     *
     *  @param ellipse   The ellipse object that holds the radius.
     *  @return         Length of X raidus.
     *
     *  @relatesalso    ACS_MeasurementEllipse
     */
    ACS_API int ACS_MeasurementEllipse_getRadiusX(const ACS_MeasurementEllipse* ellipse);

    /** @brief Get the vertical radius of the ellipse (y-axis).
     *
     *  @param ellipse   The ellipse object that holds the radius.
     *  @return         Length of Y raidus.
     *
     *  @relatesalso    ACS_MeasurementEllipse
     */
    ACS_API int ACS_MeasurementEllipse_getRadiusY(const ACS_MeasurementEllipse* ellipse);

    /** @brief Get the area aspect of the measurement.
     *
     *  @param ellipse   The ellipse object that should be converted to @ref ACS_MeasurementArea.
     *  @relatesalso    ACS_MeasurementEllipse
     */
    ACS_API const ACS_MeasurementArea* ACS_MeasurementEllipse_asMeasurementArea(const ACS_MeasurementEllipse* ellipse);

    /** @brief Get the marker aspect of the measurement.
     *
     *  @param ellipse   The ellipse object that should be converted to @ref ACS_MeasurementMarker.
     *  @relatesalso    ACS_MeasurementEllipse
     */
    ACS_API const ACS_MeasurementMarker* ACS_MeasurementEllipse_asMeasurementMarker(const ACS_MeasurementEllipse* ellipse);

    /** @brief Get the shape aspect of the measurement.
     *
     *  @param ellipse   The ellipse object that should be converted to @ref ACS_MeasurementShape.
     *  @relatesalso    ACS_MeasurementEllipse
     */
    ACS_API const ACS_MeasurementShape* ACS_MeasurementEllipse_asMeasurementShape(const ACS_MeasurementEllipse* ellipse);



#ifdef __cplusplus
}
#endif

#endif // ACS_MEASUREMENT_ELLIPSE_H
