/** @file
 * @brief ACS Measurements API
 * @author Teledyne FLIR 
 * @copyright Copyright 2023: Teledyne FLIR 
 */

#ifndef ACS_MEASUREMENTS_H
#define ACS_MEASUREMENTS_H

#include <acs/common.h>
#include <acs/thermal_value.h>

#include <acs/measurement_ellipse.h>
#include <acs/measurement_delta.h>
#include <acs/measurement_line.h>
#include <acs/measurement_rectangle.h>
#include <acs/measurement_reference.h>
#include <acs/measurement_spot.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** @struct ACS_Measurements
 *  @brief Represents a set of measurements. */
typedef struct ACS_Measurements_ ACS_Measurements;

/** @struct ACS_ListMeasurementEllipse
 *  @brief List of @ref ACS_MeasurementEllipse objects. */
typedef struct ACS_ListMeasurementEllipse_ ACS_ListMeasurementEllipse;

/** @struct ACS_ListMeasurementCircle
 *  @brief List of @ref ACS_MeasurementCircle objects. */
typedef struct ACS_ListMeasurementCircle_ ACS_ListMeasurementCircle;

/** @struct ACS_ListMeasurementDelta
 *  @brief List of @ref ACS_MeasurementDelta objects. */
typedef struct ACS_ListMeasurementDelta_ ACS_ListMeasurementDelta;

/** @struct ACS_ListMeasurementLine
 *  @brief List of @ref ACS_MeasurementLine objects. */
typedef struct ACS_ListMeasurementLine_ ACS_ListMeasurementLine;

/** @struct ACS_ListMeasurementRectangle
 *  @brief List of @ref ACS_MeasurementRectangle objects. */
typedef struct ACS_ListMeasurementRectangle_ ACS_ListMeasurementRectangle;

/** @struct ACS_ListMeasurementReference
 *  @brief List of @ref ACS_MeasurementReference objects. */
typedef struct ACS_ListMeasurementReference_ ACS_ListMeasurementReference;

/** @struct ACS_ListMeasurementSpot
 *  @brief List of @ref ACS_MeasurementSpot objects. */
typedef struct ACS_ListMeasurementSpot_ ACS_ListMeasurementSpot;


/** @struct ACS_ListRemoteMeasurementCircle
 *  @brief List of remote @ref ACS_MeasurementCircle objects. */
typedef struct ACS_ListRemoteMeasurementCircle_ ACS_ListRemoteMeasurementCircle;

/** @struct ACS_ListRemoteMeasurementLine
 *  @brief List of remote @ref ACS_MeasurementLine objects. */
typedef struct ACS_ListRemoteMeasurementLine_ ACS_ListRemoteMeasurementLine;

/** @struct ACS_ListRemoteMeasurementRectangle
 *  @brief List of remote @ref ACS_MeasurementRectangle objects. */
typedef struct ACS_ListRemoteMeasurementRectangle_ ACS_ListRemoteMeasurementRectangle;

/** @struct ACS_ListRemoteMeasurementSpot
 *  @brief List of remote @ref ACS_MeasurementSpot objects. */
typedef struct ACS_ListRemoteMeasurementSpot_ ACS_ListRemoteMeasurementSpot;


/** @brief Adds a new ACS_MeasurementEllipse to the collection.
 * The 2D coordinate is the center of the ellipse, NOT the top left corner.
 *
 *  @param measurements The collection of measurements.
 *  @param x            The x-coordinate.
 *  @param y            The y-coordinate.
 *  @param radiusX       RadiusX of the ellipse.
 *  @param radiusY       RadiusY of the ellipse.
 *  @param markerMin    Show the position marker for the minimum temperature within the ellipse.
 *  @param markerMax    Show the position marker for the maximum temperature within the ellipse.
 *  @return             The ACS_MeasurementEllipse object added to the collection.
 *
 *  @remarks        An error will be set if the limit for the number of ellipses has been reached.
 *  @remarks        Coordinates and radii must be non-negative and ellipse must not exceed the boundaries of the image.
 *  @relatesalso    ACS_Measurements
 */
