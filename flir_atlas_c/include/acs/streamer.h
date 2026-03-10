/** @file
 * @brief ACS Streamer API
 * @author Teledyne FLIR
 * @copyright Copyright 2023: Teledyne FLIR
 */

#ifndef ACS_STREAMER_H
#define ACS_STREAMER_H

#include "stream.h"
#include "renderer.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /** @struct ACS_Streamer
     * @brief Interface for transmitting a stream of data (which may be visualized). 
     * @implements ACS_Renderer
     */
    typedef struct ACS_Streamer_ ACS_Streamer;

    /** @struct ACS_VisualStreamer
     * @brief Streamer subtype for streaming visual spectrum data.
     * @implements ACS_Streamer
     */
    typedef struct ACS_VisualStreamer_ ACS_VisualStreamer;

    /** @struct ACS_ThermalStreamer
     * @brief Streamer subtype for streaming thermal data.
     * @implements ACS_Streamer
     * @implements ACS_Colorizer
     */
    typedef struct ACS_ThermalStreamer_ ACS_ThermalStreamer;


    /** @brief Free a streamer object.
     *
     * This will safely free the derived parts of the object too (similar to C++ virtual destructors).
     * @relatesalso ACS_Streamer
     */
    ACS_API void ACS_Streamer_free(ACS_Streamer* streamer);

    /** @brief Get ACS_Renderer subobject.
     * @relatesalso ACS_Streamer
     */
    ACS_API ACS_Renderer* ACS_Streamer_asRenderer(ACS_Streamer* streamer);


    /** @brief Constructs a visual streamer object.
     * @relatesalso ACS_VisualStreamer
     */
    ACS_API ACS_OWN(ACS_VisualStreamer*, ACS_VisualStreamer_free) ACS_VisualStreamer_alloc(ACS_StreamSource* streamSource);

    /** @brief Frees a visual streamer object.
     * @relatesalso ACS_VisualStreamer
     */
    ACS_API void ACS_VisualStreamer_free(ACS_VisualStreamer* streamer);

    /** @brief Get ACS_Streamer subobject.
     * @relatesalso ACS_VisualStreamer
     */
    ACS_API ACS_Streamer* ACS_VisualStreamer_asStreamer(ACS_VisualStreamer* streamer);


    /** @brief Constructs a thermal streamer object.
     * @relatesalso ACS_ThermalStreamer
     */
    ACS_API ACS_OWN(ACS_ThermalStreamer*, ACS_ThermalStreamer_free) ACS_ThermalStreamer_alloc(ACS_StreamSource* streamSource);

    /** @brief Constructs a thermal streamer object using cpu filters (thus avoiding opengl)
     * @relatesalso ACS_ThermalStreamer
     */
    ACS_API ACS_OWN(ACS_ThermalStreamer*, ACS_ThermalStreamer_free) ACS_ThermalStreamer_alloc_cpu(ACS_StreamSource* streamSource);

    /** @brief Frees a thermal streamer object.
     * @relatesalso ACS_ThermalStreamer
     */
    ACS_API void ACS_ThermalStreamer_free(ACS_ThermalStreamer* streamer);

    /** @brief Get ACS_Streamer subobject.
     * @relatesalso ACS_ThermalStreamer
     */
    ACS_API ACS_Streamer* ACS_ThermalStreamer_asStreamer(ACS_ThermalStreamer* streamer);

    /** @brief Get ACS_Colorizer subobject.
     * @relatesalso ACS_ThermalStreamer
     */
    ACS_API ACS_Colorizer* ACS_ThermalStreamer_asColorizer(ACS_ThermalStreamer* streamer);

    /** @brief Access contained ACS_ThermalImage.
     *
     * Access is thread safe since an exclusive lock for the thermal image is held while accessing it.
     * @param func Callback to gain access to the thermal image while it is locked, executed synchronously.
     * @param context User-provided data.
     * @relatesalso ACS_ThermalStreamer
     */
    ACS_API void ACS_ThermalStreamer_withThermalImage(ACS_ThermalStreamer* streamer, void (*func)(ACS_ThermalImage*, void*), void* context);

#ifdef __cplusplus
}
#endif

#endif // ACS_STREAMER_H
