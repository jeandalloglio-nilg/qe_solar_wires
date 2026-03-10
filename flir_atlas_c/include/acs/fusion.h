/** @file
 * @brief ACS Fusion API
 * @author Teledyne FLIR
 * @copyright Copyright 2023: Teledyne FLIR
 */

#ifndef ACS_FUSION_H
#define ACS_FUSION_H

#include "common.h"
#include "thermal_image.h"
#include "buffer.h"

#ifdef __cplusplus
extern "C"
{
#endif
    /** @struct ACS_Fusion */
    typedef struct ACS_Fusion_ ACS_Fusion;

    /** @enum ACS_FusionMode_
     *  @brief Specifies the modes supported for Fusion
     */
    enum ACS_FusionMode_
    {
        /** Ordinary IR image. */
        ACS_FusionMode_thermalOnly,

        /** Visual image is zoomed and fused with IR image.
         * @see VisualOnlySettings
         * @see ACS_Fusion_getVisualData for not aligned DC image
         */
        ACS_FusionMode_visualOnly,

        /** IR image is blended with visual image based on the specified blending factor value.
         * Use Blending mode to display a blended image that uses a mix of infrared pixels and digital photo pixels.
         * @see BlendingSettings
         */
        ACS_FusionMode_blending,

        /** Thermal MSX (Multi Spectral Dynamic Imaging) mode displays an infrared image where the edges of the objects are enhanced with information from the visual image.
         * @see MsxSettings
         */
        ACS_FusionMode_msx,

        /** IR image is blended with visual image based on the specified @ref ACS_ThermalValue threshold.
         * @see ThermalFusionSettings
         */
        ACS_FusionMode_thermalFusion,

        /** Picture in picture fusion, where a defined box/rectangle shape with IR image pixels is placed on top of the visual image.
         * @see PipSettings
         */
        ACS_FusionMode_pictureInPicture,

        /** Color night vision fusion. */
        ACS_FusionMode_colorNightVision


    };

    /** @brief Color mode
     */
    enum ACS_ColorMode_
    {
        /// Fusion image has visual image in color.
        ACS_ColorMode_color = 0,

        /// Fusion image has visual image in b/w.
        ACS_ColorMode_blackAndWhite = 1
    };

    /** @brief MSX (Multi Spectral Dynamic Imaging) settings of an image
     *  @see ACS_FusionMode_msx
     */
    typedef struct  ACS_Fusion_MsxSettings_
    {
        /** @brief Field that indicate how much the visual image is blended with IR (valid value range: [0.0, 100.0]).
            * Thermal image may appear a bit differently for the same MSX alpha value depending on the camera's resolution.
            * Thus the value should be based on the expected results and visible effect, but typical and suggested values are:
            * - FLIR ONE gen 2: 2.0 or 3.0
            * - FLIR ONE gen 3: 8.0 or higher
            */
        double alpha;
    } ACS_Fusion_MsxSettings;

    /** @brief PIP (Picture in Picture) settings of an image.
     *
     * In PIP fusion mode the Thermal image is displayed in defined area on top of the Visual image.
     * @see ACS_FusionMode_pictureInPicture
     */
    typedef struct  ACS_Fusion_PipSettings_
    {
        /** @brief Area where the thermal image is displayed. */
        ACS_Rectangle area;

        /** @brief The color scheme the visual image is displayed in 
         *  @ref ACS_ColorMode_
         */
        int colorMode;

    } ACS_Fusion_PipSettings;

    /** @brief Fusion rendering settings when the ThermalImage is to show only photo.
     * @see ACS_FusionMode_visualOnly
     */
    typedef struct ACS_Fusion_VisualOnlySettings_
    {
        /** @brief The color scheme the visual image is displayed in 
         *  @ref ACS_ColorMode_
         */
        int colorMode;
    } ACS_Fusion_VisualOnlySettings;

    /** @brief Settings when blending photo and thermal.
     * @see ACS_FusionMode_blending
     */
    typedef struct ACS_Fusion_BlendingSettings_
    {
        /** @brief The blending level between thermal and visual image (1.0 = only IR, 0.0 = only visual). */
        double level;

        /** @brief The color scheme the visual image is displayed in 
         *  @ref ACS_ColorMode_
         */
        int colorMode;
    } ACS_Fusion_BlendingSettings;

    /** @brief Thermal fusion settings for an image.
     * @see ACS_FusionMode_thermalFusion
     */
    typedef struct ACS_Fusion_ThermalFusionSettings_
    {
        /** @brief The minimum blending level between thermal and visual image. */
        ACS_ThermalValue min;

        /** @brief The maximum blending level between thermal and visual image. */
        ACS_ThermalValue max;
    } ACS_Fusion_ThermalFusionSettings;

    /** @brief Fusion transformation settings.  */
    typedef struct ACS_Transformation_
    {
        /** @brief Fusion horizontal panning. */
        int panX;
        /** @brief Fusion vertical panning. */
        int panY;
        /** @brief Fusion zoom factor. */
        float scale;
        /** @brief Fusion rotation factor clockwise in degrees. */
        float rotation;
    } ACS_Transformation;

    /** @brief Gets the Fusion object for the image.
     * @relatesalso ACS_Fusion
     */
    ACS_API const ACS_Fusion* ACS_ThermalImage_getFusion(const ACS_ThermalImage* image);

    /** @brief Sets the mode for Fusion object
     *  @relatesalso ACS_FusionMode_
     */
    ACS_API void ACS_Fusion_setFusionMode(ACS_Fusion* fusion, int mode);

    /** @brief Gets the mode for Fusion object
     *  @relatesalso ACS_FusionMode_
     */
    ACS_API int ACS_Fusion_getCurrentFusionMode(const ACS_Fusion* fusion);

    /** @brief Update thermal image MSX settings.
     *
     *  These settings are used when ACS_FusionMode_msx is the [active fusion mode](@ref ACS_Fusion_getCurrentFusionMode).
     *  @param settings New MSX values. Valid alpha value range: [0.0, 100.0].
     *  @note Does no longer change the fusion mode, see @ref ACS_Fusion_setFusionMode.
     */
    ACS_API void ACS_Fusion_setMsx(ACS_Fusion* fusion, ACS_Fusion_MsxSettings settings);

    /** @brief Get thermal image MSX settings.
     */
    ACS_API ACS_Fusion_MsxSettings ACS_Fusion_getMsx(const ACS_Fusion* fusion);

    /** @brief Update thermal image PIP settings.
     *
     *  These settings are used when ACS_FusionMode_pip is the [active fusion mode](@ref ACS_Fusion_getCurrentFusionMode).
     *  @param settings New PIP values.
     *  @note Does no longer change the fusion mode, see @ref ACS_Fusion_setFusionMode.
     */
    ACS_API void ACS_Fusion_setPictureInPicture(ACS_Fusion* fusion, ACS_Fusion_PipSettings settings);

    /** @brief Get thermal image PIP settings.
     */
    ACS_API ACS_Fusion_PipSettings ACS_Fusion_getPictureInPicture(const ACS_Fusion* fusion);

    /** @brief Update thermal image visual only settings.
     *
     *  These settings are used when ACS_FusionMode_visualOnly is the [active fusion mode](@ref ACS_Fusion_getCurrentFusionMode).
     *  @param settings New visual only values.
     *  @note Does no longer change the fusion mode, see @ref ACS_Fusion_setFusionMode.
     */
    ACS_API void ACS_Fusion_setVisualOnly(ACS_Fusion* fusion, ACS_Fusion_VisualOnlySettings settings);

    /** @brief Get thermal image visual only settings.
     */
    ACS_API ACS_Fusion_VisualOnlySettings ACS_Fusion_getVisualOnly(const ACS_Fusion* fusion);

    /** @brief Update thermal image blending settings.
     *
     *  These settings are used when ACS_FusionMode_blending is the [active fusion mode](@ref ACS_Fusion_getCurrentFusionMode).
     *  @param settings New blending values.
     *  @note Does no longer change the fusion mode, see @ref ACS_Fusion_setFusionMode.
     */
    ACS_API void ACS_Fusion_setBlending(ACS_Fusion* fusion, ACS_Fusion_BlendingSettings settings);

    /** @brief Get thermal image blending settings.
     */
    ACS_API ACS_Fusion_BlendingSettings ACS_Fusion_getBlending(const ACS_Fusion* fusion);

    /** @brief Update thermal image blending settings.
     *
     *  These settings are used when ACS_FusionMode_thermalFusion is the [active fusion mode](@ref ACS_Fusion_getCurrentFusionMode).
     *  @param settings New blending values.
     *  @note Does no longer change the fusion mode, see @ref ACS_Fusion_setFusionMode.
     */
    ACS_API void ACS_Fusion_setThermalFusion(ACS_Fusion* fusion, ACS_Fusion_ThermalFusionSettings settings);

    /** @brief Update thermal image blending settings.
     *
     *  Same as `setThermalFusion({ min, <max temperature> });` where <max temperature> is some internal value.
     *  @param min Lowest temperature to show.
     */
    ACS_API void ACS_Fusion_setThermalFusionAbove(ACS_Fusion* fusion, ACS_ThermalValue min);

    /** @brief Update thermal image blending settings.
     *
     *  Same as `setThermalFusion({ <min temperature>, max });` where <min temperature> is some internal value.
     *  @param max Highest temperature to show.
     */
    ACS_API void ACS_Fusion_setThermalFusionBelow(ACS_Fusion* fusion, ACS_ThermalValue max);

    /** @brief Get thermal image blending settings.
     */
    ACS_API ACS_Fusion_ThermalFusionSettings ACS_Fusion_getThermalFusion(const ACS_Fusion* fusion);

    /** @brief Set fusion transformation. */
    ACS_API void ACS_Fusion_setTransformation(ACS_Fusion* fusion, ACS_Transformation transform);

    /** @brief Get fusion transformation. */
    ACS_API ACS_Transformation ACS_Fusion_getTransformation(const ACS_Fusion* fusion);

    /** @brief Get the Photo, which is not fusion aligned.
     *
     * Raw visual pixels/"photo"/"DC", usually in YCbCr format.
     * These are only available if the thermal image was created with fusion data and visual pixel data.
     * These pixels are not aligned to the IR pixels.
     */
    ACS_API void ACS_Fusion_getVisualData(const ACS_Fusion* fusion, ACS_ImageBuffer* buffer);

#ifdef __cplusplus
}
#endif


#endif // ACS_FUSION_H