ACS_API ACS_MeasurementEllipse* ACS_Measurements_addEllipse(ACS_Measurements* measurements, int x, int y, int radiusX, int radiusY, bool markerMin, bool markerMax);

/** @brief Move an ellipse.
 *  @remarks    The 2D coordinate is the center of the ellipse, NOT the top left corner.
 *
 *  @param measurements The collection of measurements.
 *  @param id           The id of the ellipse.
 *  @param newX         The new x-coordinate.
 *  @param newY         The new y-coordinate.
 *  @param radiusX       RadiusX of the ellipse.
 *  @param radiusY       RadiusY of the ellipse.
 *
 *  @return         The ACS_MeasurementEllipse object that is moved, or NULL if id is invalid.
 *  @remarks        Coordinates and radii must be non-negative and ellipse must not exceed the boundaries of the image.
 *  @relatesalso    ACS_Measurements
 */
ACS_API ACS_MeasurementEllipse* ACS_Measurements_moveEllipse(ACS_Measurements* measurements, int id, int newX, int newY, int radiusX, int radiusY);

/** @brief Search for an ellipse object.
 *
 *  @param measurements The collection of measurements.
 *  @param id           The id of the ellipse.
 *
 *  @return         The ellipse object corresponding to id, otherwise NULL.
 *  @relatesalso    ACS_Measurements
 */
ACS_API ACS_MeasurementEllipse* ACS_Measurements_findEllipse(ACS_Measurements* measurements, int id);

/** @brief Remove the ellipse.
 *
 *  @param measurements The collection of measurements.
 *  @param id           The id of the ellipse.
 *
 *  @return         True when removal was successful, otherwise false.
 *  @relatesalso    ACS_Measurements
 */
ACS_API bool ACS_Measurements_removeEllipse(ACS_Measurements* measurements, int id);


/** @brief Adds a new ACS_MeasurementDelta to the collection.
 *
 *  @param measurements         The collection of measurements.
 *  @param member1              The first delta member shape.
 *  @param member1ValueType     @ref ACS_DeltaMemberValueType. The first delta member value type.
 *  @param member2              The second delta member shape.
 *  @param member2ValueType     @ref ACS_DeltaMemberValueType. The second delta member value type.
 *  @return                     The ACS_MeasurementDelta object added to the collection.
 *
 *  @remarks        An error will be set if the limit for the number of delta has been reached.
 *  @remarks        An error will be set of the result is not a valid measurement delta.
 *  @relatesalso    ACS_Measurements
 */
ACS_API ACS_MeasurementDelta* ACS_Measurements_addDelta(ACS_Measurements* measurements, const ACS_MeasurementShape* member1, int member1ValueType, const ACS_MeasurementShape* member2, int member2ValueType);

/** @brief Search for a delta object.
 *
 *  @param measurements The collection of measurements.
 *  @param id           The id of the delta.
 *
 *  @return         The delta object corresponding to id if it exists, otherwise NULL.
 *  @relatesalso    ACS_Measurements
 */
ACS_API ACS_MeasurementDelta* ACS_Measurements_findDelta(ACS_Measurements* measurements, int id);

/** @brief Remove the delta.
 *
 *  @param measurements The collection of measurements.
 *  @param id           The id of the delta.
 *
 *  @return         True when removal was successful, otherwise false.
 *  @relatesalso    ACS_Measurements
 */
ACS_API bool ACS_Measurements_removeDelta(ACS_Measurements* measurements, int id);


