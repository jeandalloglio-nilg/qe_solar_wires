/** @file
 * @brief ACS Palette API
 * @author Teledyne FLIR
 * @copyright Copyright 2023: Teledyne FLIR
 */

#ifndef ACS_PALETTE_H
#define ACS_PALETTE_H

#include <acs/common.h>
#include <acs/enum.h>
#include <acs/thermal_image.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** @struct ACS_Palette
     * @brief Represents a local palette.
     */
    typedef struct ACS_Palette_ ACS_Palette;

    enum ACS_PalettePreset_
    {
        ACS_PalettePreset_arctic,
        ACS_PalettePreset_blackhot,
        ACS_PalettePreset_bw,
        ACS_PalettePreset_coldest,
        ACS_PalettePreset_colorWheelRedhot,
        ACS_PalettePreset_colorWheel12,
        ACS_PalettePreset_colorWheel6,
        ACS_PalettePreset_doubleRainbow2,
        ACS_PalettePreset_hottest,
        ACS_PalettePreset_iron,
        ACS_PalettePreset_lava,
        ACS_PalettePreset_rainbow,
        ACS_PalettePreset_rainHC,
        ACS_PalettePreset_whitehot
    };

    enum ACS_PaletteMethod_
    {
        ACS_PaletteMethod_plainColor,
        ACS_PaletteMethod_jpgFixedLut
    };

    /** @brief Allocates a palette object.
     */
    ACS_API ACS_OWN(ACS_Palette*, ACS_Palette_free) ACS_Palette_alloc(void);

    /** @brief Destroys a palette object.
     */
    ACS_API void ACS_Palette_free(const ACS_Palette* palette);

    /** @brief Opens a Palette from the specified file.
     *
     * @param fileName Path to file.
     * @relatesalso ACS_Palette
     */
    ACS_API ACS_Palette* ACS_Palette_openFromFile(const ACS_NativePathChar* fileName);

    /** @brief Returns the name of the palette.
     */
    ACS_API const char* ACS_Palette_getName(const ACS_Palette* palette);

    /** @brief Returns the isInverted state of the palette.
     */
    ACS_API bool ACS_Palette_getIsInverted(const ACS_Palette* palette);

    /** @brief Returns the isInverted state of the palette.
     */
    ACS_API void ACS_Palette_setIsInverted(ACS_Palette* palette, bool isInverted);

    /** @brief Gets the image's current palette.
     */
    ACS_API ACS_OWN(ACS_Palette*, ACS_Palette_free) ACS_ThermalImage_getPalette(ACS_ThermalImage* image);

    /** @brief Get an instance of a preset palette.
     * @relatesalso ACS_PalettePreset
     */
    ACS_API ACS_OWN(const ACS_Palette*, ACS_Palette_free) ACS_Palette_getPalettePreset(int palette);

    /** @brief Sets the image palette using a preset palette.
     * @relatesalso ACS_PalettePreset
     */
    ACS_API void ACS_ThermalImage_setPalettePreset(ACS_ThermalImage* image, int palette);

    /** @brief Sets the image palette.
     */
    ACS_API void ACS_ThermalImage_setPalette(ACS_ThermalImage* image, ACS_Palette* palette);

#ifdef __cplusplus
}
#endif


#endif // ACS_PALETTE_H
