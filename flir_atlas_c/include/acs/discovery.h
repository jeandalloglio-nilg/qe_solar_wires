/** @file
 * @brief ACS Discovery API
 * @author Teledyne FLIR
 * @copyright Copyright 2023: Teledyne FLIR
 */

#ifndef ACS_DISCOVERY_H
#define ACS_DISCOVERY_H

#include "identity.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /** @struct ACS_Discovery
     * @brief Camera discovery class
     *
     * Searches for Cameras on all specified communication interfaces.
     *
     * @note All related functions are thread-safe.
     */
    typedef struct ACS_Discovery_ ACS_Discovery;

    /** @struct ACS_DiscoveredCamera
     * @brief Plain information of discovered camera, including connection settings.
     */
    typedef struct ACS_DiscoveredCamera_ ACS_DiscoveredCamera;

    /** @brief Callback type for discovered camera events.
     *
     * @param discoveredCamera Discovered camera information, containing the identity to use when connecting.
     * @param context User-defined data.
     */
    typedef void(*ACS_OnCameraFound)(const ACS_DiscoveredCamera* discoveredCamera, void* context);
    /** @brief Callback type for camera lost events.
     *
     * @param identity Identity of the lost camera.
     * @param context User-defined data.
     */
    typedef void(*ACS_OnCameraLost)(const ACS_Identity* identity, void* context);
    /** @brief Callback type for discovery finished events.
     *
     * @param interface The scanned communication interface that finished.
     * @param context User-defined data.
     */
    typedef void(*ACS_OnDiscoveryFinished)(ACS_CommunicationInterface interface, void* context);
    /** @brief Callback type for discovery error events
     *
     * @param interface The scanned communication interface that failed.
     * @param error Error information.
     * @param context User-defined data.
     */
    typedef void(*ACS_OnDiscoveryError)(ACS_CommunicationInterface interface, ACS_Error error, void* context);


    /** @brief Create a discovery object.
     * @relatesalso ACS_Discovery
     */
    ACS_API ACS_OWN(ACS_Discovery*, ACS_Discovery_free) ACS_Discovery_alloc(void);

    /** @brief Destroy a discovery object.
     * @relatesalso ACS_Discovery
     */
    ACS_API void ACS_Discovery_free(const ACS_Discovery* discovery);

    /** @brief Start scanning on the specified interface.
     *
     * @param discovery Camera discovery object.
     * @param interfaces The communication interfaces to scan on.
     * @param onCameraFound Called for each camera found. This argument is required (must not be `NULL`).
     * @param onDiscoveryError Called when an error occurred. This argument is required (must not be `NULL`).
     * @param onCameraLost Called for each camera lost. This argument is optional (can be `NULL`).
     * @param onDiscoveryFinished Called when discovery is finished. This argument is optional (can be `NULL`).
     * @param context User-provided data used in callbacks.
     *
     * @note This function is non-blocking. Callbacks will be invoked from a separate thread.
     *
     * @note Cameras can be reported as found even if they aren't connectable, depending on the implementation (entries could be cached in system services).
     *       Similarly camera lost events might be delayed, depending on the implementation (some implementations are only refreshed once per 5 minutes).
     *       One should @ref ACS_Camera_connect "connect" to the camera if faster feedback is required.
     * @relatesalso ACS_Discovery
     */
    ACS_API void ACS_Discovery_scan(
        ACS_Discovery* discovery,
        ACS_CommunicationInterfaces interfaces,
        ACS_OnCameraFound onCameraFound,
        ACS_OnDiscoveryError onDiscoveryError,
        ACS_OnCameraLost onCameraLost,
        ACS_OnDiscoveryFinished onDiscoveryFinished,
        void* context);

    /** @brief Stop scanning on all interfaces
     *
     * @note This function is non-blocking.
     * @note This function will not trigger any @ref ACS_OnDiscoveryFinished events.
     * @relatesalso ACS_Discovery
     */
    ACS_API void ACS_Discovery_stop(ACS_Discovery* discovery);

    /** @brief Check if scanning is active on any @ref ACS_CommunicationInterface "communication interface".
     * @relatesalso ACS_Discovery
     */
    ACS_API bool ACS_Discovery_isScanning(const ACS_Discovery* discovery);

    /** @brief Get the ACS_Identity to use when @ref ACS_Camera_connect "connecting" to a camera.
      @relatesalso ACS_DiscoveredCamera
     */
    ACS_API const ACS_Identity* ACS_DiscoveredCamera_getIdentity(const ACS_DiscoveredCamera* discoveredCamera);

    /** @brief Get human readable name of the camera.
      @relatesalso ACS_DiscoveredCamera
     */
    ACS_API const char* ACS_DiscoveredCamera_getDisplayName(const ACS_DiscoveredCamera* discoveredCamera);

#ifdef __cplusplus
}
#endif


#endif // ACS_DISCOVERY_H