/** @brief Add a new horizontal line to the collection.
 *
 *  @param measurements The collection of measurements.
 *  @param  y           The y-coordinate where the line is placed.
 *  @param  markerMin   Show the position marker for the minimum temperature on the line.
 *  @param  markerMax   Show the position marker for the maximum temperature on the line.
 *  @return             The ACS_MeasurementLine object added to the collection.
 *
 *  @remarks        An error will be set if the limit for the number of lines has been reached.
 *  @remarks        Coordinate must be non-negative and line must not exceed the boundaries of the image.
 *  @relatesalso    ACS_Measurements
 */
ACS_API ACS_MeasurementLine* ACS_Measurements_addHorizontalLine(ACS_Measurements* measurements, int y, bool markerMin, bool markerMax);

/** @brief Add a new vertical line to the collection.
 *
 *  @param measurements The collection of measurements.
 *  @param  x           The x-coordinate where the line is placed.
 *  @param  markerMin   Show the position marker for the minimum temperature on the line.
 *  @param  markerMax   Show the position marker for the maximum temperature on the line.
 *  @return             The ACS_MeasurementLine object added to the collection.
 *
 *  @remarks        An error will be set if the limit for the number of lines has been reached.
 *  @remarks        Coordinate must be non-negative and line must not exceed the boundaries of the image.
 *  @relatesalso    ACS_Measurements
 */
ACS_API ACS_MeasurementLine* ACS_Measurements_addVerticalLine(ACS_Measurements* measurements, int x, bool markerMin, bool markerMax);

/** @brief Move a line.
 *
 *  @param measurements The collection of measurements.
 *  @param id           The id of the line.
 *  @param newStartY    The new start y-coordinate.
 *  @return             The ACS_MeasurementLine object that is moved, or NULL if id is invalid.
 *
 *  @remarks        Coordinate must be non-negative and line must not exceed the boundaries of the image.
 *  @relatesalso    ACS_Measurements
 */
ACS_API ACS_MeasurementLine* ACS_Measurements_moveHorizontalLine(ACS_Measurements* measurements, int id, int newStartY);

/** @brief Move a line.
 *
 *  @param measurements The collection of measurements.
 *  @param id           The id of the line.
 *  @param newStartX    The new start x-coordinate.
 *  @return             The ACS_MeasurementLine object that is moved, or NULL if id is invalid.
 *
 *  @remarks        Coordinate must be non-negative and line must not exceed the boundaries of the image.
 *  @relatesalso    ACS_Measurements
 */
ACS_API ACS_MeasurementLine* ACS_Measurements_moveVerticalLine(ACS_Measurements* measurements, int id, int newStartX);

/** @brief Adds a new ACS_MeasurementLine to the collection.
 *  @param measurements The collection of measurements.
 *  @param x1           The start x-coordinate.
 *  @param y1           The start y-coordinate.
 *  @param x2           The end x-coordinate.
 *  @param y2           The end y-coordinate.
 *  @param markerMin    Show the position marker for the minimum temperature on the line.
 *  @param markerMax    Show the position marker for the maximum temperature on the line.
 *  @return             The ACS_MeasurementLine object added to the collection.
 *
 *  @remarks            An error will be set if the limit for the number of lines has been reached.
 *  @remarks            Coordinates must be non-negative and must not exceed the boundaries of the image.
 *  @relatesalso        ACS_Measurements
 */
ACS_API ACS_MeasurementLine* ACS_Measurements_addLine(ACS_Measurements* measurements, int x1, int y1, int x2, int y2, bool markerMin, bool markerMax);

/** @brief Move a line.
 *
 *  @param measurements The collection of measurements.
 *  @param id           The id of the line.
 *  @param newStartX    The new start x-coordinate.
 *  @param newStartY    The new start y-coordinate.
 *  @param newEndX      The new end x-coordinate.
 *  @param newEndY      The new end y-coordinate.
 *  @return             The ACS_MeasurementLine object that is moved, or NULL if id is invalid.
 *
 *  @remarks        Coordinates must be non-negative and must not exceed the boundaries of the image.
 *  @relatesalso    ACS_Measurements
 */
