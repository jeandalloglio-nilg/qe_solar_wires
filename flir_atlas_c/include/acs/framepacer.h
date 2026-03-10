/** @file
 * @brief FramePacer API
 * @author Teledyne FLIR
 * @copyright Copyright 2023: Teledyne FLIR
 */

#ifndef ACS_FRAMEPACER_H
#define ACS_FRAMEPACER_H

#include "common.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**@brief Describes how frame synchronization should be performed in @ref ACS_FramePacer*/
    enum ACS_FrameSynchronizationStrategy_
    {
        /** @brief Sleep until next frame start. Can theoretically give some frames a shorter time to render due to oversleep, but should work well on most platforms.
         * @remarks Prefer using this whenever possible.
         */
        ACS_FrameSynchronizationStrategy_ThreadSleep,

        /**@brief Wait for next frame start time in a tight loop.
         * @remarks Performance/efficiency characteristics may vary widely between platforms, measuring is advised. 
         */
        ACS_FrameSynchronizationStrategy_Spinlock,

        /**@brief Sets frame start time to the current time without waiting.
         * @remarks Allows user-defined synchronization.
         */
        ACS_FrameSynchronizationStrategy_Manual
    };

    /** @struct ACS_FramePacer
     * @brief Used for keeping correct render frame rate for sequences. 
     */
    typedef struct ACS_FramePacer_ ACS_FramePacer;
    

    /** @brief Create a frame pacer object.
     * @param fps Target frame rate.
     * @param enableLogging Enables logging when `true`.
     * @param logIntervalInFrames Interval between frame rate log writes (only used when logging is enabled). 
     * @relatesalso ACS_FramePacer
     */
    ACS_API ACS_OWN(ACS_FramePacer*, ACS_FramePacer_free) ACS_FramePacer_alloc(double fps, bool enableLogging, int logIntervalInFrames);

    
    /** @brief Destroy a frame pacer object.
     * @relatesalso ACS_FramePacer
     */
    ACS_API void ACS_FramePacer_free(const ACS_FramePacer* framePacer);

    /**@brief Gets the target frame rate.
      * @relatesalso ACS_FramePacer
     */
    ACS_API double ACS_FramePacer_getFrameRate(const ACS_FramePacer* framePacer);

    /** @brief Changes target frame rate. 
     * @remarks When calling this function, consider if FramePacer should be reset (@ref reset).
     * @remarks If logging, consider if log interval should be changed (@ref changeLogInterval).
     * @param framePacer A frame pacer object.
     * @param fps New target frame rate.
     * @relatesalso ACS_FramePacer
     */
    ACS_API void ACS_FramePacer_setFrameRate(ACS_FramePacer* framePacer, double fps);

    /**@brief Returns whether or not FramePacer instance is set to log.
     * @returns true if logging, otherwise false.
     * @relatesalso ACS_FramePacer
     */
    ACS_API bool ACS_FramePacer_getLogging(const ACS_FramePacer* framePacer);

    /**@brief Enable/disable logging.
     * @relatesalso ACS_FramePacer
     */
    ACS_API void ACS_FramePacer_setLogging(ACS_FramePacer* framePacer, bool enable);

    /**@brief Get how often logging is be performed (as number of frames between log entries).
     * @relatesalso ACS_FramePacer
     */
    ACS_API int ACS_FramePacer_getLogInterval(const ACS_FramePacer* framePacer);

    /**@brief Set (in number of frames) how often logging should be performed.
     * @relatesalso ACS_FramePacer
     */
    ACS_API void ACS_FramePacer_setLogInterval(ACS_FramePacer* framePacer, int logIntervalInFrames);

    /**@brief Resets timers and counters.
     * @remarks Should be called just before render loop begins if FramePacer instantiation is separated from render loop.
     * @remarks Discards logging information. For manual frame synchronization use @ref ACS_FramePacer_frameSync with @ref ACS_FrameSynchronizationStrategy_Manual instead.
     * @relatesalso ACS_FramePacer
     */
    ACS_API void ACS_FramePacer_reset(ACS_FramePacer* framePacer);

    /**@brief Synchronizes frame rate.
     * @remarks Call this once per iteration in render loop.
     * @param framePacer A frame pacer object.
     * @param frameSynchronizationStrategy A @ref ACS_FrameSynchronizationStrategy_
     * @relatesalso ACS_FramePacer
     */
    ACS_API void ACS_FramePacer_frameSync(ACS_FramePacer* framePacer, int frameSynchronizationStrategy);

    /**@brief Time in microseconds since last frame sync.
     * @relatesalso ACS_FramePacer
     */
    ACS_API int ACS_FramePacer_usedFrameTimeUS(const ACS_FramePacer* framePacer);

    /**@brief Remaining time in microseconds until next frame should start.
     * @returns A positive number if there is time left, otherwise zero.
     * @relatesalso ACS_FramePacer
     */
    ACS_API int ACS_FramePacer_remainingFrameTimeUS(const ACS_FramePacer* framePacer);


    #ifdef __cplusplus
}
#endif


#endif // ACS_FRAMEPACER_H