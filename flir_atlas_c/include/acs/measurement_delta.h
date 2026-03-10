/** @file
 * @brief ACS Measurement Delta API
 * @author Teledyne FLIR 
 * @copyright Copyright 2023: Teledyne FLIR 
 */

#ifndef ACS_MEASUREMENT_DELTA_H
#define ACS_MEASUREMENT_DELTA_H

#include <acs/common.h>
#include <acs/measurement_shape.h>
#include <acs/thermal_delta.h>


#ifdef __cplusplus
extern "C"
{
#endif
    
    /** @struct ACS_MeasurementDelta
        @brief Represents the difference (delta) between two measurements in an image. 
        @see   Single delta:
        @see   ACS_Measurements_addDelta
        @see   ACS_Measurements_findDelta
        @see   ACS_Measurements_removeDelta
        @see   Multiple deltas:
        @see   ACS_Measurements_getAllDeltas
        @see   ACS_ListMeasurementDelta
    */
    typedef struct ACS_MeasurementDelta_ ACS_MeasurementDelta;
    
    /** @brief Defines delta member value type. */
    enum ACS_DeltaMemberValueType
    {
        /** @brief Min marker value for the specified member type.
         *  Valid for member types: rectangle, circle, line.
         */
        ACS_DeltaMemberValueType_min,
        /** @brief Max marker value for the specified member type.
         *  Valid for member types: rectangle, circle, line.
         */
        ACS_DeltaMemberValueType_max,
        /** @brief Average value for the specified member type.
         *  Valid for member types: rectangle, circle, line.
         */
        ACS_DeltaMemberValueType_avg,
        /** @brief Arbitrary value.
         *  Valid for member types: spot, reference temperature.
         */
        ACS_DeltaMemberValueType_value
    };


    /** @brief  Gets the value of this Delta. */
    ACS_API ACS_ThermalDelta ACS_MeasurementDelta_getDeltaValue(const ACS_MeasurementDelta* delta);

    /** @brief Gets the first delta member shape.
     *  @relatesalso    ACS_MeasurementDelta
     */
    ACS_API const ACS_MeasurementShape* ACS_MeasurementDelta_getMember1(const ACS_MeasurementDelta*);

    /** @brief Gets the second delta member shape.
     *  @relatesalso    ACS_MeasurementDelta
     */
    ACS_API const ACS_MeasurementShape* ACS_MeasurementDelta_getMember2(const ACS_MeasurementDelta*);

    /** @brief Gets the first delta member value type.
     *
     *  @returns        @ref ACS_DeltaMemberValueType.
     *  @relatesalso    ACS_MeasurementDelta
     */
    ACS_API int ACS_MeasurementDelta_getMember1ValueType(const ACS_MeasurementDelta* delta);
    /** @brief Gets the second delta member value type.
     *
     *  @returns        @ref ACS_DeltaMemberValueType
     *  @relatesalso    ACS_MeasurementDelta
     */
    ACS_API int ACS_MeasurementDelta_getMember2ValueType(const ACS_MeasurementDelta* delta);

    /** @brief Sets the first delta member and it's type. 
     *
     *  @param delta                The delta object.
     *  @param member1              shape used as a first delta member
     *  @param member1ValueType     @ref ACS_DeltaMemberValueType. Type of the value used for the given first delta member shape (eg. min, max)
     *
     *  @relatesalso    ACS_MeasurementDelta
     */
    ACS_API void ACS_MeasurementDelta_setMember1(ACS_MeasurementDelta* delta, const ACS_MeasurementShape* member1, int member1ValueType);

    /** @brief Sets the second delta member and it's type.
     *
     *  @param delta                The delta object.
     *  @param member2              shape used as a second delta member
     *  @param member2ValueType     @ref ACS_DeltaMemberValueType. Type of the value used for the given second delta member shape (eg. min, max)
     *
     *  @relatesalso    ACS_MeasurementDelta
     */
    ACS_API void ACS_MeasurementDelta_setMember2(ACS_MeasurementDelta* delta, const ACS_MeasurementShape* member2, int member2ValueType);

    /** @brief Get the shape aspect of the measurement.
     *  @relatesalso    ACS_MeasurementDelta
     */
    ACS_API const ACS_MeasurementShape* ACS_MeasurementDelta_asMeasurementShape(const ACS_MeasurementDelta* delta);



#ifdef __cplusplus
}
#endif

#endif // ACS_MEASUREMENT_DELTA_H