ACS_API ACS_MeasurementLine* ACS_Measurements_moveLine(ACS_Measurements* measurements, int id, int newStartX, int newStartY, int newEndX, int newEndY);

/** @brief Search for a line object.
 *
 *  @param measurements The collection of measurements.
 *  @param id           The id of the line.
 *
 *  @return         The line object corresponding to id, or NULL if no such object exists.
 *  @relatesalso    ACS_Measurements
 */
ACS_API ACS_MeasurementLine* ACS_Measurements_findLine(ACS_Measurements* measurements, int id);

/** @brief Remove the line.
 *
 *  @param measurements The collection of measurements.
 *  @param id           The id of the line.
 *
 *  @return         True when removal was successful, otherwise false.
 *  @relatesalso    ACS_Measurements
 */
ACS_API bool ACS_Measurements_removeLine(ACS_Measurements* measurements, int id);


/** @brief Adds a new ACS_MeasurementRectangle to the collection.
 *
 *  @param measurements The collection of measurements.
 *  @param x            The x-coordinate.
 *  @param y            The y-coordinate.
 *  @param width        Width of the rectangle.
 *  @param height       Height of the rectangle.
 *  @param markerMin    Show the position marker for the minimum temperature within the rectangle.
 *  @param markerMax    Show the position marker for the maximum temperature within the rectangle.
 *  @return             The ACS_MeasurementRectangle object added to the collection.
 *
 *  @remarks        An error will be set if the limit for the number of rectangles has been reached.
 *  @remarks        Coordinates and size arguments must be non-negative and rectangle must not exceed the boundaries of the image.     
 *  @relatesalso    ACS_Measurements
 */
ACS_API ACS_MeasurementRectangle* ACS_Measurements_addRectangle(ACS_Measurements* measurements, int x, int y, int width, int height, bool markerMin, bool markerMax);

/** @brief Move a rectangle.
 *
 *  @param measurements The collection of measurements.
 *  @param id           The id of the rectangle.
 *  @param newX         The new x-coordinate.
 *  @param newY         The new y-coordinate.
 *  @param width        Width of the rectangle.
 *  @param height       Height of the rectangle.
 *  @return             The ACS_MeasurementRectangle object that is moved, or NULL if id is invalid.
 *
 *  @remarks        Coordinates and size arguments must be non-negative and rectangle must not exceed the boundaries of the image.
 *  @relatesalso    ACS_Measurements
 */
ACS_API ACS_MeasurementRectangle* ACS_Measurements_moveRectangle(ACS_Measurements* measurements, int id, int newX, int newY, int width, int height);

/** @brief Search for a rectangle object.
 *
 *  @param measurements The collection of measurements.
 *  @param id           The id of the rectangle.
 *
 *  @return         The rectangle object corresponding to id if it exists, otherwise NULL.
 *  @relatesalso    ACS_Measurements
 */
ACS_API ACS_MeasurementRectangle* ACS_Measurements_findRectangle(ACS_Measurements* measurements, int id);

/** @brief Remove the rectangle.
 *
 *  @param measurements The collection of measurements.
 *  @param id           The id of the rectangle.
 *
 *  @return         True when removal was successful, otherwise false.
 *  @relatesalso    ACS_Measurements
 */
ACS_API bool ACS_Measurements_removeRectangle(ACS_Measurements* measurements, int id);


/** @brief Adds a new ACS_MeasurementReference to the collection.
 *
 *  @param measurements The collection of measurements.
 *  @param value        The arbitrary value for this measurement
 *  @return             The ACS_MeasurementReference object added to the collection.
 *
 *  @remarks        An error will be set if the limit for the number of references has been reached.
 *  @remarks        Value must be a valid @ref ACS_ThermalValue
 *  @relatesalso    ACS_Measurements
 */
ACS_API ACS_MeasurementReference* ACS_Measurements_addReference(ACS_Measurements* measurements, ACS_ThermalValue value);

