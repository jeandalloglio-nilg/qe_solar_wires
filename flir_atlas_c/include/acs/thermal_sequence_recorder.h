/** @file
 * @brief ACS Thermal Sequence Recorder API
 * EXPERIMENTAL! Warning, these APIs are subject to change in future releases
 * @author Teledyne FLIR
 * @copyright Copyright 2023: Teledyne FLIR
 */

#ifndef ACS_THERMALSEQUENCERECORDER_H
#define ACS_THERMALSEQUENCERECORDER_H

#include <acs/common.h>
#include <acs/thermal_image.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** @brief Specifies the supported recorder states. 
     * 
     * The recorder can be in one of the following states:
     * - `Stopped` (default)
     * - `Recording`
     * - `Paused`
     * 
     * @see ACS_ThermalSequenceRecorder_getState
     * @see ACS_ThermalSequenceRecorder_start
     * @see ACS_ThermalSequenceRecorder_pause
     * @see ACS_ThermalSequenceRecorder_resume
     * @see ACS_ThermalSequenceRecorder_stop
     */
	enum ACS_RecorderState_
	{
		ACS_RecorderState_stopped,
		ACS_RecorderState_paused,
		ACS_RecorderState_recording,
	};

    /** @struct ACS_ThermalSequenceRecorder
     * @brief The Thermal Sequence Recorder is used to record FFF frames to a file.
     * 
     * # Main usage
     * 
     * There are two main approaches for adding frames to the recorder:
     * 1. Add frames explicitly by calling:
     *   - @ref ACS_ThermalSequenceRecorder_addImage, which adds a frame from a ThermalImage.
     * 2. Attach the recorder to a camera stream that supports it. The stream will automatically add frames to the recorder.
     *    - This is done by calling @ref ACS_Stream_attachRecorder.
     *    - Supported streams:
     *      - USB stream (Windows only)
     *      - emulator stream
     * 
     * ## Recording states
     * - The recorder can be in one of the following states:
     *   - `Stopped`
     *   - `Recording`
     *   - `Paused` 
     * - *Note*: Adding frames to the recorder when not in `Recording` state will be ignored, unless pre-recording is enabled.
     * 
     * - Use @ref ACS_ThermalSequenceRecorder_getState to get the current state of the recorder. 
     * - Use @ref ACS_ThermalSequenceRecorder_start, @ref ACS_ThermalSequenceRecorder_pause, @ref ACS_ThermalSequenceRecorder_resume, and @ref ACS_ThermalSequenceRecorder_stop to change the state of the recorder.
     * 
     * ## Settings
     * ### Compression setting
     * The compression setting controls whether the recorded frames gets compressed or not. 
     * Frames can contain both IR pixels and visual pixels. Different encodings are used for these pixel types; JPEG-LS (lossless) for IR pixels and JPEG for visual pixels.
     * - Default value is false
     * - @ref ACS_ThermalSequenceRecorder_Settings_getEnableCompression
     * - @ref ACS_ThermalSequenceRecorder_Settings_setEnableCompression
     * - *Note*: If the incoming frames are compressed, the compression setting will not affect the recorded frames (frames won't be decompressed).
     * 
     * ### Buffer size setting
     * The circular buffer size controls how many frames can be stored in the internal buffer during processing, before they are dropped.
     * Under most circumstances, the default value is sufficient. However, if frames are dropped, the buffer size can be increased. 
     * The compression setting can affect how fast the buffer fills up, but this also depends on if the incoming frames are compressed or not.
     * - Default value is 30
     * - @ref ACS_ThermalSequenceRecorder_Settings_getSizeCircularBuffer 
     * - @ref ACS_ThermalSequenceRecorder_Settings_setSizeCircularBuffer
     * 
     * ### Frame interval time setting
     * The frame interval time setting, if enabled, controls the minimum time between two frames. Any frames added sooner than this are ignored.
     * - Enable/disable frame interval time:
     *   - Default value is false
     *   - @ref ACS_ThermalSequenceRecorder_Settings_getFrameIntervalTimeEnabled
     *   - @ref ACS_ThermalSequenceRecorder_Settings_setFrameIntervalTimeEnabled
     * - Control the frame interval time in ms:
     *   - Default value is 1000 ms
     *   - @ref ACS_ThermalSequenceRecorder_Settings_getFrameIntervalTimeInMs
     *   - @ref ACS_ThermalSequenceRecorder_Settings_setFrameIntervalTimeInMs
     * 
     * ### Pre-recording setting
     * Pre-recording allows buffering of incoming frames before the recording is actually started. 
     * When an event triggers the recording to start, the configured maximum number of pre-recording frames are also added to the recorded sequence.
     * - Enable/disable pre-recording: 
     *   - Default value is false
     *   - @ref ACS_ThermalSequenceRecorder_Settings_getPrerecordingEnabled 
     *   - @ref ACS_ThermalSequenceRecorder_Settings_setPrerecordingEnabled
     * - Control the maximum number of pre-recording frames:
     *   - Default value is 10
     *   - @ref ACS_ThermalSequenceRecorder_Settings_getPrerecordingMaxFrames 
     *   - @ref ACS_ThermalSequenceRecorder_Settings_setPrerecordingMaxFrames
     */
    
    typedef struct ACS_ThermalSequenceRecorder_ ACS_ThermalSequenceRecorder;

    /** @brief Construction of a thermal sequence recorder object.
     * @relatesalso ACS_ThermalSequenceRecorder
     */
    ACS_API ACS_OWN(ACS_ThermalSequenceRecorder*, ACS_ThermalSequenceRecorder_free) ACS_ThermalSequenceRecorder_alloc(void);

    /** @brief Destruction of a thermal sequence recorder.
     * @relatesalso ACS_ThermalSequenceRecorder
     */
    ACS_API void ACS_ThermalSequenceRecorder_free(const ACS_ThermalSequenceRecorder* recorder);

    /** @brief Start recording.
     * 
     * Set the [recorder state](@ref ACS_RecorderState_) to `Recording`.
     * 
     * Note:
     * - If the recorder is attached to a stream, then frames will be recorded automatically by the stream after start is called.
     * - If the recorder is not attached to a stream, then frames must be recorded explicitly by calling @ref ACS_ThermalSequenceRecorder_addImage.
     * - If pre-recording is enabled, then the pre-recorded frames will be added to the recording upon calling start.
     *
     * @param fullPath The destination file to which frames will be written.
     * @see ACS_ThermalSequenceRecorder_stop, ACS_ThermalSequenceRecorder_pause, ACS_ThermalSequenceRecorder_resume
     * @relatesalso ACS_ThermalSequenceRecorder
     */
    ACS_API void ACS_ThermalSequenceRecorder_start(ACS_ThermalSequenceRecorder* recorder, const ACS_NativePathChar* fullPath);

    /** @brief Stop recording and save the destination file.
     * 
     * Set the [recorder state](@ref ACS_RecorderState_) to `Stopped`.
     * Stopping the recorder will keep processing the buffer until the queue is empty.
     * 
     * @see ACS_ThermalSequenceRecorder_start, ACS_ThermalSequenceRecorder_pause, ACS_ThermalSequenceRecorder_resume
     * @relatesalso ACS_ThermalSequenceRecorder
     */
    ACS_API void ACS_ThermalSequenceRecorder_stop(ACS_ThermalSequenceRecorder* recorder);

    /** @brief Pause recording.
     * 
     * If called when in `Recording` state: 
     * - The [recorder state](@ref ACS_RecorderState_) will be set to `Paused`.
     * - Elapsed recording time will be paused.
     * 
     * @see ACS_ThermalSequenceRecorder_start, ACS_ThermalSequenceRecorder_stop, ACS_ThermalSequenceRecorder_resume
     * @relatesalso ACS_ThermalSequenceRecorder
     */
    ACS_API void ACS_ThermalSequenceRecorder_pause(ACS_ThermalSequenceRecorder* recorder);

    /** @brief Resume recording.
     * 
     * If called when in `Paused` state:
     * - The [recorder state](@ref ACS_RecorderState_) will be set to `Recording`.
     * - Elapsed time will be resumed.
     * 
     * @see ACS_ThermalSequenceRecorder_start, ACS_ThermalSequenceRecorder_stop, ACS_ThermalSequenceRecorder_pause
     * @relatesalso ACS_ThermalSequenceRecorder
     */
    ACS_API void ACS_ThermalSequenceRecorder_resume(ACS_ThermalSequenceRecorder* recorder);

    /** @brief Add FFF frame to the recorder.
     *
     * @param fff  A reference to a FFF frame.
     * @param length length of frame data in bytes
     * @note Adding frames to the recorder when not in `Recording` state (see @ref ACS_RecorderState_) will be ignored, unless pre-recording is enabled.
     * @relatesalso ACS_ThermalSequenceRecorder
     */
    ACS_API void ACS_ThermalSequenceRecorder_add(ACS_ThermalSequenceRecorder* recorder, const unsigned char* fff, int length);

    /** @brief Add ThermalImage to the recorder.
     *
     * @param image  Thermal image to add.
     * @note Adding frames to the recorder when not in `Recording` state (see @ref ACS_RecorderState_) will be ignored, unless pre-recording is enabled.
     * @relatesalso ACS_ThermalSequenceRecorder
     */
    ACS_API void ACS_ThermalSequenceRecorder_addImage(ACS_ThermalSequenceRecorder* recorder, ACS_ThermalImage* image);

    /** @brief Get the number of recorded frames.
     *
     * @return Number of recorded frames.
     * @relatesalso ACS_ThermalSequenceRecorder
     */
    ACS_API size_t ACS_ThermalSequenceRecorder_getFrameCounter(const ACS_ThermalSequenceRecorder* recorder);

    /** @brief Get the number of lost frames.
     *
     * @return Number of lost frames.
     * @relatesalso ACS_ThermalSequenceRecorder
     */
    ACS_API size_t ACS_ThermalSequenceRecorder_getLostFramesCounter(const ACS_ThermalSequenceRecorder* recorder);

    /** @brief Get the elapsed recording time in milliseconds.
     *
     * @return Elapsed recording time in milliseconds.
     * @relatesalso ACS_ThermalSequenceRecorder
     */
    ACS_API size_t ACS_ThermalSequenceRecorder_elapsedMilliSeconds(const ACS_ThermalSequenceRecorder* recorder);

    /** @brief Get the recorder state.
     * 
     * @return The @ref ACS_RecorderState_.
     * @relatesalso ACS_ThermalSequenceRecorder
     */
    ACS_API int ACS_ThermalSequenceRecorder_getState(const ACS_ThermalSequenceRecorder* recorder);

    /** @brief Get whether recording timespan is enabled.
     * 
     * @relatesalso ACS_ThermalSequenceRecorder
     */
    ACS_API bool ACS_ThermalSequenceRecorder_Settings_getFrameIntervalTimeEnabled(const ACS_ThermalSequenceRecorder* recorder);

    /** @brief Set whether recording timespan is enabled.
     * @relatesalso ACS_ThermalSequenceRecorder
     */
    ACS_API void ACS_ThermalSequenceRecorder_Settings_setFrameIntervalTimeEnabled(ACS_ThermalSequenceRecorder* recorder, bool value);

    /** @brief Get recording timespan in ms.
     * @relatesalso ACS_ThermalSequenceRecorder
     */
    ACS_API size_t ACS_ThermalSequenceRecorder_Settings_getFrameIntervalTimeInMs(const ACS_ThermalSequenceRecorder* recorder);

    /** @brief Set recording timespan in ms.
     * @relatesalso ACS_ThermalSequenceRecorder
     */
    ACS_API void ACS_ThermalSequenceRecorder_Settings_setFrameIntervalTimeInMs(ACS_ThermalSequenceRecorder* recorder, size_t milliSeconds);

    /** @brief Get whether pre-recording is enabled.
     * @relatesalso ACS_ThermalSequenceRecorder
     */
    ACS_API bool ACS_ThermalSequenceRecorder_Settings_getPrerecordingEnabled(const ACS_ThermalSequenceRecorder* recorder);

    /** @brief Set whether pre-recording is enabled.
     * @relatesalso ACS_ThermalSequenceRecorder
     */
    ACS_API void ACS_ThermalSequenceRecorder_Settings_setPrerecordingEnabled(ACS_ThermalSequenceRecorder* recorder, bool value);

    /** @brief Get the maximum number of pre-recording frames.
     * @relatesalso ACS_ThermalSequenceRecorder
     */
    ACS_API size_t ACS_ThermalSequenceRecorder_Settings_getPrerecordingMaxFrames(const ACS_ThermalSequenceRecorder* recorder);

    /** @brief Set the maximum number of pre-recording frames.
     * @relatesalso ACS_ThermalSequenceRecorder
     */
    ACS_API void ACS_ThermalSequenceRecorder_Settings_setPrerecordingMaxFrames(ACS_ThermalSequenceRecorder* recorder, size_t maxFrames);

    /**@brief Get maximum number of frames in circular buffer.
     * @relatesalso ACS_ThermalSequenceRecorder
     */
    ACS_API size_t ACS_ThermalSequenceRecorder_Settings_getSizeCircularBuffer(const ACS_ThermalSequenceRecorder* recorder);

    /**@brief Set maximum number of frames in circular buffer.
     * @relatesalso ACS_ThermalSequenceRecorder
     */
    ACS_API void ACS_ThermalSequenceRecorder_Settings_setSizeCircularBuffer(ACS_ThermalSequenceRecorder* recorder, size_t maxFrames);

    /**@brief Get the enableCompression flag.
     * @relatesalso ACS_ThermalSequenceRecorder
     */
    ACS_API bool ACS_ThermalSequenceRecorder_Settings_getEnableCompression(ACS_ThermalSequenceRecorder* recorder);

    /**@brief Set the enableCompression flag.
    * @relatesalso ACS_ThermalSequenceRecorder
    */
    ACS_API void ACS_ThermalSequenceRecorder_Settings_setEnableCompression(ACS_ThermalSequenceRecorder* recorder, bool enable);

#ifdef __cplusplus
}
#endif

#endif // ACS_THERMALSEQUENCERECORDER_H
