/** @file
 * @brief ACS Stream API
 * @author Teledyne FLIR
 * @copyright Copyright 2023: Teledyne FLIR
 */

#ifndef ACS_STREAM_H
#define ACS_STREAM_H

#include "common.h"
#include "thermal_sequence_recorder.h"

#ifdef __cplusplus
extern "C"
{
#endif
    /** @struct ACS_Stream
     * @brief Camera stream interface
     * @see ACS_Camera_getStream
     */
    typedef struct ACS_Stream_ ACS_Stream;

    /** @brief Pixel source object for Camera streaming
     * @see ACS_VisualStreamer
     * @see ACS_ThermalStreamer
     */
    typedef ACS_Stream ACS_StreamSource;

    /// Callback type for camera events when a new frame is available
    typedef void (*ACS_OnImageReceived)(void* context);


    /** @brief Starts the frame grabbing.
     *
     * @param onImageReceived Callback to receive a notification for each new frame that arrives.
     * @warning This callback is invoked on a separate "streaming thread", and should only be used to trigger/notify a render thread
     *          of the new image. Executing heavy processing/tasks in this callback may result in reduced performance or lost frames.
     * @param onError Callback to monitor if the stream is broken. Invoked on a background thread. The stream will be @ref stop "stopped" automatically on error.
     * @param context User-provided data.
     * @relatesalso ACS_Stream
     */
    ACS_API void ACS_Stream_start(ACS_Stream* stream, ACS_OnImageReceived onImageReceived, ACS_OnError onError, ACS_CallbackContext context);

    /** @brief Stops the frame grabbing.
     *
     * @post @ref ACS_Stream_isStreaming() == `false`
     * @relatesalso ACS_Stream
     */
    ACS_API void ACS_Stream_stop(ACS_Stream* stream);

    /** @brief Check if the stream is in frame grabbing state.
     * @relatesalso ACS_Stream
     */
    ACS_API bool ACS_Stream_isStreaming(const ACS_Stream* stream);

    /** @brief Check if a stream is thermal (contains radiometric/measurable pixels).
     *
     * The stream can only be used together with @ref ACS_ThermalImage if the stream is thermal.
     * @relatesalso ACS_Stream
     */
    ACS_API bool ACS_Stream_isThermal(const ACS_Stream* stream);

    /** @brief Get the stream's pixel source.
     * @relatesalso ACS_Stream
     */
    ACS_API ACS_StreamSource* ACS_Stream_getSource(ACS_Stream* stream);

    /** @brief Set the camera's streamed frame rate.
     *
     * @note Some cameras will not change frame rate while streaming.
     * @param hz Frames per second, must be in range [getMinFrameRate(), getMaxFrameRate()].
     * @relatesalso ACS_Stream
     */
    ACS_API void ACS_Stream_setFrameRate(ACS_Stream* stream, double hz);

    /** @brief Get the camera's streamed frame rate.
     * @relatesalso ACS_Stream
     */
    ACS_API double ACS_Stream_getFrameRate(const ACS_Stream* stream);

    /** @brief Get the lowest possible frame rate for this camera stream.
     * @relatesalso ACS_Stream
     */
    ACS_API double ACS_Stream_getMinFrameRate(const ACS_Stream* stream);

    /** @brief Get the highest possible frame rate for this camera stream.
     * @relatesalso ACS_Stream
     */
    ACS_API double ACS_Stream_getMaxFrameRate(const ACS_Stream* stream);

    /** @brief Check if Vivid IR filter is supported for this camera stream.
     * @relatesalso ACS_Stream
     */
    ACS_API bool ACS_Stream_isVividIrSupported(const ACS_Stream* stream);

    /** @brief Vivid IR upscale method. Changing this may give faster response, but lower quality.
     * @relatesalso ACS_Stream
     */
    ACS_API void ACS_Stream_setVividIrUpscale(ACS_Stream* stream, int upscale);

    /** @brief Vivid IR latency mode. Changing this may give faster response, but lower quality.
     * @relatesalso ACS_Stream
     */
    ACS_API void ACS_Stream_setVividIrLatency(ACS_Stream* stream, int latency);

    /** @brief Vivid IR denoise filter on or off. Changing this may give faster response, but lower quality.
     * @relatesalso ACS_Stream
     */
    ACS_API void ACS_Stream_setVividIrUseDenoise(ACS_Stream* stream, bool useDenoise);

    /** @brief Vivid IR parameters, in particular upscale, latency, "use denoise filter" settings. Changing this may give faster response, but lower quality.
     * @relatesalso ACS_Stream
     */
    ACS_API void ACS_Stream_setVividIrCustomParameters(ACS_Stream* stream, int upscale, int latency, bool useDenoise);

    /** @brief Vivid IR upscale method.
     * @relatesalso ACS_Stream
     */
    ACS_API int ACS_Stream_getVividIrUpscale(const ACS_Stream* stream);

    /** @brief Vivid IR latency mode.
     * @relatesalso ACS_Stream
     */
    ACS_API int ACS_Stream_getVividIrLatency(const ACS_Stream* stream);

    /** @brief Vivid IR use denoise filter setting.
     * @relatesalso ACS_Stream
     */
    ACS_API bool ACS_Stream_getVividIrUseDenoise(const ACS_Stream* stream);

    /** @brief Attach a Thermal Sequence Recorder to the stream to record incoming frames.
     * 
     * @param recorder The recorder to attach.
     * @note Not all streams support attaching a recorder. Supported streams: 
     * @note   - USB stream (Windows only)
     * @note   - emulator stream
     * @see ACS_Stream_detachRecorder
     * @see ACS_ThermalSequenceRecorder
     * @relatesalso ACS_Stream
     */
    ACS_API void ACS_Stream_attachRecorder(ACS_Stream* stream, ACS_ThermalSequenceRecorder* recorder);

    /** @brief Detach the Thermal Sequence Recorder from the stream.
     * @see ACS_Stream_attachRecorder
     * @see ACS_ThermalSequenceRecorder
     * @relatesalso ACS_Stream
     */
    ACS_API void ACS_Stream_detachRecorder(ACS_Stream* stream);

#ifdef __cplusplus
}
#endif

#endif // ACS_STREAM_H