/** @brief Set a new value for the reference measurement.
 *
 *  @param measurements The collection of measurements.
 *  @param id           The id of the reference.
 *  @param value        The new arbitrary value for this measurement
 *  @return             The ACS_MeasurementReference object that is updated, or NULL if id is invalid.
 *
 *  @remarks        Value must be a valid @ref ACS_ThermalValue
 *  @relatesalso    ACS_Measurements
 */
ACS_API ACS_MeasurementReference* ACS_Measurements_setReferenceValue(ACS_Measurements* measurements, int id, ACS_ThermalValue value);

/** @brief Search for a reference object.
 *
 *  @param measurements The collection of measurements.
 *  @param id           The id of the reference.
 *
 *  @return         The reference object corresponding to @p id, or NULL if it was not found.
 *  @relatesalso    ACS_Measurements
 */
ACS_API ACS_MeasurementReference* ACS_Measurements_findReference(ACS_Measurements* measurements, int id);

/** @brief Remove the reference.
 *
 *  @param measurements The collection of measurements.
 *  @param id           The id of the reference.
 *
 *  @return         True when removal was successful, otherwise false.
 *  @relatesalso    ACS_Measurements
 */
ACS_API bool ACS_Measurements_removeReference(ACS_Measurements* measurements, int id);


/** @brief Adds a new ACS_MeasurementSpot to the collection.
 *
 *  @param measurements The collection of measurements.
 *  @param x            The x-coordinate.
 *  @param y            The y-coordinate.
 *  @return             The ACS_MeasurementSpot object added to the collection.
 *
 *  @remarks        An error will be set if spots limit has been reached.
 *  @remarks        Coordinates must be positive and not exceed the boundaries of the image.
 *  @relatesalso    ACS_Measurements
 */
ACS_API ACS_MeasurementSpot* ACS_Measurements_addSpot(ACS_Measurements* measurements, int x, int y);

/** @brief Move a spot.
 *
 *  @param measurements The collection of measurements.
 *  @param id           The id of the spot.
 *  @param newX         The new x-coordinate.
 *  @param newY         The new y-coordinate.
 *  @return             The ACS_MeasurementSpot object that is moved, or NULL if id is invalid.
 *
 *  @remarks        Coordinates must be positive and not exceed the boundaries of the image.
 *  @relatesalso    ACS_Measurements
 */
ACS_API ACS_MeasurementSpot* ACS_Measurements_moveSpot(ACS_Measurements* measurements, int id, int newX, int newY);

/** @brief Search for a spot object.
 *
 *  @param measurements The collection of measurements.
 *  @param id           The id of the spot.
 *  @return             The spot object corresponding to id, or NULL if it does not exist.
 *
 *  @relatesalso    ACS_Measurements
 */
ACS_API ACS_MeasurementSpot* ACS_Measurements_findSpot(ACS_Measurements* measurements, int id);

/** @brief Remove the spot.
 *
 *  @param measurements The collection of measurements.
 *  @param id           The id of the spot.
 *  @return             True when removal was succesful, otherwise false.
 *
 *  @relatesalso    ACS_Measurements
 */
ACS_API bool ACS_Measurements_removeSpot(ACS_Measurements* measurements, int id);


/** @brief Get all the active ellipses in the image.
 *
 *  @return         An array of ACS_MeasurementEllipse objects, the valid size/length of the array is set in the argument 'size'.
 *  @relatesalso    ACS_Measurements
 */
ACS_API ACS_OWN(ACS_ListMeasurementEllipse*, ACS_ListMeasurementEllipse_free) ACS_Measurements_getAllEllipses(ACS_Measurements* measurements);

/** @brief Get all the active deltas in the image.
 *
 *  @return         An array of ACS_MeasurementDelta objects, the valid size/length of the array is set in the argument 'size'.
 *  @relatesalso    ACS_Measurements
 */
ACS_API ACS_OWN(ACS_ListMeasurementDelta*, ACS_ListMeasurementDelta_free) ACS_Measurements_getAllDeltas(ACS_Measurements* measurements);

