/** @file
 * @brief ACS Thermal Delta API
 * @author Teledyne FLIR
 * @copyright Copyright 2023: Teledyne FLIR
 */

#ifndef ACS_THERMAL_DELTA_H
#define ACS_THERMAL_DELTA_H

#include "common.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /** @brief This class represents an IR value with additional information. */
    typedef struct ACS_ThermalDelta_
    {
        /** @brief Thermal difference/delta value. */
        double value;

        /** @brief @ref ACS_TemperatureUnit */
        int unit;

        /** @brief @ref ACS_ThermalValueState */
        int state;

    } ACS_ThermalDelta;

    /** @brief Returns the equivalent Celsius value as a copy,
     * @param delta The @ref ACS_ThermalDelta delta to convert
     */
    ACS_API ACS_ThermalDelta ACS_ThermalDelta_asCelsius(ACS_ThermalDelta delta);

    /** @brief Returns the equivalent Kelvin value as a copy,
     * @param delta The @ref ACS_ThermalDelta delta to convert
     */
    ACS_API ACS_ThermalDelta ACS_ThermalDelta_asKelvin(ACS_ThermalDelta delta);

    /** @brief Returns the equivalent Fahrenheit value as a copy,
     * @param delta The @ref ACS_ThermalDelta value to convert
     */
    ACS_API ACS_ThermalDelta ACS_ThermalDelta_asFahrenheit(ACS_ThermalDelta delta);

    
#ifdef __cplusplus
}
#endif


#endif // ACS_THERMAL_DELTA_H