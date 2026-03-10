/** @file
 * @brief ACS Thermal Value API
 * @author Teledyne FLIR
 * @copyright Copyright 2023: Teledyne FLIR
 */

#ifndef ACS_THERMAL_VALUE_H
#define ACS_THERMAL_VALUE_H

#include "common.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /** @brief Specifies the supported temperature units. */
    enum ACS_TemperatureUnit
    {
        ACS_TemperatureUnit_celsius,    ///< The Celsius temperature scale was previously known as the centigrade scale.
        ACS_TemperatureUnit_fahrenheit, ///< The Fahrenheit scale.
        ACS_TemperatureUnit_kelvin,     ///< The Kelvin scale is a thermodynamic (absolute) temperature scale where absolute zero, the theoretical absence of all thermal energy, is zero (0 K).
    };

    /** @brief Specifies the IR value State. */
    enum ACS_ThermalValueState
    {
        /** @brief Value is invalid or could not be calculated. */
        ACS_ThermalValueState_invalid,

        /** @brief Value is OK. */
        ACS_ThermalValueState_ok,

        /** @brief Value is too high. */
        ACS_ThermalValueState_overflow,

        /** @brief Value is too low. */
        ACS_ThermalValueState_underflow,

        /** @brief Value is outside image. */
        ACS_ThermalValueState_outside,

        /** @brief Value is unreliable. */
        ACS_ThermalValueState_warning,

        /** @brief Value is not yet calculated, unstable image after restart/case change. */
        ACS_ThermalValueState_unstable,

        /** @brief Value is OK + compensated with a reference temperature delta. */
        ACS_ThermalValueState_delta
    };

    /** @struct ACS_ThermalValue
    *   @brief This class represents an IR value with additional information.
    */
    typedef struct ACS_ThermalValue_
    {
        double value; ///< temperature in current unit
        int unit;  ///< see ACS_TemperatureUnit
        int state;  ///< see ACS_ThermalValueState
    } ACS_ThermalValue;

    /** @struct ACS_TemperatureRange_
     *  @brief Represents a range of thermal values.
     */ 
    struct ACS_TemperatureRange_
    {
        ACS_ThermalValue min; ///< Min thermal value
        ACS_ThermalValue max; ///< Max thermal value
    };
    typedef struct ACS_TemperatureRange_ ACS_TemperatureRange;

    /** @brief Create a formatted string from a ACS_ThermalValue.
     * @param value The value to convert to a formatted string.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API ACS_OWN(ACS_String*, ACS_String_free) ACS_ThermalValue_format(ACS_ThermalValue value);

    /** @brief Returns the equivalent Celsius value as a copy,
     * @param value The @ref ACS_ThermalValue value to convert
     */
    ACS_API ACS_ThermalValue ACS_ThermalValue_asCelsius(ACS_ThermalValue value);

    /** @brief Returns the equivalent Kelvin value as a copy,
     * @param value The @ref ACS_ThermalValue value to convert
     */
    ACS_API ACS_ThermalValue ACS_ThermalValue_asKelvin(ACS_ThermalValue value);

    /** @brief Returns the equivalent Fahrenheit value as a copy,
     * @param value The @ref ACS_ThermalValue value to convert
     */
    ACS_API ACS_ThermalValue ACS_ThermalValue_asFahrenheit(ACS_ThermalValue value);

    /** @brief Create a ThermalValue with unit ACS_TemperatureUnit_celsius.
     * @param value The value to convert
     */
    ACS_API ACS_ThermalValue ACS_ThermalValue_fromCelsius(double value);

    /** @brief Create a ThermalValue with unit ACS_TemperatureUnit_kelvin.
     * @param value The value to convert
     */
    ACS_API ACS_ThermalValue ACS_ThermalValue_fromKelvin(double value);

    /** @brief Create a ThermalValue with unit ACS_TemperatureUnit_fahrenheit.
     * @param value The value to convert
     */
    ACS_API ACS_ThermalValue ACS_ThermalValue_fromFahrenheit(double value);

#ifdef __cplusplus
}
#endif


#endif // ACS_THERMAL_VALUE_H