/** @brief Get all the active lines in the image.
 *
 *  @return         An array of ACS_MeasurementLine objects, the valid size/length of the array is set in the argument 'size'.
 *  @relatesalso    ACS_Measurements
 */
ACS_API ACS_OWN(ACS_ListMeasurementLine*, ACS_ListMeasurementLine_free) ACS_Measurements_getAllLines(ACS_Measurements* measurements);

/** @brief Get all the active rectangles in the image.
 *
 *  @return         An array of ACS_MeasurementRectangle objects, the valid size/length of the array is set in the argument 'size'.
 *  @relatesalso    ACS_Measurements
 */
ACS_API ACS_OWN(ACS_ListMeasurementRectangle*, ACS_ListMeasurementRectangle_free) ACS_Measurements_getAllRectangles(ACS_Measurements* measurements);

/** @brief Get all the active references in the image.
 *
 *  @return         An array of ACS_MeasurementReference objects, the valid size/length of the array is set in the argument 'size'.
 *  @relatesalso    ACS_Measurements
 */
ACS_API ACS_OWN(ACS_ListMeasurementReference*, ACS_ListMeasurementReference_free) ACS_Measurements_getAllReferences(ACS_Measurements* measurements);

/** @brief Get all the active spots in the image.
 *
 *  @return         An array of ACS_MeasurementSpot objects, the valid size/length of the array is set in the argument 'size'.
 *  @relatesalso    ACS_Measurements
 */
ACS_API ACS_OWN(ACS_ListMeasurementSpot*, ACS_ListMeasurementSpot_free) ACS_Measurements_getAllSpots(ACS_Measurements* measurements);


/** @brief Releases memory reserved for list.
 *
 *  @remarks        Items in list are not owned by the user and MUST NOT be freed separately.
 *  @relatesalso    ACS_ListMeasurementEllipse
 */
ACS_API void ACS_ListMeasurementEllipse_free(const ACS_ListMeasurementEllipse* list);

/** @brief Returns the number of elements in the list.
 *  @relatesalso    ACS_ListMeasurementEllipse
 */
ACS_API size_t ACS_ListMeasurementEllipse_size(ACS_ListMeasurementEllipse* list);

/** @brief Retrieves item at specified index if it exists.
 *
 *  @param list     The list that holds @ref ACS_MeasurementEllipse objects.
 *  @param index    Index of object to retrieve.
 *
 *  @returns        Object if it exists, otherwise NULL.
 *  @relatesalso    ACS_ListMeasurementEllipse
 */
ACS_API ACS_MeasurementEllipse* ACS_ListMeasurementEllipse_item(ACS_ListMeasurementEllipse* list, size_t index);

/** @brief Releases memory allocated for list.
 *
 *  @remarks Items in list are not owned by the user and MUST NOT be freed separately.
 *  @relatesalso    ACS_ListMeasurementDelta
 */
ACS_API void ACS_ListMeasurementDelta_free(const ACS_ListMeasurementDelta* list);

/** @brief Retrieves item at specified index if it exists.
 *
 *  @param list     The list that holds @ref ACS_MeasurementDelta objects.
 *  @param index    Index of object to retrieve.
 *
 *  @returns        Object if it exists, otherwise NULL.
 *  @relatesalso    ACS_ListMeasurementDelta
 */
ACS_API ACS_MeasurementDelta* ACS_ListMeasurementDelta_item(ACS_ListMeasurementDelta* list, size_t index);

/** @brief Returns the number of elements in the list.
 *  @relatesalso    ACS_ListMeasurementDelta
 */
ACS_API size_t ACS_ListMeasurementDelta_size(ACS_ListMeasurementDelta* list);

/** @brief Releases memory reserved for list.
 *
 *  @remarks        Items in list are not owned by the user and MUST NOT be freed separately.
 *  @relatesalso    ACS_ListMeasurementLine
 */
