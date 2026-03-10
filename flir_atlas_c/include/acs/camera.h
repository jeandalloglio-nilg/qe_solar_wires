/** @file
 * @brief ACS Camera API
 * @author Teledyne FLIR
 * @copyright Copyright 2023: Teledyne FLIR
 */

#ifndef ACS_CAMERA_H
#define ACS_CAMERA_H

#include "common.h"
#include "discovery.h"
#include "import.h"
#include "remote.h"
#include "stream.h"

#ifdef __cplusplus
extern "C"
{
#endif
    /** @struct ACS_Camera
     * @brief Manages a FLIR camera
     *
     * @note This class is not thread-safe, so calls must be synchronised if the object is accessed from multiple threads.
     */
    typedef struct ACS_Camera_ ACS_Camera;

    /** @struct ACS_SecurityParameters
     * @brief Security-related parameters used for camera connections
     * @note The security parameters may contain information regarding a certificate which allows Transport Layer Security (TLS/SSL)
     */
    typedef struct ACS_SecurityParameters_ ACS_SecurityParameters;

    /** @brief General parameters used for camera connections
     */
    typedef struct ACS_ConnectParameters_ ACS_ConnectParameters;

    /** @brief Status of a request to register with a camera to allow secure communication
     */
    enum ACS_AuthenticationStatus {
        ACS_AuthenticationStatus_unknown,    ///< Represents the case where status cannot be retrieved/determined
        ACS_AuthenticationStatus_approved,   ///< Approved to communicate with the camera 
        ACS_AuthenticationStatus_pending     ///< Waiting for user to acknowledge the connection on the camera display or in a web-based user interface, depending on the camera  
    };

    /** @struct ACS_AuthenticationResponse_
     * @brief Represents the result of a request to register with the camera for secure communication
     */
    struct ACS_AuthenticationResponse_
    {
        int authenticationStatus; ///< A status value corresponding to a @ref ACS_AuthenticationStatus 
    };
    typedef struct ACS_AuthenticationResponse_ ACS_AuthenticationResponse;


    /** @brief Callback type for when the camera is erroneously disconnected.
     *
     * This callback will _not_ be called if the camera is intentionally via @ref ACS_Camera_disconnect or @ref ACS_Camera_free.
     * @param err Error information describing why the camera was disconnected.
     * @param context User-provided data.
     */
    typedef void(*ACS_OnDisconnected)(ACS_Error err, void* context);

    /** @brief Constructs a camera object.
     * @relatesalso ACS_Camera
     */
    ACS_API ACS_OWN(ACS_Camera*, ACS_Camera_free) ACS_Camera_alloc(void);

    /** @brief Frees a camera object.
     * Will @ref ACS_Camera_disconnect "disconnect" the camera.
     * @relatesalso ACS_Camera
     */
    ACS_API void ACS_Camera_free(ACS_Camera* camera);

    /** @brief Helper constant for using default timeout when registering with camera*/
    #define ACS_AUTHENTICATE_USE_DEFAULT_TIMEOUT ((int) -1)

    /** @brief Registers application with camera to allow secure communication.
     * @note Requires interaction on the camera side, either through the camera's display (if present) or through a web-based GUI. The type of interaction depends on the camera used.
     * @param camera Camera structure for the camera to register with
     * @param identity An identifier that represents the device to register with.
     * @param certificatePath The path at which the certificate used for the registration will be created.
     * @param baseName The first part of the certificate file name (the part shared between public and private key files).
     * @param commonName An arbitrary name. This name should uniquely identify the connecting resource (machine and application, etc.).
     * @param timeoutMs Connection timeout in ms. The constant @ref ACS_AUTHENTICATE_USE_DEFAULT_TIMEOUT may be used to apply a default timeout. Note that the value 0 will NOT be treated as an endless timeout, and will be replaced by the default timeout.
     * @relatesalso ACS_Camera
     */
    ACS_API ACS_AuthenticationResponse ACS_Camera_authenticate(ACS_Camera* camera, const ACS_Identity* identity, const char* certificatePath, const char* baseName, const char* commonName, int timeoutMs);

    /** @brief Connect with the camera.
     *
     * @note This function is blocking.
     * @note A call to connect or disconnect invalidates all pointers returned from a ACS_Camera_ (e.g. anything returned from @ref ACS_Camera_getImporter and @ref ACS_Camera_getRemoteControl).
     *
     * @param camera The camera object to connect with.
     * @param identity An identity that represent the device to connect to.
     * @param securityParameters Currently unsupported, should be `NULL`.
     * @param onDisconnected Called when the camera is disconnected unexpectedly. This argument is required.
     * @param context User-provided data used in callbacks.
     * @param connectParameters Currently unsupported, should be `NULL`.
     * @return An error code on failure, or an object with @ref ACS_Error::code of `0` on success.
     * @post @ref ACS_Camera_isConnected returns `true` if connect succeeded, or `false` if connect failed.
     * @relatesalso ACS_Camera
     */
    ACS_API ACS_Error ACS_Camera_connect(ACS_Camera* camera, const ACS_Identity* identity, const ACS_SecurityParameters* securityParameters, ACS_OnDisconnected onDisconnected, void* context, const ACS_ConnectParameters* connectParameters);

    /** @copydoc ACS_Camera_connect */
    ACS_API ACS_Error ACS_Camera_connect2(ACS_Camera* camera, const ACS_Identity* identity, const ACS_SecurityParameters* securityParameters, ACS_OnDisconnected onDisconnected, ACS_CallbackContext context, const ACS_ConnectParameters* connectParameters);

    /** @brief Disconnect from the camera.
     *
     * @note This function is blocking.
     * @note A call to connect or disconnect invalidates all pointers returned from a camera (e.g. anything returned from @ref ACS_Camera_getImporter and @ref ACS_Camera_getRemoteControl).
     * @post @ref ACS_Camera_isConnected returns `false`.
     * @relatesalso ACS_Camera
     */
    ACS_API void ACS_Camera_disconnect(ACS_Camera* camera);
    
    /** @brief Get the connection status
     * @relatesalso ACS_Camera
     */
    ACS_API bool ACS_Camera_isConnected(const ACS_Camera* camera);

    /** @brief Get camera importer controller.
     *
     * @retval NULL Returned if the camera isn't in connected state or the camera doesn't support image import.
     * @relatesalso ACS_Camera
     */
    ACS_API ACS_Importer* ACS_Camera_getImporter(ACS_Camera* camera);

    /** @brief Get camera exporter controller.
     *
     * @retval NULL Returned if the camera isn't in connected state or the camera doesn't support image export.
     * @relatesalso ACS_Camera
     */
    ACS_API ACS_Exporter* ACS_Camera_getExporter(ACS_Camera* camera);

    /** @brief Get camera remote controller.
     *
     * @retval NULL Returned if the camera isn't in connected state or the camera doesn't support remote control.
     * @relatesalso ACS_Camera
     */
    ACS_API ACS_RemoteControl* ACS_Camera_getRemoteControl(ACS_Camera* camera);

    /** @brief Get number of available streaming interfaces.
     *
     * @return Number of available streams. Returns zero if the camera isn't in connected state, or if streaming is not supported.
     * @see ACS_Camera_getStream
     * @relatesalso ACS_Camera
     */
    ACS_API size_t ACS_Camera_getStreamCount(const ACS_Camera* camera);

    /** @brief Get available streaming interface at specified index.
     *
     * The camera object maintains an internal list of available streams, where the list's size equals @ref ACS_Camera_getStreamCount.
     * @param camera Camera object.
     * @param index Zero-based index, see @ref ACS_Camera_getStreamCount.
     * @return The stream at @p index, or `NULL` on any error.
     * @see ACS_Camera_getStreamCount
     * @relatesalso ACS_Camera
     */
    ACS_API ACS_Stream* ACS_Camera_getStream(ACS_Camera* camera, size_t index);

#ifdef __cplusplus
}
#endif

#endif // ACS_CAMERA_H
