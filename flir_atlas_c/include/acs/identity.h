/** @file
 * @brief ACS Camera Identity API
 * @author Teledyne FLIR
 * @copyright Copyright 2023: Teledyne FLIR
 */

#ifndef ACS_IDENTITY_H
#define ACS_IDENTITY_H

#include "common.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /** @brief Type of @ref ACS_CommunicationInterface_ "communication interfaces". Similar to @ref ACS_CommunicationInterfaces except it must contain _exactly one_ enumerator. */
    typedef unsigned int ACS_CommunicationInterface;

    /** @brief Type of @ref ACS_CommunicationInterface_ "communication interfaces". Similar to @ref ACS_CommunicationInterface except it may contain zero or multiple interfaces (combined with bit-or). */
    typedef unsigned int ACS_CommunicationInterfaces;

    /** @brief Types of communication interfaces. 
     * 
     * @note The USB interface is only supported on Windows.
     */
    enum ACS_CommunicationInterface_
    {
        /// USB port. T1K, EXX, T6XX, T4XX
        ACS_CommunicationInterface_usb = 0x01,
        /// Network adapter. A300, A310, AX8
        ACS_CommunicationInterface_network = 0x2,
        /// Emulating device interface
        ACS_CommunicationInterface_emulator = 0x8,
    };

    /** @struct ACS_Identity
     * @brief Represents a Camera identity.
     *
     * Used when discovering and connecting to cameras.
     */
    typedef struct ACS_Identity_ ACS_Identity;

    /** @brief Fill an identity object from a known IP address.
     *
     * It's highly recommended to use the @ref ACS_Discovery class instead to find cameras and obtain their identity.
     * @relatesalso ACS_Identity
     */
    ACS_API ACS_OWN(ACS_Identity*, ACS_Identity_free) ACS_Identity_fromIpAddress(const char* ipAddress);

    /** @brief Copy an identity object.
     * @relatesalso ACS_Identity
     */
    ACS_API ACS_OWN(ACS_Identity*, ACS_Identity_free) ACS_Identity_copy(const ACS_Identity* identity);

    /** @brief Destroy an identity object.
     * @relatesalso ACS_Identity
     */
    ACS_API void ACS_Identity_free(const ACS_Identity* identity);

    /** @brief Get the @ref ACS_CommunicationInterface_ "communication interface" through which the camera will be accessed.
     * @relatesalso ACS_Identity
     */
    ACS_API ACS_CommunicationInterface ACS_Identity_getCommunicationInterface(const ACS_Identity* identity);

    /** @brief Get the IP address i.e. 192.168.1.10
     * @retval NULL Returned if the @ref ACS_Identity_getCommunicationInterface "identity's communication interface" is not @ref ACS_CommunicationInterface_network "network".
     * @relatesalso ACS_Identity
     */
    ACS_API const char* ACS_Identity_getIpAddress(const ACS_Identity* identity);

    /** @brief Get a per session unique Id for the camera
     *
     * This is ONLY unique per session, starting a new scan might make a new Id for the same camera.
     *
     * This may or may not be human-readable. It's recommended to connect to the @ref ACS_Camera "camera" and query for @ref ACS_Remote_CameraInformation "camera information" to get a descriptive camera name.
     * @relatesalso ACS_Identity
     */
    ACS_API const char* ACS_Identity_getDeviceId(const ACS_Identity* identity);

    /** @brief Hosttarget.
     *  @relatesalso ACS_Identity
     */
    ACS_API const char* ACS_Identity_hosttarget(const ACS_Identity* identity);

    /** @brief MAC address.
     *  @relatesalso ACS_Identity
     */
    ACS_API const char* ACS_Identity_macAddress(const ACS_Identity* identity);

    /** @brief Default gateway e.g. 192.168.1.20
     *  @relatesalso ACS_Identity
     */
    ACS_API const char* ACS_Identity_defaultGateway(const ACS_Identity* identity);

    /** @brief Subnet mask address e.g. 255.255.255.0
     *  @relatesalso ACS_Identity
     */
    ACS_API const char* ACS_Identity_subnetMask(const ACS_Identity* identity);

    /** @brief DHCP setting.
     *  @relatesalso ACS_Identity
     */
    ACS_API bool ACS_Identity_isDhcpEnabled(const ACS_Identity* identity);

    /** @brief True if IP settings are valid, otherwise false.
     *  @relatesalso ACS_Identity
     */
    ACS_API bool ACS_Identity_isValid(const ACS_Identity* identity);

    /** @brief Network adapter name.
     *  @relatesalso ACS_Identity
     */
    ACS_API const char* ACS_Identity_adapterInfo_name(const ACS_Identity* identity);

    /** @brief True if adapter is of a wireless ("WiFi") type, otherwise false.
     *  @relatesalso ACS_Identity
     */
    ACS_API bool ACS_Identity_adapterInfo_isWireless(const ACS_Identity* identity);

    /** @brief MAC address.
     *  @relatesalso ACS_Identity
     */
    ACS_API const char* ACS_Identity_adapterInfo_macAddress(const ACS_Identity* identity);

    /** @brief Network adapter IP address e.g. 192.168.1.10
     *  @relatesalso ACS_Identity
     */
    ACS_API const char* ACS_Identity_adapterInfo_ipAddress(const ACS_Identity* identity);

    /** @brief Adapter subnet mask address i.e. 255.255.255.0
     *  @relatesalso ACS_Identity
     */
    ACS_API const char* ACS_Identity_adapterInfo_mask(const ACS_Identity* identity);


#ifdef __cplusplus
}
#endif


#endif // ACS_IDENTITY_H
