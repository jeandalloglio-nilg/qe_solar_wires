/** @file
 * @brief ACS Renderer and Colorizer API
 * @author Teledyne FLIR
 * @copyright Copyright 2023: Teledyne FLIR
 */

#ifndef ACS_RENDERER_H
#define ACS_RENDERER_H

#include "buffer.h"
#include "enum.h"
#include "thermal_image.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /** @struct ACS_Renderer
     * @interface ACS_Renderer
     * @brief Interface for rendering images.
     */
     /** @copybrief ACS_Renderer */
    typedef struct ACS_Renderer_ ACS_Renderer;

    /** @struct ACS_Colorizer
     * @interface ACS_Colorizer
     * @brief Interface for rendering thermal images.
     * @extends ACS_Renderer
     */
     /** @copybrief ACS_Colorizer */
    typedef struct ACS_Colorizer_ ACS_Colorizer;

    /** @struct ACS_ImageColorizer
     * @brief Renders still thermal images.
     * @implements ACS_Colorizer
     */
    typedef struct ACS_ImageColorizer_ ACS_ImageColorizer;

    /** @brief Execute renderer.
     *
     * Polls image data from the source (e.g. a @ref ACS_Stream or a @ref ACS_ThermalImage), then performs image processing (see @ref ACS_Colorizer_).
     * Result could be stored in an internal buffer (see @ref ACS_Renderer_getImage, @ref ACS_Colorizer_getScaleImage) or rendered directly to screen, depending on configuration.
     * @relatesalso ACS_Renderer
     */
    ACS_API void ACS_Renderer_update(ACS_Renderer* renderer);

    /** @brief Get render result produced by @ref ACS_Renderer_update.
     * @note Rendered image will not contain scale. For getting rendered image containing scale, see @ref ACS_Colorizer_getScaleImage.
     * @return Returns pointer to the render output, or `NULL` if @ref ACS_Renderer_update has not been called previously or no source image was available yet.
     * @relatesalso ACS_Renderer
     */
    ACS_API const ACS_ImageBuffer* ACS_Renderer_getImage(const ACS_Renderer* renderer);

    /** @brief Set ACS_Renderer output color space.
     *
     * @param acsColorSpace Color space of type @ref ACS_ColorSpaceType_
     * @relatesalso ACS_Renderer_
     */
    ACS_API void ACS_Renderer_setOutputColorSpace(ACS_Renderer* renderer, int acsColorSpace);


    /** @brief Get ACS_Renderer subobject.
     * @relatesalso ACS_Colorizer
     */
    ACS_API ACS_Renderer* ACS_Colorizer_asRenderer(ACS_Colorizer* colorizer);

    /** @brief Create a colorizer for still images.
     *
     * @param image Pointer to the image to colorize/render. Lifetime of @p image is managed by the user; it must not be freed before the new ACS_ImageColorizer object is freed.
     * @relatesalso ACS_ImageColorizer
     */
    ACS_API ACS_OWN(ACS_ImageColorizer*, ACS_ImageColorizer_free) ACS_ImageColorizer_alloc(const ACS_ThermalImage* image);

    /** @brief Destroy the colorizer.
     * @relatesalso ACS_ImageColorizer
     */
    ACS_API void ACS_ImageColorizer_free(ACS_ImageColorizer* colorizer);

    /** @brief Get ACS_Colorizer subobject.
     * @relatesalso ACS_ImageColorizer
     */
    ACS_API ACS_Colorizer* ACS_ImageColorizer_asColorizer(ACS_ImageColorizer* colorizer);

    /** @brief Gets if new rendered images will have scale automatically set based on the min and max values in the image.
     * @note auto-adjusted scale is disabled by default
     * @return true if auto-adjusted scale is enabled, otherwise false.
     * @relatesalso ACS_Colorizer
     */
    ACS_API bool ACS_Colorizer_isAutoScale(const ACS_Colorizer* colorizer);

    /** @brief When enabled, new rendered images will have scale automatically set based on the min and max values in the image.
     * @note Calling this function does not trigger any render. @ref ACS_Renderer_update "Re-render" if necessary.
     * 
     * @note it is recommended to keep the rendering setting consistent with the ACS_ThermalImage information within ACS_Scale.
     * For this purpose code similar to the following could be used:
	 * @code{.cpp}
     * // auto scale enabled
     * ACS_Colorizer* colorizer = ...
     * ACS_Colorizer_setAutoScale(colorizer, true);
     * // calling ACS_Renderer_update(), among other internal work, will re-calculate scale range that we can then apply to ACS_Scale object
     * ACS_Renderer_update(ACS_Colorizer_asRenderer(colorizer));
     * 
     * ACS_Scale* scale = ... // obtain Scale instance from ACS_ThermalImage or in live stream from ACS_Streamer.withThermalImage()
     * // set the range calculated by the colorizer
     * auto tempRange = ACS_Colorizer_getScaleRange(colorizer);
     * ACS_Scale_setScale(scale, tempRange.min, tempRange.max);
     * //
     * // auto scale disabled
     * ACS_Colorizer* colorizer = ...
     * ACS_Colorizer_setAutoScale(colorizer, false);
     * // first get scale
     * ACS_Scale* scale = ... // obtain scale from ACS_ThermalImage or in live stream from ACS_Streamer.withThermalImage()
     * // then set manual range provided from the user
     * ACS_Scale_setScale(scale, userMinScaleValue, userMaxScaleValue);
     * // finally update the colorizer so the rendered image is colorized using user-defined scale range
     * ACS_Renderer_update(ACS_Colorizer_asRenderer(colorizer));
     * @endcode
	 *
     * @param autoScale Use true to enable auto-adjusted scale, false to disable.
     * @relatesalso ACS_Colorizer
     */
    ACS_API void ACS_Colorizer_setAutoScale(ACS_Colorizer* colorizer, bool autoScale);

    /** @brief Get the range of the scale, indicated by its minimum and maximum values when scale auto-adjust is set (see @ref ACS_Colorizer_isAutoScale, @ref ACS_Colorizer_setAutoScale).
     * @note Calling this function is only valid if @ref ACS_Colorizer_isAutoScale "scale auto-adjust" was enabled at the time of the last render.
     * @return Scale thermal values if @ref ACS_Renderer_update has been called and @ref ACS_Colorizer_isAutoScale "scale auto-adjust"
     * was enabled when @ref ACS_Renderer_update "update" was called, otherwise an error.
     * @relatesalso ACS_Colorizer
     */
    ACS_API ACS_TemperatureRange ACS_Colorizer_getScaleRange(const ACS_Colorizer* colorizer);

    /** @brief Get render result produced by @ref ACS_Renderer_update "update" the last time @ref ACS_Colorizer_setRenderScale "scale rendering was enabled" when @ref ACS_Renderer_update "update" was called.
     *
     * @note if scale rendering was disabled during last call to @ref ACS_Renderer_update "update", the result from this function may be out of sync with the result of @ref ACS_Colorizer_getScaleRange.
     * @note For getting rendered image without scale, see @ref ACS_Renderer_getImage.
     * @return Returns pointer to the render output, or `NULL` if @ref ACS_Renderer_update has not been called previously with scale rendering enabled (see @ref ACS_Colorizer_setRenderScale) or no source image was available yet.
     * @relatesalso ACS_Colorizer
     */
    ACS_API const ACS_ImageBuffer* ACS_Colorizer_getScaleImage(const ACS_Colorizer* colorizer);

    /** @brief Gets whether or not scale should be rendered when rendering a new image.
     * @note Scale rendering is disabled by default
     * @return true if scale will be rendered, otherwise false.
     * @relatesalso ACS_Colorizer
     */
    ACS_API bool ACS_Colorizer_isRenderScale(const ACS_Colorizer* colorizer);

    /** @brief Sets whether or not scale should be rendered when rendering a new image.
     * @param renderScale Use true to enable, false to disable.
     * @relatesalso ACS_Colorizer
     */
    ACS_API void ACS_Colorizer_setRenderScale(ACS_Colorizer* colorizer, bool renderScale);

    /** @brief Set the region of interest (ROI) that is used to calculate the histogram in the Agc filter.
     *  @note To disable the ROI and use the entire image contents, set an empty ROI ACS_Rectangle{0,0,0,0}.
     */
    ACS_API void ACS_Colorizer_setRegionOfInterest(ACS_Colorizer* colorizer, ACS_Rectangle roi);

    /** @brief Get the region of interest (ROI) that is used to calculate the histogram in the Agc filter. */
    ACS_API ACS_Rectangle ACS_Colorizer_getRegionOfInterest(const ACS_Colorizer* colorizer);

    /** @brief Gets if the colorizer is used in streaming mode or used for opening a single image. */
    ACS_API bool ACS_Colorizer_isStreaming(const ACS_Colorizer* colorizer);

    /** @brief Sets if the colorizer is used in streaming mode or used for opening a single image. 
    * @param streaming    Set the streaming mode. If true, the implementation will prevent flickering. This should be set to false when opening single files.
    * @remarks This is set to false by default, which may cause some flickering in live streaming when using some of the ColorDistribution modes.
    */
    ACS_API void ACS_Colorizer_setIsStreaming(ACS_Colorizer* colorizer, bool streaming);

    /** @struct ACS_DebugImageWindow
     * @brief EXPERIMENTAL! Do not use in production!
     * @remarks Window able to display an @ref ACS_ImageBuffer "image" on screen, intended for development/demonstration purposes only.
     */
    typedef struct ACS_DebugImageWindow_ ACS_DebugImageWindow;

    /** @brief EXPERIMENTAL! Create a window. */
    ACS_API ACS_OWN(ACS_DebugImageWindow*, ACS_DebugImageWindow_free) ACS_DebugImageWindow_alloc(const char* title);
    ACS_API ACS_OWN(ACS_DebugImageWindow*, ACS_DebugImageWindow_free) ACS_DebugImageWindow_PosXY_alloc(const char* title, int x, int y);

    /** @brief EXPERIMENTAL! Process OS events and check keyboard input. */
    ACS_API bool ACS_DebugImageWindow_poll(ACS_DebugImageWindow* window);
    /** @brief EXPERIMENTAL! Display image on screen. */
    ACS_API void ACS_DebugImageWindow_update(ACS_DebugImageWindow* window, const ACS_ImageBuffer* image);
    /** @brief EXPERIMENTAL! Destroy window and free resources. */
    ACS_API void ACS_DebugImageWindow_free(const ACS_DebugImageWindow* window);

#ifdef __cplusplus
}
#endif


#endif // ACS_RENDERER_H
