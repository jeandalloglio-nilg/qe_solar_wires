/** @file
 * @brief ACS Thermal Sequence Player API
 * EXPERIMENTAL! Warning, these APIs are subject to change in future releases
 * @author Teledyne FLIR
 * @copyright Copyright 2023: Teledyne FLIR 
 */

#ifndef ACS_THERMAL_SEQUENCE_PLAYER_H
#define ACS_THERMAL_SEQUENCE_PLAYER_H

#include "common.h"
#include <acs/thermal_image.h>

#ifdef __cplusplus
extern "C"
{
#endif
    /** @struct ACS_ThermalSequencePlayer
     * @brief Represents a ThermalSequencePlayer object that can handle sequence files.
     */
    typedef struct ACS_ThermalSequencePlayer_ ACS_ThermalSequencePlayer;

    /** @brief Construction of a thermal sequence player object by opening a file containing a thermal sequence.
     *
     * @param filePath     Path to file.
     * @relatesalso ACS_ThermalSequencePlayer
     */
    ACS_API ACS_OWN(ACS_ThermalSequencePlayer*, ACS_ThermalSequencePlayer_free) ACS_ThermalSequencePlayer_alloc(const ACS_NativePathChar* filePath);

    /** @brief Construction of a thermal sequence player object by providing a thermal image file.
     *
     * @param image ACS_ThermalImage that is loaded into the player.
     * @relatesalso ACS_ThermalSequencePlayer
     */
    ACS_API ACS_OWN(ACS_ThermalSequencePlayer*, ACS_ThermalSequencePlayer_free) ACS_ThermalSequencePlayer_fromThermalImageFile(ACS_ThermalImage* image);

    /** @brief Destruction of a thermal sequence player.
     * @relatesalso ACS_ThermalSequencePlayer
     */
    ACS_API void ACS_ThermalSequencePlayer_free(const ACS_ThermalSequencePlayer* player);

    /** @brief Gets the number of frames in sequence.
     * @relatesalso ACS_ThermalSequencePlayer
     */
    ACS_API size_t ACS_ThermalSequencePlayer_frameCount(const ACS_ThermalSequencePlayer* player);

    /** @brief Gets the selected frame.
     * 
     * @param index    Frame index.
     * @returns Pointer to ThermalImage from sequence if index is valid, otherwise NULL.
     * @relatesalso ACS_ThermalSequencePlayer
     */
    ACS_API ACS_ThermalImage* ACS_ThermalSequencePlayer_getFrame(ACS_ThermalSequencePlayer* player, size_t index);

    /** @brief Gets the current frame.
     * @relatesalso ACS_ThermalSequencePlayer
     */
    ACS_API ACS_ThermalImage* ACS_ThermalSequencePlayer_getCurrentFrame(ACS_ThermalSequencePlayer* player); 

    /** @brief Move to the first image in the sequence, if not already selected.
     * @relatesalso ACS_ThermalSequencePlayer
     */
    ACS_API void ACS_ThermalSequencePlayer_first(ACS_ThermalSequencePlayer* player); 

    /** @brief Move to the last image in the sequence, if not already selected.
     * @relatesalso ACS_ThermalSequencePlayer
     */
    ACS_API void ACS_ThermalSequencePlayer_last(ACS_ThermalSequencePlayer* player);

    /** @brief Move to the previous frame.
     *
     * @returns True if not already at the beginning of the file, otherwise false.
     * @relatesalso ACS_ThermalSequencePlayer
     */
    ACS_API bool ACS_ThermalSequencePlayer_previous(ACS_ThermalSequencePlayer* player); 

    /** @brief Move to the next frame.
     *
     * @returns True if not at the end of file, otherwise false.
     * @relatesalso ACS_ThermalSequencePlayer
     */
    ACS_API bool ACS_ThermalSequencePlayer_next(ACS_ThermalSequencePlayer* player); 

    /** @brief Gets the selected frame number.
     * @remarks Index is zero-based.
     * @relatesalso ACS_ThermalSequencePlayer
     */
    ACS_API size_t ACS_ThermalSequencePlayer_getSelectedIndex(ACS_ThermalSequencePlayer* player);

    /** @brief Sets the selected frame number. 
     * @remarks Index is zero-based.
     * @remarks If index is out-of-bounds, a global error code will be set (@see ACS_getLastError).  
     * @param newIndex    New frame index.
     * @relatesalso ACS_ThermalSequencePlayer
     */
    ACS_API void ACS_ThermalSequencePlayer_setSelectedIndex(ACS_ThermalSequencePlayer* player, size_t newIndex); 

    /** @brief Gets the playback rate (frames per second).
     *
     * The current playback rate on the sequence.
     * @relatesalso ACS_ThermalSequencePlayer
     */
    ACS_API double ACS_ThermalSequencePlayer_getPlaybackRate(const ACS_ThermalSequencePlayer* player);

    /** @brief Sets the playback rate (frames per second).
     *
     * Play a sequence in requested frame rate.
     * @param newFramerate    Desired frame rate in frames per second.
     * @relatesalso ACS_ThermalSequencePlayer
     */
    ACS_API void ACS_ThermalSequencePlayer_setPlaybackRate(ACS_ThermalSequencePlayer* player, double newFramerate);
        
    /** @brief Access frame and operate on it with a user-supplied function.
     *
     * @param func User-supplied frame manipulation function.
     * @param context User data for use by \p func.
     * @relatesalso ACS_ThermalSequencePlayer
     */
    ACS_API void ACS_ThermalSequencePlayer_withFrame(ACS_ThermalSequencePlayer* player, size_t index, void (*func)(ACS_ThermalImage*, void*), void* context);

    /** @brief Iterate over all thermal images.
     * @relatesalso ACS_ThermalSequencePlayer
     */
    ACS_API void ACS_ThermalSequencePlayer_forEach(ACS_ThermalSequencePlayer* player, void(*func)(ACS_ThermalImage*, void*), void* context);


    /** @brief Iterate over a range of thermal images.
     * @relatesalso ACS_ThermalSequencePlayer
     */
    ACS_API void ACS_ThermalSequencePlayer_forEachInRange(ACS_ThermalSequencePlayer* player, size_t firstIndex, size_t lastIndex, void(*func)(ACS_ThermalImage*, void*), void* context);

    /** @brief Iterate over a range of thermal images.
     * @relatesalso ACS_ThermalSequencePlayer
     */
    ACS_API void ACS_ThermalSequencePlayer_forEachEnumerate(ACS_ThermalSequencePlayer* player, size_t firstIndex, size_t lastIndex, void(*func)(size_t index, ACS_ThermalImage*, void* context));

#ifdef __cplusplus
}
#endif


#endif // ACS_THERMAL_SEQUENCE_PLAYER_H