ACS_API void ACS_ListMeasurementLine_free(const ACS_ListMeasurementLine* list);

/** @brief Returns the number of elements in the list.
 *  @relatesalso    ACS_ListMeasurementLine
 */
ACS_API size_t ACS_ListMeasurementLine_size(ACS_ListMeasurementLine* list);

/** @brief Retrieves item at specified index if it exists.
 *
 *  @param list     The list that holds @ref ACS_MeasurementLine objects.
 *  @param index    Index of object to retrieve.
 *
 *  @returns        Object if it exists, otherwise NULL.
 *  @relatesalso    ACS_ListMeasurementLine
 */
ACS_API ACS_MeasurementLine* ACS_ListMeasurementLine_item(ACS_ListMeasurementLine* list, size_t index);

/** @brief Releases memory reserved for list.
 *
 *  @remarks        Items in list are not owned by the user and MUST NOT be freed separately.
 *  @relatesalso    ACS_ListMeasurementRectangle
 */
ACS_API void ACS_ListMeasurementRectangle_free(const ACS_ListMeasurementRectangle* list);

/** @brief Returns the number of elements in the list.
 *  @relatesalso    ACS_ListMeasurementRectangle
 */
ACS_API size_t ACS_ListMeasurementRectangle_size(ACS_ListMeasurementRectangle* list);

/** @brief Retrieves item at specified index if it exists.
 *
 *  @param list     The list that holds @ref ACS_MeasurementRectangle objects.
 *  @param index    Index of object to retrieve.
 *
 *  @returns        Object if it exists, otherwise NULL.
 *  @relatesalso    ACS_ListMeasurementRectangle
 */
ACS_API ACS_MeasurementRectangle* ACS_ListMeasurementRectangle_item(ACS_ListMeasurementRectangle* list, size_t index);

/** @brief Releases memory reserved for list.
 *
 *  @remarks        Items in list are not owned by the user and MUST NOT be freed separately.
 *  @relatesalso    ACS_ListMeasurementReference
 */
ACS_API void ACS_ListMeasurementReference_free(const ACS_ListMeasurementReference* list);

/** @brief Returns the number of elements in the list.
 *  @relatesalso    ACS_ListMeasurementReference
 */
ACS_API size_t ACS_ListMeasurementReference_size(ACS_ListMeasurementReference* list);

/** @brief Retrieves item at specified index if it exists.
 *
 *  @param list     The list that holds @ref ACS_MeasurementReference objects.
 *  @param index    Index of object to retrieve.
 *
 *  @returns        Object if it exists, otherwise NULL.
 *  @relatesalso    ACS_ListMeasurementReference
 */
ACS_API ACS_MeasurementReference* ACS_ListMeasurementReference_item(ACS_ListMeasurementReference* list, size_t index);

/** @brief Releases memory reserved for list.
 *
 *  @remarks        Items in list are not owned by the user and MUST NOT be freed separately.
 *  @relatesalso    ACS_ListMeasurementSpot
 */
ACS_API void ACS_ListMeasurementSpot_free(const ACS_ListMeasurementSpot* list);

/** @brief Returns the number of elements in the list.
 *  @relatesalso    ACS_ListMeasurementSpot
 */
ACS_API size_t ACS_ListMeasurementSpot_size(ACS_ListMeasurementSpot* list);

/** @brief Retrieves item at specified index if it exists.
 *
 *  @param list     The list that holds @ref ACS_MeasurementSpot objects.
 *  @param index    Index of object to retrieve.
 *
 *  @returns        Object if it exists, otherwise NULL.
 *  @relatesalso    ACS_ListMeasurementSpot
 */
ACS_API ACS_MeasurementSpot* ACS_ListMeasurementSpot_item(ACS_ListMeasurementSpot* list, size_t index);


#ifdef __cplusplus
}
#endif

#endif // ACS_MEASUREMENTS_H
