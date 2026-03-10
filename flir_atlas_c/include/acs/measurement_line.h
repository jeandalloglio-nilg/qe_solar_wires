/** @file
 * @brief ACS Measurement Line API
 * @author Teledyne FLIR 
 * @copyright Copyright 2023: Teledyne FLIR 
 */

#ifndef ACS_MEASUREMENT_LINE_H
#define ACS_MEASUREMENT_LINE_H

#include <acs/common.h>
#include <acs/measurement_area.h>
#include <acs/measurement_marker.h>
#include <acs/measurement_shape.h>


#ifdef __cplusplus
extern "C"
{
#endif
    
    /** @struct ACS_MeasurementLine
        @brief Represents a line shaped measurement area. 
        @see   Single line:
        @see   ACS_Measurements_addLine
        @see   ACS_Measurements_addHorizontalLine
        @see   ACS_Measurements_addVerticalLine
        @see   ACS_Measurements_findLine
        @see   ACS_Measurements_moveLine
        @see   ACS_Measurements_moveHorizontalLine
        @see   ACS_Measurements_moveVerticalLine
        @see   ACS_Measurements_removeLine
        @see   Multiple lines:
        @see   ACS_Measurements_getAllLines
        @see   ACS_ListMeasurementLine
    */
    typedef struct ACS_MeasurementLine_ ACS_MeasurementLine;
    
    /** @brief  Set the location (2D coordinate) of the line.
     *
     *  @param line     The line object.
     *  @param x1       The start x-coordinate.
     *  @param y1       The start y-coordinate.
     *  @param x2       The end x-coordinate.
     *  @param y2       The end y-coordinate.
     *
     *  @remarks        Coordinates must be positive values and not exceed the boundaries of the image.
     *  @relatesalso    ACS_MeasurementLine
     */
    ACS_API void ACS_MeasurementLine_setLine(ACS_MeasurementLine* line, int x1, int y1, int x2, int y2);

    /** @brief Set the location of a vertical line.
     *
     *  @param line     The line object.
     *  @param x        The x-coordinate where the line is placed.
     *
     *  @remarks        X-coordinate must be a positive value and not exceed the boundaries of the image.
     *  @relatesalso    ACS_MeasurementLine
     */
    ACS_API void ACS_MeasurementLine_setVerticalLine(ACS_MeasurementLine* line, int x);

    /** @brief Set the location of a horizontal line.
     *
     *  @param line     The line object.
     *  @param y        The y-coordinate where the line is placed.
     *
     *  @remarks        Y-coordinate must be a positive value and not exceed the boundaries of the image.
     *  @relatesalso    ACS_MeasurementLine
     */
    ACS_API void ACS_MeasurementLine_setHorizontalLine(ACS_MeasurementLine* line, int y);

    /** @brief Get the start position of the line.
     *
     *  @return         A Point with (x,y)-coordinates. Error if point is (-1,-1).
     *  @relatesalso    ACS_MeasurementLine
     */
    ACS_API ACS_Point ACS_MeasurementLine_getStartPosition(const ACS_MeasurementLine* line);

    /** @brief Get the end position of the line.
     *
     *  @return         A Point with (x,y)-coordinates. Error if point is (-1,-1).
     *  @relatesalso    ACS_MeasurementLine
     */
    ACS_API ACS_Point ACS_MeasurementLine_getEndPosition(const ACS_MeasurementLine* line);

    /** @brief Get the area aspect of the measurement.
     *  @relatesalso    ACS_MeasurementLine
     */
    ACS_API const ACS_MeasurementArea* ACS_MeasurementLine_asMeasurementArea(const ACS_MeasurementLine* line);

    /** @brief Get the marker aspect of the measurement.
     *  @relatesalso    ACS_MeasurementLine
     */
    ACS_API const ACS_MeasurementMarker* ACS_MeasurementLine_asMeasurementMarker(const ACS_MeasurementLine* line);

    /** @brief Get the shape aspect of the measurement.
     *  @relatesalso    ACS_MeasurementLine
     */
    ACS_API const ACS_MeasurementShape* ACS_MeasurementLine_asMeasurementShape(const ACS_MeasurementLine* line);



#ifdef __cplusplus
}
#endif

#endif // ACS_MEASUREMENT_LINE_H
