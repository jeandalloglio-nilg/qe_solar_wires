/** @file
 * @brief ACS Remote API
 * @author Teledyne FLIR
 * @copyright Copyright 2023: Teledyne FLIR
 */

#ifndef ACS_REMOTE_H
#define ACS_REMOTE_H

#include <acs/common.h>
#include <acs/import.h>
#include <acs/measurements.h>
#include <acs/thermal_value.h>

#include "thermal_image.h"


#ifdef __cplusplus
extern "C"
{
#endif

    /** @struct ACS_RemoteControl
     * @brief Camera remote control. Can read, manipulate and listen on events from a Camera.
     *
     * Operating on properties of the remote control will send requests to the camera.
     *
     * @see ACS_Camera_getRemoteControl
     */
    typedef struct ACS_RemoteControl_ ACS_RemoteControl;

    /** @struct ACS_Remote_CameraInformation
     * @brief Represents camera device information.
     */
    typedef struct ACS_Remote_CameraInformation_ ACS_Remote_CameraInformation;

    /** @struct ACS_RemotePalette
     * @brief Represents a remote (non-local) palette.
     */
    typedef struct ACS_RemotePalette_ ACS_RemotePalette;

    /** @struct ACS_ListRemotePalette
     * @brief Represents a list of remote (non-local) palettes.
     */
    typedef struct ACS_ListRemotePalette_ ACS_ListRemotePalette;

    /** @struct ACS_StoredImage
     * @brief Path to a stored image (or IR+visual pair of images) on a camera.
     */
    typedef struct ACS_StoredImage_ ACS_StoredImage;

    /** @struct ACS_StoredLocalImage
     * @brief Path to an imported image on the local file system (NOT on the camera).
     */
    typedef struct ACS_StoredLocalImage_ ACS_StoredLocalImage;

    /** @struct ACS_StoredMemoryImage
     * @brief Memory buffer containing an imported image.
     */
    typedef struct ACS_StoredMemoryImage_ ACS_StoredMemoryImage;

    /** @struct ACS_MeasurementCalcMaskFlags
     *  @brief Calculation mask for measurement area
     */
    typedef struct ACS_MeasurementCalcMaskFlags_ ACS_MeasurementCalcMaskFlags;

    /** @brief File format of stored images. */
    enum ACS_Storage_FileFormat
    {
        ACS_Storage_FileFormat_unknown,  ///< Unknown or invalid storage format.
        ACS_Storage_FileFormat_jpeg,  ///< Image stored as JPEG (or RJPEG (Radiometric JPEG) if it contains measurable thermal data).
        ACS_Storage_FileFormat_jpegVisual,  ///< Store visual image as JPEG (no radiometric/IR image available).
        ACS_Storage_FileFormat_jpegFusionPng,  ///< Store fused radiometric image and visual image as RJPEG using PNG compression.
        ACS_Storage_FileFormat_irVisual,  ///< Store radiometric image and visual image into @ref ACS_Storage_FileFormat_isCombo "separate files".
    };

    /** @brief Defines the focus calculation method. I.e. based on maximizing the image contrast or based on laser distance measurement.      */
    enum ACS_Focus_CalculationMethod
    {
        /// calculation based on maximizing the image contrast
        ACS_Focus_CalculationMethod_contrast,
        /// calculation based on laser distance measurement
        ACS_Focus_CalculationMethod_laser,
        /// calculation method automatically chosen by the camera (if supported)
        ACS_Focus_CalculationMethod_automatic
    };

    /**@brief Values corresponding to modes used by cameras for firefighting.*/
    enum ACS_FireCamera_Mode
    {
        ACS_FireCamera_Mode_basic,     ///< Default mode for firefighting. Basic mode on K45, K55. TI Basic NFPA on K65.
        ACS_FireCamera_Mode_search,    ///< Search mode
        ACS_FireCamera_Mode_detection, ///< Detection mode
        ACS_FireCamera_Mode_fire,      ///< Fire mode. Similar to Basic/NFPA mode but with a higher threshold for heat colorization.
        ACS_FireCamera_Mode_whiteHot   ///< Heat detection mode. Optimized for searching for hotspots during overhaul after fire is out.
    };

    

    /**@brief Time display formats used by cameras.*/
    enum ACS_TimeDisplayFormat
    {
        ACS_TimeDisplayFormat_12H,   ///< 12h time display (i.e. a.m./p.m.)
        ACS_TimeDisplayFormat_24H    ///< 24h unambiguous time display (U.S.: "military time")
    };

    /**@brief Date display formats used by cameras.*/
    enum ACS_DateDisplayFormat
    {
        ACS_DateDisplayFormat_ymd,   ///< YYYY-MM-DD
        ACS_DateDisplayFormat_mdy,   ///< MM/DD/YYYY
        ACS_DateDisplayFormat_dmy    ///< DD/MM/YYYY
    };

    /**@brief GUI HotSpot shapes used by cameras for firefighting.*/
    enum ACS_FireCamera_HotSpotShape
    {
        ACS_FireCamera_HotSpotShape_disabled,   ///< Disabled.
        ACS_FireCamera_HotSpotShape_nfpa,       ///< NFPA/basic mode style.
        ACS_FireCamera_HotSpotShape_crosshair   ///< Crosshair style.
    };

    /** @brief Camera NUC (Non-Uniform Calibration, also known as FFC) status monitoring interface. */
    enum ACS_NucState_
    {
        /// Indicates that the *last* NUC (Non-Uniform Calibration) is invalid.
        ACS_NucState_invalid,
            
        /// Indicates that there is NUC activity in progress.
        ACS_NucState_progress,
            
        /// Indicates that thermal pixel values are radiometrically calibrated.
        ACS_NucState_validRad,
            
        /// Indicates that thermal pixels are calibrated enough to yield an image, but are not yet radiometrically calibrated.
        ACS_NucState_validImg,
        
        /// Indicates that an NUC is desired.
        ACS_NucState_desired,
            
        /// Indicated that approximate radiometric values are being given
        ACS_NucState_radApprox,
            
        /// Indicates that a corrupted/invalid NUC state was detected.
        ACS_NucState_bad,
        
        /// Indicates a unknown state
        ACS_NucState_unknown
    };

    /** @brief Camera shutter status monitoring interface. */
    enum ACS_ShutterState_
    {
        /// Represents an invalid shutter state. This value should be treated as an error.
        ACS_ShutterState_invalid,
            
        /// Represents the "device powered off" shutter state.
        ACS_ShutterState_off,
            
        /// Represents the "device powered on" state of the hardware shutter.
        ACS_ShutterState_on,
            
        /// Represents the NUC state of the hardware shutter.
        ACS_ShutterState_nuc,
            
        /// Indicates that a corrupted/invalid shutter state was detected.
        ACS_ShutterState_bad,
        
        /// Indicates a unknown state
        ACS_ShutterState_unknown
    };

    /** @enum ACS_ChannelType_
     */
    enum ACS_ChannelType_
    {
        ACS_ChannelType_ir,
        ACS_ChannelType_visual,
        ACS_ChannelType_fusion,
    };

    /** @brief Geometric measurement types. 
     */
    enum ACS_RemoteMeasurementMarkerTypes_
    {
        ACS_RemoteMeasurementMarkerTypes_rectangle,
        ACS_RemoteMeasurementMarkerTypes_circle,
        ACS_RemoteMeasurementMarkerTypes_line
    };

    /** @brief Defines type of a measurement value property. */
    enum ACS_RemoteMeasurementValueType_
    {
        ACS_RemoteMeasurementValueType_average,
        ACS_RemoteMeasurementValueType_min,
        ACS_RemoteMeasurementValueType_max
    };

    /** @brief Defines a type of a measurement marker property. */
    enum ACS_RemoteMarkerType_
    {
        ACS_RemoteMarkerType_coldSpot,
        ACS_RemoteMarkerType_hotSpot
    };

    /** @brief Definition of image's display mode. Depending on selected mode thermal or visual data is used to render the image.
     */
    enum ACS_DisplayMode_
    {
        /** @brief The camera sends video in basic mode. Use this for VISUAL-only mode. */
        ACS_DisplayMode_none = 0,
        /** @brief The camera video in fusion blending mode. Use this for IR-only mode. */
        ACS_DisplayMode_fusion,
        /** @brief The camera video in picture in picture mode. */
        ACS_DisplayMode_pip,
        /** @brief The camera video in multi-spectral image mode. */
        ACS_DisplayMode_msx,
        /** @brief Image displayed a digital. */
        ACS_DisplayMode_digital,
        /** @brief Image displayed as difference. */
        ACS_DisplayMode_diff,
        /** @brief Image displayed as blending between thermal and digital images. */
        ACS_DisplayMode_blending,
    };

    /** @brief Defines span levels for ACS_ChannelType_fusion.
     */
    enum ACS_FusionSpanLevel_
    {
        /** @brief The camera sends video in visual (DC) mode. Used with ACS_DisplayMode_none. */
        ACS_FusionSpanLevel_dc = 0,
        /** @brief The camera sends video in infrared (IR) mode. Used with ACS_DisplayMode_none. */
        ACS_FusionSpanLevel_ir = 1,
        /** @brief The camera sends video in any other mode than ACS_DisplayMode_none. */
        ACS_FusionSpanLevel_thermal_fusion = 7,
    };

    enum ACS_FirmwareUpdate_OperatingMode_
    {
        /// standard operating mode for F1 dongle
        ACS_FirmwareUpdate_OperatingMode_operational,
        /// mode used to perform firmware update
        ACS_FirmwareUpdate_OperatingMode_upgrade
    };

    enum ACS_FirmwareUpdate_Status_
    {
        /// no firmware update is started
        ACS_FirmwareUpdate_Status_noUpdate,
        /// information on firmware update progress
        ACS_FirmwareUpdate_Status_info,
        /// the firmware update failed, camera will be rebooted
        ACS_FirmwareUpdate_Status_failure,
        /// the firmware update was successful, camera will be rebooted
        ACS_FirmwareUpdate_Status_success,
        /// the firmware update is ongoing, camera is rebooting
        ACS_FirmwareUpdate_Status_rebooting,
        /// the firmware update starts writing a package
        ACS_FirmwareUpdate_Status_startWritingPackage,
        /// the firmware update has written a package
        ACS_FirmwareUpdate_Status_doneWritingPackage,
        /// there was a failure writing a package
        ACS_FirmwareUpdate_Status_failureWritingPackage,
        /// the firmware update starts
        ACS_FirmwareUpdate_Status_startExecutingUpdate,
        /// the firmware update is done
        ACS_FirmwareUpdate_Status_doneExecutingUpdate,
        /// the firmware update cannot start, since the camera is in invalid mode
        ACS_FirmwareUpdate_Status_failureInvalidMode,
        /// the package write failed, retry
        ACS_FirmwareUpdate_Status_retryWritingPackage
    };

    /** @struct ACS_ListTemperatureRange
     * @brief Represents a temperature range.
     */
    typedef struct ACS_ListTemperatureRange_ ACS_ListTemperatureRange;

    /** @struct ACS_FireCameraTemperatureRange
     * @brief Temperature range specific to cameras for firefighting. 
     */
    typedef struct ACS_FireCameraTemperatureRange_
    {
        /// Range lower value
        int min;

        /// Range upper value
        int max;

        /// Indicates if camera should automatically switch to high gain mode.
        bool isAuto;
    } ACS_FireCameraTemperatureRange;

    /** @brief Remote camera spot measurement tool.
     */
    typedef struct ACS_RemoteMeasurementSpot_
    {
        int id;
    } ACS_RemoteMeasurementSpot;

    /** @brief Remote camera rectangle shaped measurement.
     */
    typedef struct ACS_RemoteMeasurementRectangle_
    {
        int id;
    } ACS_RemoteMeasurementRectangle;

    /** @brief Remote camera circle shaped measurement.
     */
    typedef struct ACS_RemoteMeasurementCircle_
    {
        int id;
    } ACS_RemoteMeasurementCircle;

    /** @brief Remote camera line shaped measurement.
     */
    typedef struct ACS_RemoteMeasurementLine_
    {
        int id;
    } ACS_RemoteMeasurementLine;

    /** @brief Union of structs representing different types of measurement geometries.
     *  @relatesalso ACS_RemoteMeasurementMarker_Type_
     */
    typedef union ACS_RemoteMeasurementMarker_TypeValue_
    {
        ACS_RemoteMeasurementRectangle rectangle;
        ACS_RemoteMeasurementCircle circle;
        ACS_RemoteMeasurementLine line;
        
    } ACS_RemoteMeasurementMarker_TypeValue;      

    /** @brief Tagged union used for unifying Measurement marker interfaces.
     * @remarks User must ensure that type and value match. 
     */
    typedef struct ACS_RemoteMeasurementMarker_ 
    {
        /**@brief @ref ACS_RemoteMeasurementMarkerTypes_ */
        int type;

        /**@brief The measurement marker value corresponding to the type. Must be valid for correct usage.
         * @remarks Inconsistent usage is invalid. 
         */
        ACS_RemoteMeasurementMarker_TypeValue value;

    } ACS_RemoteMeasurementMarker;

    /** @struct ACS_ListDisplayMode
     * @brief List of fusion display modes.
     */
    typedef struct ACS_ListDisplayMode_ ACS_ListDisplayMode;

    /** @struct ACS_ListFireCameraTemperatureRange
     * @brief Integral temperature range with bool value indicating if range is automatic
     */
    typedef struct ACS_ListFireCameraTemperatureRange_ ACS_ListFireCameraTemperatureRange;

    /** @struct ACS_ListFireCameraMode
     * @brief List of @ref ACS_FireCamera_Mode "fire camera mode" enumeration values.
     */
    typedef struct ACS_ListFireCameraMode_ ACS_ListFireCameraMode;

    /** @struct ACS_FireCameraModeControl
     * @brief Mode used by cameras for firefighting
     */
    typedef struct ACS_FireCameraModeControl_ ACS_FireCameraModeControl;

    typedef void(*ACS_OnReceivedRemoteMeasurementSpot)(ACS_RemoteMeasurementSpot, void*);
    typedef void(*ACS_OnReceivedRemoteMeasurementRectangle)(ACS_RemoteMeasurementRectangle, void*);
    typedef void(*ACS_OnReceivedRemoteMeasurementCircle)(ACS_RemoteMeasurementCircle, void*);
    typedef void(*ACS_OnReceivedRemoteMeasurementLine)(ACS_RemoteMeasurementLine, void*);

    /** @brief Get camera model name.
     * @relatesalso ACS_Remote_CameraInformation
     */
    ACS_API const char* ACS_Remote_CameraInformation_getName(const ACS_Remote_CameraInformation* cameraInfo);

    /** @brief Camera sensor width in pixels.
     * @relatesalso ACS_Remote_CameraInformation
     */
    ACS_API int ACS_Remote_CameraInformation_getResolutionWidth(const ACS_Remote_CameraInformation* cameraInfo);

    /** @brief Camera sensor height in pixels.
     * @relatesalso ACS_Remote_CameraInformation
     */
    ACS_API int ACS_Remote_CameraInformation_getResolutionHeight(const ACS_Remote_CameraInformation* cameraInfo);

    /** @brief Get camera displayname
     * @relatesalso ACS_Remote_CameraInformation
     */
    ACS_API const char* ACS_Remote_CameraInformation_getDisplayName(const ACS_Remote_CameraInformation* cameraInfo);

    /** @brief Get camera description
     * @relatesalso ACS_Remote_CameraInformation
     */
    ACS_API const char* ACS_Remote_CameraInformation_getDescription(const ACS_Remote_CameraInformation* cameraInfo);

    /** @brief Get camera serial number
     * @relatesalso ACS_Remote_CameraInformation
     */
    ACS_API const char* ACS_Remote_CameraInformation_getSerialNumber(const ACS_Remote_CameraInformation* cameraInfo);

    /** @brief Get camera os image kit name
     * @relatesalso ACS_Remote_CameraInformation
     */
    ACS_API const char* ACS_Remote_CameraInformation_getOsImageKitName(const ACS_Remote_CameraInformation* cameraInfo);
    /** @brief Get camera sw combination version
     * @relatesalso ACS_Remote_CameraInformation
     */
    ACS_API const char* ACS_Remote_CameraInformation_getSwCombinationVersion(const ACS_Remote_CameraInformation* cameraInfo);

    /** @brief Get camera configuration kit name
     * @relatesalso ACS_Remote_CameraInformation
     */
    ACS_API const char* ACS_Remote_CameraInformation_getConfKitName(const ACS_Remote_CameraInformation* cameraInfo);

    /** @brief Get camera article
     * @relatesalso ACS_Remote_CameraInformation
     */
    ACS_API const char* ACS_Remote_CameraInformation_getArticle(const ACS_Remote_CameraInformation* cameraInfo);

    /** @brief Get camera software date
     * @relatesalso ACS_Remote_CameraInformation
     */
    ACS_API const char* ACS_Remote_CameraInformation_getDate(const ACS_Remote_CameraInformation* cameraInfo);

    /** @brief Get camera firmware revision
     * @relatesalso ACS_Remote_CameraInformation
     */
    ACS_API const char* ACS_Remote_CameraInformation_getFirmwareRevision(const ACS_Remote_CameraInformation* cameraInfo);

    /** @brief Get camera model name
     * @returns Camera model name if available, otherwise null.
     * @relatesalso ACS_Remote_CameraInformation
     */
    ACS_API const char* ACS_Remote_CameraInformation_getModelName(const ACS_Remote_CameraInformation* cameraInfo);

    /** @brief Get camera hardware type
     * @returns Camera hardware type if available, otherwise null
     * @relatesalso ACS_Remote_CameraInformation
     */
    ACS_API const char* ACS_Remote_CameraInformation_getHwType(const ACS_Remote_CameraInformation* cameraInfo);


    /** @brief Destroy a stored image object.
     * @note Only destroys the in-memory object, will not touch any file on the camera.
     * @relatesalso ACS_StoredImage
     */
    ACS_API void ACS_StoredImage_free(ACS_StoredImage* storedImage);

    /** @brief Get stored thermal image.
     * @relatesalso ACS_StoredImage
     */
    ACS_API const ACS_FileReference* ACS_StoredImage_getThermalImage(const ACS_StoredImage* storedImage);

    /** @brief Get visual JPEG if thermal and visual images were @ref ACS_Storage_FileFormat_isCombo "separated" (`NULL` if they're both stored into the same thermal image file)
     * @relatesalso ACS_StoredImage
     */
    ACS_API const ACS_FileReference* ACS_StoredImage_getVisualImage(const ACS_StoredImage* storedImage);

    /** @brief Destroy a stored local image object.
     * @note Only destroys the in-memory object, will not touch any file in the file system.
     * @relatesalso ACS_StoredLocalImage
     */
    ACS_API void ACS_StoredLocalImage_free(const ACS_StoredLocalImage* storedImage);

    /** @brief Get path to the stored thermal image.
     * @relatesalso ACS_StoredLocalImage
     */
    ACS_API const char* ACS_StoredLocalImage_getThermalImage(const ACS_StoredLocalImage* storedImage);

    /** @brief Get path to the visual JPEG if thermal and visual images were @ref ACS_Storage_FileFormat_isCombo "separated" (`NULL` if they're both stored into the same thermal image file)
     * @relatesalso ACS_StoredLocalImage
     */
    ACS_API const char* ACS_StoredLocalImage_getVisualImage(const ACS_StoredLocalImage* storedImage);

    /** @brief Destroy a stored memory image object.
     * @note Only destroys the in-memory object, will not touch any file in the file system.
     * @relatesalso ACS_StoredMemoryImage
     */
    ACS_API void ACS_StoredMemoryImage_free(const ACS_StoredMemoryImage* storedImage);

    /** @brief Get the stored thermal image buffer.
     * @relatesalso ACS_StoredMemoryImage
     */
    ACS_API const ACS_ByteBuffer* ACS_StoredMemoryImage_getThermalImage(const ACS_StoredMemoryImage* storedImage);

    /** @brief Get the visual JPEG buffer if thermal and visual images were @ref ACS_Storage_FileFormat_isCombo "separated" (`NULL` if they're both stored into the same thermal image file)
     * @relatesalso ACS_StoredMemoryImage
     */
    ACS_API const ACS_ByteBuffer* ACS_StoredMemoryImage_getVisualImage(const ACS_StoredMemoryImage* storedImage);

    /** @brief Test if a @ref ACS_Storage_FileFormat "file format" is a "combo format".
     *
     * Combo file format means that when saved, there'll be two separate image files, one for radiometric/IR and one for visual data.
     * @relatesalso ACS_Storage_FileFormat
     */
    ACS_API bool ACS_Storage_FileFormat_isCombo(int format);


#define ACS_PROPERTY_DECLARE_IMPL(ArgumentType, ReturnType, name) \
        /** @brief Represents a property on the remote camera. */ \
        typedef struct ACS_Property_##name##_ ACS_Property_##name; \
        /** @brief Callback type for successfully read property events. */ \
        typedef void(*ACS_Property_##name##_OnReceived)(ArgumentType, void* context); \
        /** @brief Read from property asynchronously. */ \
        ACS_API void ACS_Property_##name##_get(const ACS_Property_##name* property, ACS_Property_##name##_OnReceived, ACS_OnError onError, void* context); \
        /** @brief Read from property synchronously/blocking. */ \
        ACS_API ReturnType ACS_Property_##name##_getSync(const ACS_Property_##name* property); \
        /** @brief Write to property asynchronously. */ \
        ACS_API void ACS_Property_##name##_set(ACS_Property_##name* property, ArgumentType newValue, ACS_OnCompletion, ACS_OnError onError, void* context); \
        /** @brief Write to property synchronously/blocking. */ \
        ACS_API void ACS_Property_##name##_setSync(ACS_Property_##name* property, ArgumentType newValue); \
        /** @brief Subscribe to changes to the property. */ \
        ACS_API void ACS_Property_##name##_subscribe(const ACS_Property_##name* property, ACS_Property_##name##_OnReceived, void* context); \
        /** @brief Remove subscription. */ \
        ACS_API void ACS_Property_##name##_unsubscribe(const ACS_Property_##name* property)

#define ACS_PROPERTY_VALUE_DECLARE(T, name) ACS_PROPERTY_DECLARE_IMPL(T, T, name)
#define ACS_PROPERTY_POINTER_DECLARE(T, name) ACS_PROPERTY_DECLARE_IMPL(const T*, ACS_OWN(T*, T##_free), name)

    ACS_PROPERTY_VALUE_DECLARE(int, Int);
    ACS_PROPERTY_VALUE_DECLARE(double, Double);
    ACS_PROPERTY_VALUE_DECLARE(bool, Bool);
    ACS_PROPERTY_VALUE_DECLARE(ACS_ThermalValue, ThermalValue);
    ACS_PROPERTY_VALUE_DECLARE(struct tm, Tm);
    ACS_PROPERTY_VALUE_DECLARE(ACS_TemperatureRange, TemperatureRange);
    ACS_PROPERTY_VALUE_DECLARE(ACS_Rectangle, Rectangle);
    ACS_PROPERTY_VALUE_DECLARE(ACS_Circle, Circle);
    ACS_PROPERTY_VALUE_DECLARE(ACS_Point, Point);
    ACS_PROPERTY_VALUE_DECLARE(ACS_Line, Line);
    ACS_PROPERTY_VALUE_DECLARE(ACS_FireCameraTemperatureRange, FireCameraTemperatureRange);
    ACS_PROPERTY_POINTER_DECLARE(ACS_String, String);
    ACS_PROPERTY_POINTER_DECLARE(ACS_MeasurementCalcMaskFlags, MeasurementCalcMaskFlags);
    ACS_PROPERTY_POINTER_DECLARE(ACS_Remote_CameraInformation, CameraInformation);
    ACS_PROPERTY_POINTER_DECLARE(ACS_FileReference, FileReference);
    ACS_PROPERTY_POINTER_DECLARE(ACS_RemotePalette, RemotePalette);
    ACS_PROPERTY_POINTER_DECLARE(ACS_ListRemotePalette, ListRemotePalette);
    ACS_PROPERTY_POINTER_DECLARE(ACS_ListTemperatureRange, ListTemperatureRange);
    ACS_PROPERTY_POINTER_DECLARE(ACS_ListDisplayMode, ListDisplayMode);
    ACS_PROPERTY_POINTER_DECLARE(ACS_ListFireCameraTemperatureRange, ListFireCameraTemperatureRange);
    ACS_PROPERTY_POINTER_DECLARE(ACS_ListFireCameraMode, ListFireCameraMode);
    ACS_PROPERTY_POINTER_DECLARE(ACS_ListRemoteMeasurementSpot, ListRemoteMeasurementSpot);
    ACS_PROPERTY_POINTER_DECLARE(ACS_ListRemoteMeasurementRectangle, ListRemoteMeasurementRectangle);
    ACS_PROPERTY_POINTER_DECLARE(ACS_ListRemoteMeasurementCircle, ListRemoteMeasurementCircle);
    ACS_PROPERTY_POINTER_DECLARE(ACS_ListRemoteMeasurementLine, ListRemoteMeasurementLine);

    
    /** @brief Convenient C-string wrapper for @ref ACS_Property_String_set. */
    ACS_API void ACS_Property_String_setChars(ACS_Property_String* property, const char* newValue, ACS_OnCompletion onCompletion, ACS_OnError onError, void* context);
    /** @brief Convenient C-string wrapper for @ref ACS_Property_String_setSync. */
    ACS_API void ACS_Property_String_setSyncChars(ACS_Property_String* property, const char* newValue);


    /** @brief Get information about that device.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API const ACS_Property_CameraInformation* ACS_Remote_cameraInformation(const ACS_RemoteControl* remote);

    /** @brief Remaining battery percentage.
     * @note Valid value range from 0 to 100 where 0 is empty and 100 is full.
     * Important: In certain cases (e.g. in case of FLIR ONE prior to starting streaming) the battery percentage value may not be available.
     * When the value is not available, an invalid (negative) value is returned.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API const ACS_Property_Int* ACS_Remote_Battery_percentage(const ACS_RemoteControl* remote);

    /** @brief Control camera's date and time settings.
     * Uses the standardized tm struct to transfer date and time information.
     * @note When setting time, some cameras do not respect the input seconds value and will arbitrarily set the seconds value to 0.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_Tm* ACS_Remote_System_time(ACS_RemoteControl* remote);

    /** @brief Control camera's time zone name.
     * See: https://www.iana.org/time-zones
     * See: https://en.wikipedia.org/wiki/List_of_tz_database_time_zones
     * @note Some cameras do not support setting the time zone name.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_String* ACS_Remote_System_timeZoneName(ACS_RemoteControl* remote);

    /** @brief Check if camera startup procedure is done, in particular that all processes are up and running.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API const ACS_Property_Bool* ACS_Remote_System_isSystemUp(const ACS_RemoteControl* remote);

    /**@brief Set camera to perform a factory reset on next reboot
     * @relatesalso ACS_RemoteControl
     */
    ACS_API void ACS_Remote_System_factoryReset_execute(ACS_RemoteControl* remote, ACS_OnCompletion onCompletion, ACS_OnError onError, void* context);

    /**@brief Set camera to perform a factory reset on next reboot
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Error ACS_Remote_System_factoryReset_executeSync(ACS_RemoteControl* remote);

    /**@brief Reboots camera
     * @relatesalso ACS_RemoteControl
     */
    ACS_API void ACS_Remote_System_reboot_execute(ACS_RemoteControl* remote, ACS_OnCompletion onCompletion, ACS_OnError onError, void* context);

    /**@brief Reboots camera
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Error ACS_Remote_System_reboot_executeSync(ACS_RemoteControl* remote);

    /** @brief @ref ACS_TemperatureUnit "Temperature unit" (e.g. Celsius, Fahrenheit) used in the graphical user interface on cameras for firefighting.
     * @note Specific to cameras used for firefighting.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_Int* ACS_Remote_CameraUISettingsControl_temperatureUnit(ACS_RemoteControl* remote);

    /** @brief @ref ACS_FireCamera_TimeDisplayFormat "Time display format" (a.m/p.m or 24h display) used in the graphical user interface on cameras for firefighting.
     * @note supported values: "12H", "24H"
     * @note Specific to cameras used for firefighting.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_Int* ACS_Remote_CameraUISettingsControl_timeDisplayFormat(ACS_RemoteControl* remote);

    /** @brief Date display format (e.g. yyyy-mm-dd) used in the graphical user interface on cameras for firefighting.
     * @note Supported separators: '-', '/'. Supported orders: YY-MM-DD, YYYY-MM-DD, DD-MM-YY, DD-MM-YYYY, MM-DD-YY, MM-DD-YYYY
     * @note Specific to cameras used for firefighting.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_Int* ACS_Remote_CameraUISettingsControl_dateDisplayFormat(ACS_RemoteControl* remote);

    /** @brief Current active @ref ACS_FireCamera_Mode "mode" on cameras for firefighting.
     * @note Specific to cameras used for firefighting.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_Int* ACS_Remote_FireCameraControl_currentMode(ACS_RemoteControl* remote);

    /** @brief @ref ACS_FireCamera_HotSpotShape "Hotspot indicator shape" used in the graphical user interface on cameras for firefighting.
     * @note Specific to cameras used for firefighting.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_Int* ACS_Remote_FireCameraControl_hotSpotShape(ACS_RemoteControl* remote);

    /** @brief show the termperature bar in the graphical user interface on cameras for firefighting.
     * @note Specific to cameras used for firefighting.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_Bool* ACS_Remote_FireCameraControl_showTemperatureBar(ACS_RemoteControl* remote);

    /** @brief show the reference bar in the graphical user interface on cameras for firefighting.
     * @note Specific to cameras used for firefighting.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_Bool* ACS_Remote_FireCameraControl_showReferenceBar(ACS_RemoteControl* remote);

    /** @brief show the digital readout in the graphical user interface on cameras for firefighting.
     * @note Specific to cameras used for firefighting.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_Bool* ACS_Remote_FireCameraControl_showDigitalReadout(ACS_RemoteControl* remote);

    /** @brief turn on and off blackbox (continuous) recording on cameras for firefighting.
     * @note Specific to cameras used for firefighting
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_Bool* ACS_Remote_FireCameraControl_blackBox(ACS_RemoteControl* remote);

    /** @brief Change operation @ref ACS_FireCamera_Mode "mode"
     * @note Specific to cameras used for firefighting.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_Int* ACS_Remote_FireCameraControl_changeMode(ACS_RemoteControl* remote);

    /** @brief Get all @ref ACS_FireCamera_Mode "modes" for fire cameras
     * @note Specific to cameras used for firefighting.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API const ACS_Property_ListFireCameraMode* ACS_Remote_FireCameraControl_allModes(ACS_RemoteControl* remote);

    /** @brief Get available @ref ACS_FireCamera_Mode "modes" for fire cameras (modes with available true)
     * @note Specific to cameras used for firefighting.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API const ACS_Property_ListFireCameraMode* ACS_Remote_FireCameraControl_availableModes(ACS_RemoteControl* remote);

    /**@brief Get object representing fire camera mode
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_FireCameraModeControl* ACS_Remote_FireCameraControl_getMode(ACS_RemoteControl* remote, enum ACS_FireCamera_Mode mode);

    /** @brief True if mode is enabled in the camera's configuration (i.e. the camera model).
     *  @relatesalso ACS_FireCameraModeControl
     */
    ACS_API ACS_Property_Bool* ACS_FireCameraModeControl_available( ACS_FireCameraModeControl* mode);

    /** @brief Query the (fire camera specific) temperature range (including if automatic gain is used) which the camera uses.
     *  @relatesalso ACS_FireCameraModeControl
     */
    ACS_API const ACS_Property_FireCameraTemperatureRange* ACS_FireCameraModeControl_range(const ACS_FireCameraModeControl* mode);

    /** @brief Custom mode high temperature isotherm on cameras for firefighting.
     * @note Specific to cameras used for firefighting.
     * @relatesalso ACS_FireCameraModeControl
     */
    ACS_API ACS_Property_Double* ACS_Remote_FireCameraControl_CustomMode_isoAboveHighT(ACS_RemoteControl* remote);

    /** @brief Custom mode linked high temperature isotherm on cameras for firefighting.
     * @note Specific to cameras used for firefighting.
     * @relatesalso ACS_FireCameraModeControl
     */
    ACS_API ACS_Property_Double* ACS_Remote_FireCameraControl_CustomMode_isoLinkedHighT(ACS_RemoteControl* remote);

    /** @brief Custom mode linked low temperature isotherm on cameras for firefighting.
     * @note Specific to cameras used for firefighting.
     * @relatesalso ACS_FireCameraModeControl
     */
    ACS_API ACS_Property_Double* ACS_Remote_FireCameraControl_CustomMode_isoLinkedLowT(ACS_RemoteControl* remote);

    /** @brief Get a file reference that can be used for importing and exporting custom boot logo from a camera
     * @note Specific to cameras used for firefighting
     * @relatesalso ACS_FireCameraModeControl
     */
    ACS_API const ACS_Property_FileReference* ACS_Remote_FireCameraControl_userBootLogo(ACS_RemoteControl* remote);

    /** @brief Get size of the list (i.e. mode count).
     * @relates ACS_ListFireCameraMode
     */
    ACS_API size_t ACS_ListFireCameraMode_getSize(const ACS_ListFireCameraMode* list);

    /** @brief Get value representing a @ref ACS_FireCamera_Mode "fire camera mode" value at specified index.
     * @relates ACS_ListFireCameraMode
     */
    ACS_API int ACS_ListFireCameraMode_getItem(const ACS_ListFireCameraMode* list, size_t index);

    /** @brief Get size of the list (i.e. temperature range count).
     * @relates ACS_ListFireCameraTemperatureRange
     */
    ACS_API size_t ACS_ListFireCameraTemperatureRange_getSize(ACS_ListFireCameraTemperatureRange* list);

    /** @brief Get @ref ACS_FireCameraTemperatureRange "fire camera temperature range" object at specified index.
     * @relates ACS_ListFireCameraTemperatureRange
     */
    ACS_API ACS_FireCameraTemperatureRange ACS_ListFireCameraTemperatureRange_getItem(ACS_ListFireCameraTemperatureRange* list, size_t index);

    /** @brief Request camera to store an image into the image storage.
     * @relatesalso ACS_RemoteControl
     * @see ACS_Remote_Storage_snapshot_executeSync
     * @see ACS_Remote_Storage_snapshotToLocal_execute
     * @see ACS_Remote_Storage_snapshotToLocalFile_execute
     * @see ACS_Remote_Storage_snapshotToMemory_execute
     */
    ACS_API void ACS_Remote_Storage_snapshot_execute(ACS_RemoteControl* remote, void(*onReceived)(const ACS_StoredImage*, void*), ACS_OnError onError, void* context);
    /** @brief Synchronous/blocking wrapper around @ref ACS_Remote_Storage_snapshot_execute.
     * @relatesalso ACS_RemoteControl
     * @see ACS_Remote_Storage_snapshot_execute
     */
    ACS_API ACS_OWN(ACS_StoredImage*, ACS_StoredImage_free) ACS_Remote_Storage_snapshot_executeSync(ACS_RemoteControl* remote);

    /** @brief EXPERIMENTAL! Request camera to take a snapshot that is @ref ACS_Importer "imported" to the specified local path.
     *
     * This function will not store the image on the camera, making it useful in cases where @ref ACS_Remote_Storage_snapshot_execute() cannot be used (example: camera does not have an image storage).
     * @param destinationFolder Path to the local directory where the file will be stored (example: "/home/user/images/").
     * @relatesalso ACS_RemoteControl
     * @see ACS_Remote_Storage_snapshotToLocal_executeSync
     * @see ACS_Remote_Storage_snapshot_execute
     * @see ACS_Remote_Storage_snapshotToLocalFile_execute
     * @see ACS_Remote_Storage_snapshotToMemory_execute
     */
    ACS_API void ACS_Remote_Storage_snapshotToLocal_execute(ACS_RemoteControl* remote, void(*onReceived)(const ACS_StoredLocalImage*, void*), ACS_OnError onError, void* context, const char* destinationFolder);
    /** @brief Synchronous/blocking wrapper around @ref ACS_Remote_Storage_snapshotToLocal_execute.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_OWN(ACS_StoredLocalImage*, ACS_StoredLocalImage_free) ACS_Remote_Storage_snapshotToLocal_executeSync(ACS_RemoteControl* remote, const char* destinationFolder);

    /** @brief EXPERIMENTAL! Request camera to take a snapshot that is @ref ACS_Importer "imported" to the specified local path.
     *
     * This function will not store the image on the camera, making it useful in cases where @ref ACS_Remote_Storage_snapshot_execute() cannot be used (example: camera does not have an image storage).
     * @param destinationFile Path of the local file to be stored (example: "/home/user/images/image.jpg").
     * @param destinationVisualFile Path of the visual JPEG if thermal and visual images are @ref ACS_Storage_FileFormat_isCombo "separated" (see @ref ACS_Remote_Storage_fileFormat). Should be `NULL` if they're both stored into the same thermal image file.
     * @relatesalso ACS_RemoteControl
     * @see ACS_Remote_Storage_snapshotToLocalFile_executeSync
     * @see ACS_Remote_Storage_snapshot_execute
     * @see ACS_Remote_Storage_snapshotToLocal_execute
     * @see ACS_Remote_Storage_snapshotToMemory_execute
     */
    ACS_API void ACS_Remote_Storage_snapshotToLocalFile_execute(ACS_RemoteControl* remote, void(*onReceived)(const ACS_StoredLocalImage*, void*), ACS_OnError onError, void* context, const char* destinationFile, const char* destinationVisualFile);
    /** @brief Synchronous/blocking wrapper around @ref ACS_Remote_Storage_snapshotToLocalFile_execute.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_OWN(ACS_StoredLocalImage*, ACS_StoredLocalImage_free) ACS_Remote_Storage_snapshotToLocalFile_executeSync(ACS_RemoteControl* remote, const char* destinationFile, const char* destinationVisualFile);

    /** @brief EXPERIMENTAL! Request camera to take a snapshot that is @ref ACS_Importer "imported" into a memory buffer.
     *
     * This function will not store the image on the camera, making it useful in cases where @ref ACS_Remote_Storage_snapshot_execute() cannot be used (example: camera does not have an image storage).
     * @relatesalso ACS_RemoteControl
     * @see ACS_Remote_Storage_snapshotToMemory_executeSync
     * @see ACS_Remote_Storage_snapshot_execute
     * @see ACS_Remote_Storage_snapshotToLocal_execute
     * @see ACS_Remote_Storage_snapshotToLocalFile_execute
     */
    ACS_API void ACS_Remote_Storage_snapshotToMemory_execute(ACS_RemoteControl* remote, void(*onReceived)(const ACS_StoredMemoryImage*, void*), ACS_OnError onError, void* context);
    /** @brief Synchronous/blocking wrapper around @ref ACS_Remote_Storage_snapshotToMemory_execute.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_OWN(ACS_StoredMemoryImage*, ACS_StoredMemoryImage_free) ACS_Remote_Storage_snapshotToMemory_executeSync(ACS_RemoteControl* remote);

    /** @brief The @ref ACS_Storage_FileFormat "image file format" used for @ref ACS_Remote_Storage_snapshotTo_execute "snapshots".
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_Int* ACS_Remote_Storage_fileFormat(ACS_RemoteControl* remote);

    /** @brief Triggers a single autofocus / AF event.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API void ACS_Remote_Focus_autofocus_execute(ACS_RemoteControl* remote, ACS_OnCompletion onCompletion, ACS_OnError onError, void* context);
    /** @brief Synchronous/blocking wrapper around @ref ACS_Remote_Focus_autofocus_execute.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API void ACS_Remote_Focus_autofocus_executeSync(ACS_RemoteControl* remote);


    /** @brief Distance from camera to the focused object.
     * @remarks Fix unit in meters.
     *          The range of valid values is camera-dependent. Please refer to camera documentation for more information.
     * @note    Distance in meters compared to distance unit in @ref ACS_ThermalParameters_objectDistance.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_Double* ACS_Remote_ThermalParameters_objectDistance(ACS_RemoteControl* remote);

    /** @brief Emissivity for the focused object.
     * @remarks Emissivity is a value between 0.001 and 1.0 that specifies how much radiation an object emits
     *          compared to the radiation of a theoretical reference object of the same temperature (called a 'blackbody').
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_Double* ACS_Remote_ThermalParameters_objectEmissivity(ACS_RemoteControl* remote);

    /** @brief Reflected temperature for the focused object.
     * @remarks This parameter is used to compensate for the radiation reflected in the object.
     *          If the emissivity is low and the object temperature relatively far from that of
     *          the reflected it will be important to set and compensate for the reflected apparent temperature correctly.
     * @note    Temperature in temperature unit (see ACS_TemperatureUnit).
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_ThermalValue* ACS_Remote_ThermalParameters_objectReflectedTemperature(ACS_RemoteControl* remote);

    /** @brief The relative humidity.
     * @remarks Relative humidity is a term used to describe the amount of water vapor that exists
     *          in a gaseous mixture of air and water.
     *          E.g. 0.30 implies a relative humidity of 30%.
     * @note    Fraction range compared to percentage range in @ref ACS_ThermalParameters_relativeHumidity.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_Double* ACS_Remote_ThermalParameters_relativeHumidity(ACS_RemoteControl* remote);

    /** @brief The atmospheric temperature.
     * @remarks The atmosphere is the medium between the object being measured and the camera, normally air.
     * @note    Temperature in temperature unit (see ACS_TemperatureUnit).
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_ThermalValue* ACS_Remote_ThermalParameters_atmosphericTemperature(ACS_RemoteControl* remote);

    /** @brief The atmospheric transmission.
     * @remarks Transmission coefficient of the atmosphere between the scene and the camera, normally air.
     *          If this value is set to 0, the atmospheric transmission will calculated automatically.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_Double* ACS_Remote_ThermalParameters_atmosphericTransmission(ACS_RemoteControl* remote);

    /** @brief The external optics temperature.
     * @remarks External optics temperature is a temperature of an external accessory attached to the Thermal camera
     *          Used to compensate for optical accessory, such as a heat shield or a macro lenses.
     *          Specify the temperature of, e.g., an external lens or heat shield.
     *          External optics will absorb some of the radiation.
     * @note    Temperature in temperature unit (see ACS_TemperatureUnit).
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_ThermalValue* ACS_Remote_ThermalParameters_externalOpticsTemperature(ACS_RemoteControl* remote);

    /** @brief The external optics transmission.
     * @remarks Used to compensate for optical accessory, such as a heat shield or a macro lenses.
     *          Specify the transmission of, e.g., an external lens or heat shield.
     *          External optics will absorb some of the radiation.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_Double* ACS_Remote_ThermalParameters_externalOpticsTransmission(ACS_RemoteControl* remote);

    /**@brief Type definition provided for clarity*/
    typedef unsigned int ACS_Focus_Speed;

    /** @brief The minimum speed at which focus distance can be changed*/
    #define ACS_FOCUS_SPEED_SLOW ((ACS_Focus_Speed) 1)

    /** @brief Faster than the slow setting */
    #define ACS_FOCUS_SPEED_FAST ((ACS_Focus_Speed) 10)

    /** @brief Faster than the fast setting */
    #define ACS_FOCUS_SPEED_FASTER ((ACS_Focus_Speed) 50)

    /** @brief The maximum speed at which focus distance can be changed*/
    #define ACS_FOCUS_SPEED_MAX ((ACS_Focus_Speed) 100)

    /** @brief Starts an increase of the distance between the camera and the focus plane.
     * @param speed A camera-dependent value which controls how quickly the speed at which change will occur.
     * @note Calling this function will override any ongoing increase or decrease of the focus distance.
     * @note Valid values for speed are 1..100. However, using (pre-)defined constants is recommended.
     * @note Calling this function with @ref ACS_FOCUS_SPEED_SLOW is equivalent to the "focus far" operation in previous SDKs.
     * @see ACS_Remote_Focus_distanceStartDecrease_execute
     * @see ACS_Remote_Focus_distanceStop_execute
     * @relatesalso ACS_RemoteControl
     */
    ACS_API void ACS_Remote_Focus_distanceStartIncrease_execute(ACS_RemoteControl* remote, ACS_OnCompletion onCompletion, ACS_OnError onError, void* context, ACS_Focus_Speed speed);

    /** @brief Synchronous start of an increase of the distance between the camera and the focus plane.
     * @param speed A camera-dependent value which controls how quickly the speed at which change will occur.
     * @note Valid values for speed are 1..100. However, using (pre-)defined constants is recommended.
     * @note Calling this function will override any ongoing increase or decrease of the focus distance.
     * @note Calling this function with @ref ACS_FOCUS_SPEED_SLOW is equivalent to the "focus far" operation in previous SDKs.
     * @see ACS_Remote_Focus_distanceStartDecrease_executeSync
     * @see ACS_Remote_Focus_distanceStop_executeSync
     * @relatesalso ACS_RemoteControl
     */
    ACS_API void ACS_Remote_Focus_distanceStartIncrease_executeSync(ACS_RemoteControl* remote, ACS_Focus_Speed speed);

    /** @brief Starts a decrease of the distance between the camera and the focus plane.
     * @param speed A camera-dependent value which controls how quickly the speed at which change will occur.
     * @note Valid values for speed are 1..100. However, using (pre-)defined constants is recommended.
     * @note Calling this function will override any ongoing increase or decrease of the focus distance.
     * @note Calling this function with @ref ACS_FOCUS_SPEED_SLOW is equivalent to the "focus near" operation in previous SDKs.
     * @see ACS_Remote_Focus_distanceStartIncrease_execute
     * @see ACS_Remote_Focus_distanceStop_execute
     * @relatesalso ACS_RemoteControl
     */
    ACS_API void ACS_Remote_Focus_distanceStartDecrease_execute(ACS_RemoteControl* remote, ACS_OnCompletion onCompletion, ACS_OnError onError, void* context, ACS_Focus_Speed speed);

    /** @brief Synchronous start of a decrease of the distance between the camera and the focus plane.
     * @param speed A camera-dependent value which controls how quickly the speed at which change will occur.
     * @note Valid values for speed are 1..100. However, using (pre-)defined constants is recommended.
     * @note Calling this function will override any ongoing increase or decrease of the focus distance.
     * @note Calling this function using @ref ACS_FOCUS_SPEED_SLOW is equivalent to the "focus near" operation in previous SDKs.
     * @see ACS_Remote_Focus_distanceStartIncrease_executeSync
     * @see ACS_Remote_Focus_distanceStop_executeSync
     * @relatesalso ACS_RemoteControl
     */
    ACS_API void ACS_Remote_Focus_distanceStartDecrease_executeSync(ACS_RemoteControl* remote, ACS_Focus_Speed speed);

    /** @brief Stops any ongoing increase or decrease of focus distance.
     * @note If there is no ongoing increase or decrease of focus distance, calling this function will have no tangible effect.
     * @see ACS_Remote_Focus_distanceStartIncrease_execute
     * @see ACS_Remote_Focus_distanceStartDecrease_execute
     * @relatesalso ACS_RemoteControl
     */
    ACS_API void ACS_Remote_Focus_distanceStop_execute(ACS_RemoteControl* remote, ACS_OnCompletion onCompletion, ACS_OnError onError, void* context);

    /** @brief Synchronous stop of any ongoing increase or decrease of focus distance.
     * @note If there is no ongoing increase or decrease of focus distance, calling this function will have no tangible effect.
     * @see ACS_Remote_Focus_distanceStartIncrease_executeSync
     * @see ACS_Remote_Focus_distanceStartDecrease_executeSync
     * @relatesalso ACS_RemoteControl
     */
    ACS_API void ACS_Remote_Focus_distanceStop_executeSync(ACS_RemoteControl* remote);

    /** @brief gets/sets the cameras focus distance.
     * @note Setting this property does not yield instantaneous results as it affects moving parts within the camera.
     * @note The range of valid values is camera-dependent. Please refer to camera documentation for more information.
     * @note Setting this property directly is considered advanced. Interactive applications should use Start/Stop functions wherever possible.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_Double* ACS_Remote_Focus_distance(ACS_RemoteControl* remote);

    /** @brief Defines the @ref ACS_Focus_CalculationMethod "focus calculation method". I.e. based on maximizing the image contrast or based on laser distance measurement.
     *
     * See @ref ACS_Focus_CalculationMethod for possible values.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_Int* ACS_Remote_Focus_calculationMethod(ACS_RemoteControl* remote);

    /** @brief gets/sets the cameras focus position.
     * @note The focus position is a camera-dependent distance to the focus plane.
     * @note Setting this property does not yield instantaneous results as it affects moving parts within the camera.
     * @note The range of valid values is camera-dependent. Please refer to camera documentation for more information.
     * @warning Setting this property directly is considered advanced. Interactive applications should use @ref ACS_Remote_Focus_distanceStartIncrease_execute "Start" (or @ref ACS_Remote_Focus_distanceStartDecrease_execute "Start")/@ref ACS_Remote_Focus_distanceStopIncrease_execute "Stop" functions wherever possible.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_Int* ACS_Remote_Focus_position(ACS_RemoteControl* remote);

    /** @brief Process a one point NUC (Non-uniformity correction) with a black body method.
     * @remarks Also known as FFC/Flat field correction
     */
    ACS_API void ACS_Remote_Calibration_nuc_execute(ACS_RemoteControl* remote, ACS_OnCompletion onCompletion, ACS_OnError onError, void* context);

    /** @brief Process a one point NUC (Non-uniformity correction) with a black body method.
     * @remarks Also known as FFC/Flat field correction
     */
    ACS_API ACS_Error ACS_Remote_Calibration_nuc_executeSync(ACS_RemoteControl* remote);
    
    /** @brief Get NUC (Non-Uniform Calibration) state.
     *
     * @note Certain camera models may report `ACS_NucState_unknown` until streaming is started.
     * @returns A property with a @ref ACS_NucState_ value
     */
    ACS_API const ACS_Property_Int* ACS_Remote_Calibration_nucState(ACS_RemoteControl* remote);

    /** @brief Get shutter state.
     *
     * @note Certain camera models may report `ACS_NucState_unknown` until streaming is started.
     * @ref ACS_ShutterState_ 
     */
    ACS_API const ACS_Property_Int* ACS_Remote_Calibration_shutterState(ACS_RemoteControl* remote);
    
    /** @brief Define NUC (Non-Uniform Calibration) maximum interval between each automatic NUCs in seconds.
     *  For FLIR One setting nucInterval to 0 disables automatic nuc, any other value enables automatic nuc
    */
    ACS_API ACS_Property_Int* ACS_Remote_Calibration_nucIntervalSeconds(ACS_RemoteControl* remote);

    /** @brief get the number of available temperature ranges.
     */
    ACS_API const ACS_Property_ListTemperatureRange* ACS_Remote_TemperatureRange_ranges(ACS_RemoteControl* remote);
    
    /** @brief select a temperature range from index.
     *
     * @remarks Setting this value results in an asynchronous operation. As such, immediate-term subsequent reads may return incorrect value. To ensure that the value has been set correctly, subscribe to the property instead.
     * @remarks Certain camera models, such as FLIR ONE, streaming should be started prior to setting this value, in order to ensure that the value is correctly represented in ACS_Image_CameraInformation
     */
    ACS_API ACS_Property_Int* ACS_Remote_TemperatureRange_selectedIndex(ACS_RemoteControl* remote);

    /** @brief Auto adjust mode. Defines the adjust mode of the temperature scale on the live screen on the camera.
     * Auto mode (true value): In this mode, the camera is continuously auto-adjusted for the best image brightness and contrast.
     * Manual mode (false value): This mode allows manual adjustments of the temperature span and the temperature level.
     */
    ACS_API ACS_Property_Bool* ACS_Remote_Scale_autoAdjust(ACS_RemoteControl* remote);

    /** @brief Min scale/span temperature value reported by a remote camera. 
     * @note Value can be modified in manual scale mode, but you need to make sure auto adjust mode is set to manual prior to setting this property in order for this setting to take effect.
     */
    ACS_API ACS_Property_ThermalValue* ACS_Remote_Scale_min(ACS_RemoteControl* remote);

    /** @brief Max scale/span temperature value reported by a remote camera. 
     * @note Value can be modified in manual scale mode, but you need to make sure auto adjust mode is set to manual prior to setting this property in order for this setting to take effect.
     */
    ACS_API ACS_Property_ThermalValue* ACS_Remote_Scale_max(ACS_RemoteControl* remote);

    /** @brief Scale/span temperature range reported by a remote camera. 
     * @note Range can be modified in manual scale mode, but you need to make sure auto adjust mode is set to manual prior to setting this property in order for this setting to take effect.
     */
    ACS_API ACS_Property_TemperatureRange* ACS_Remote_Scale_range(ACS_RemoteControl* remote);

    /** @brief active mode. Enables/disables the scale active mode
     */
    ACS_API ACS_Property_Bool* ACS_Remote_Scale_active(ACS_RemoteControl* remote);

    /** @brief Control whether camera should hide it's whole GUI/overlay - i.e. measurement tools.
     */
    ACS_API ACS_Property_Bool* ACS_Remote_Overlay_hide(ACS_RemoteControl* remote);

    /** @brief Defines the active channel on displaying image on live view on the camera.
     * @note This property may change dynamically after ::displayMode property is changed. Emulated cameras are NOT affected.
     * Writing this value should be done with caution, because in order to camera physically change the mode it is also required to apply proper values to ::displayMode and ::fusionSpanLevel.
     * This is highly recommended to use ::displayMode only, which automatically makes sure both ::fusionSpanLevel and this property are set accordingly.
     * Avoid setting this property while streaming.
     * @ref ACS_ChannelType_
     */
    ACS_API ACS_Property_Int* ACS_Remote_Fusion_activeChannel(ACS_RemoteControl* remote);

    /** @brief Defines coordinates of Picture-in-Picture window.
     */
    ACS_API ACS_Property_Rectangle* ACS_Remote_Fusion_pipWindow(ACS_RemoteControl* remote);

    /** @brief Defines if camera supports MSX mode, if it doesn't support fusion modes.
     */
    ACS_API const ACS_Property_Bool* ACS_Remote_Fusion_msxSupported(const ACS_RemoteControl* remote);

    /** @brief Defines if camera always uses fusion mode(s) to stream image.
     */
    ACS_API const ACS_Property_Bool* ACS_Remote_Fusion_fusionAlwaysOn(const ACS_RemoteControl* remote);

    /** @brief Defines available display modes on the camera.
     */
    ACS_API const ACS_Property_ListDisplayMode* ACS_Remote_Fusion_validModes(const ACS_RemoteControl* remote);

    /** @brief Defines selected display mode on the camera's live view.
     * @ref ACS_DisplayMode_ 
     */
    ACS_API ACS_Property_Int* ACS_Remote_Fusion_displayMode(ACS_RemoteControl* remote);

    /** @brief Defines how fusion span level is displayed.
     * @note This property may change dynamically after ::displayMode property is changed. Emulated cameras are NOT affected.
     * Writing this value should be done with caution, because in order to camera physically change the mode it is also required to apply proper values to ::displayMode and ::activeChannel.
     * This is highly recommended to use ::displayMode only, which automatically makes sure both ::activeChannel and this property are set accordingly.
     * @ref ACS_FusionSpanLevel_ 
     */
    ACS_API ACS_Property_Int* ACS_Remote_Fusion_fusionSpanLevel(ACS_RemoteControl* remote);

    /** @brief Defines the temperature range to display in thermal fusion mode.
     */  
    ACS_API ACS_Property_TemperatureRange* ACS_Remote_Fusion_fusionTemperatureRange(ACS_RemoteControl* remote);

    /** @brief The fusion distance in meters. This property affects the @ref atlas::image::Transformation "fusion panning". */
    ACS_API ACS_Property_Double* ACS_Remote_Fusion_distance(ACS_RemoteControl* remote);

    /** @brief Defines currently used camera's palette.
     */
    ACS_API ACS_Property_RemotePalette* ACS_Remote_Palette_currentPalette(ACS_RemoteControl* remote);

    /** @brief Defines list of palettes available on the particular camera.
     */
    ACS_API const ACS_Property_ListRemotePalette* ACS_Remote_Palette_availablePalettes(const ACS_RemoteControl* remote);


    /** @brief Adds and activates a new spot measurement tool to the camera's measurements at the given position.
     *
     * @note There are limitation in the number of allowed measurements depending on the camera type.
     */
    ACS_API void ACS_Remote_Measurements_addSpot_execute(ACS_RemoteControl* remote, ACS_Point point, ACS_OnReceivedRemoteMeasurementSpot onReceived, ACS_OnError onError, void* context);

    /** @brief Adds and activates a new spot measurement tool to the camera's measurements at the given position.
     *
     * @note There are limitation in the number of allowed measurements depending on the camera type.
     */
    ACS_API ACS_RemoteMeasurementSpot ACS_Remote_Measurements_addSpot_executeSync(ACS_RemoteControl* remote, ACS_Point point);

    /** @brief Get a list of all active spot measurement tools on the camera.
     */
    ACS_API const ACS_Property_ListRemoteMeasurementSpot* ACS_Remote_Measurements_spots(const ACS_RemoteControl* remote);

    /** @brief Adds and activates a new rectangle measurement tool to the camera's measurements described by the given rectangular area.
     *
     * @note There are limitation in the number of allowed measurements depending on the camera type.
     */
    ACS_API void ACS_Remote_Measurements_addRectangle_execute(ACS_RemoteControl* remote, ACS_Rectangle rectangle, ACS_OnReceivedRemoteMeasurementRectangle onReceived, ACS_OnError onError, void* context);

    /** @brief Adds and activates a new rectangle measurement tool to the camera's measurements described by the given rectangular area.
     *
     * @note There are limitation in the number of allowed measurements depending on the camera type.
     */
    ACS_API ACS_RemoteMeasurementRectangle ACS_Remote_Measurements_addRectangle_executeSync(ACS_RemoteControl* remote, ACS_Rectangle rectangle);

    /** @brief Get a list of all active rectangle measurement tools on the camera.
     */
    ACS_API const ACS_Property_ListRemoteMeasurementRectangle* ACS_Remote_Measurements_rectangles(const ACS_RemoteControl* remote);

    /** @brief Adds and activates a new circle measurement tool to the camera's measurements described by the given circular area.
     *
     * @note There are limitation in the number of allowed measurements depending on the camera type.
     */
    ACS_API void ACS_Remote_Measurements_addCircle_execute(ACS_RemoteControl* remote, ACS_Circle circle, ACS_OnReceivedRemoteMeasurementCircle onReceived, ACS_OnError onError, void* context);

    /** @brief Adds and activates a new circle measurement tool to the camera's measurements described by the given circular area.
     *
     * @note There are limitation in the number of allowed measurements depending on the camera type.
     */
    ACS_API ACS_RemoteMeasurementCircle ACS_Remote_Measurements_addCircle_executeSync(ACS_RemoteControl* remote, ACS_Circle circle);

    /** @brief Get a list of all active circle measurement tools on the camera.
     */
    ACS_API const ACS_Property_ListRemoteMeasurementCircle* ACS_Remote_Measurements_circles(const ACS_RemoteControl* remote);

    /** @brief Adds and activates a new line measurement tool to the camera's measurements described by the given parameters.
     *
     * @note There are limitation in the number of allowed measurements depending on the camera type.
     */
    ACS_API void ACS_Remote_Measurements_addLine_execute(ACS_RemoteControl* remote, ACS_Line line, ACS_OnReceivedRemoteMeasurementLine onReceived, ACS_OnError onError, void* context);

    /** @brief Adds and activates a new line measurement tool to the camera's measurements described by the given parameters.
     *
     * @note There are limitation in the number of allowed measurements depending on the camera type.
     */
    ACS_API ACS_RemoteMeasurementLine ACS_Remote_Measurements_addLine_executeSync(ACS_RemoteControl* remote, ACS_Line line);

    /** @brief Get a list of all active line measurement tools on the camera.
     */
    ACS_API const ACS_Property_ListRemoteMeasurementLine* ACS_Remote_Measurements_lines(const ACS_RemoteControl* remote);

    /** @brief Remove/deactivate a particular measurement tool from the camera's measurements.
     */
    ACS_API void ACS_Remote_Measurements_removeSpot_execute(ACS_RemoteControl* remote, const ACS_RemoteMeasurementSpot* spot, ACS_OnCompletion onCompletion, ACS_OnError onError, void* context);

    /** @brief Remove/deactivate a particular measurement tool from the camera's measurements.
     */
    ACS_API ACS_Error ACS_Remote_Measurements_removeSpot_executeSync(ACS_RemoteControl* remote, const ACS_RemoteMeasurementSpot* spot);

    /** @brief Remove/deactivate a particular measurement tool from the camera's measurements.
     */
    ACS_API void ACS_Remote_Measurements_removeMarker_execute(ACS_RemoteControl* remote, const ACS_RemoteMeasurementMarker* marker, ACS_OnCompletion onCompletion, ACS_OnError onError, void* context);

    /** @brief Remove/deactivate a particular measurement tool from the camera's measurements.
     */
    ACS_API ACS_Error ACS_Remote_Measurements_removeMarker_executeSync(ACS_RemoteControl* remote, const ACS_RemoteMeasurementMarker* marker);

    /** @brief Remove/deactivate all measurement tools from the camera's measurements.
     */
    ACS_API void ACS_Remote_Measurements_removeAll_execute(ACS_RemoteControl* remote, ACS_OnCompletion onCompletion, ACS_OnError onError, void* context);

    /** @brief Remove/deactivate all measurement tools from the camera's measurements.
     */
    ACS_API ACS_Error ACS_Remote_Measurements_removeAll_executeSync(ACS_RemoteControl* remote);

    /** @brief Get a position of the given active spot measurement tool.
     *
     * @note The shape ID must first have been queried, before accessing its properties.
     * @remarks Error if shapeHandle does not refer to a correct/known/queried shape.
     */
    ACS_API ACS_Property_Point* ACS_Remote_Measurements_spotPosition(ACS_RemoteMeasurementSpot shapeHandle, ACS_RemoteControl* remote);

    /** @brief Defines a temperature value of the given active spot measurement tool.
     *
     * @note The shape ID must first have been queried, before accessing its properties.
     * @remarks Error if shapeHandle does not refer to a correct/known/queried shape.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API const ACS_Property_ThermalValue* ACS_Remote_Measurements_spotValue(const ACS_RemoteControl* remote, ACS_RemoteMeasurementSpot shapeHandle);

    /** @brief Get an area of the given active rectangle measurement tool described as a rectangular shape.
     *
     * @note The shape ID must first have been queried, before accessing its properties.
     * @remarks Error if shapeHandle does not refer to a correct/known/queried shape.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_Rectangle* ACS_Remote_Measurements_rectangleShape(ACS_RemoteMeasurementRectangle shapeHandle, ACS_RemoteControl* remote);

    /** @brief Get an area of the given active circle measurement tool.
     *
     * @note The shape ID must first have been queried, before accessing its properties.
     * @remarks Error if shapeHandle does not refer to a correct/known/queried shape.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_Circle* ACS_Remote_Measurements_circleShape(ACS_RemoteMeasurementCircle shapeHandle, ACS_RemoteControl* remote);

    /** @brief Get an area of the given active line measurement tool.
     *
     * @note The shape ID must first have been queried, before accessing its properties.
     * @remarks Error if shapeHandle does not refer to a correct/known/queried shape.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_Line* ACS_Remote_Measurements_lineShape(ACS_RemoteMeasurementLine shapeHandle, ACS_RemoteControl* remote);

    /** @brief Get a position of the given marker inside the given active measurement tool.
     *
     * @note The shape ID must first have been queried, before accessing its properties.
     * @remarks Error if shapeHandle does not refer to a correct/known/queried shape.
     * @param markerType @ref ACS_RemoteMarkerType_
     * @relatesalso ACS_RemoteControl
     */
    ACS_API const ACS_Property_Point* ACS_Remote_Measurements_position(const ACS_RemoteControl* remote, const ACS_RemoteMeasurementMarker* shapeHandle, int markerType);

    /** @brief Get a position of the hottest spot inside the given active measurement tool.
     *
     * @note The shape ID must first have been queried, before accessing its properties.
     * @see Measurements::position(shapeHandle, MarkerType::hotSpot)
     * @remarks Error if shapeHandle does not refer to a correct/known/queried shape.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API const ACS_Property_Point* ACS_Remote_Measurements_hotSpotPosition(const ACS_RemoteControl* remote, const ACS_RemoteMeasurementMarker* shapeHandle);

    /** @brief Get a position of the coldest spot inside the given active measurement tool.
     *
     * @note The shape ID must first have been queried, before accessing its properties.
     * @see Measurements::position(shapeHandle, MarkerType::coldSpot)
     * @remarks Error if shapeHandle does not refer to a correct/known/queried shape.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API const ACS_Property_Point* ACS_Remote_Measurements_coldSpotPosition(const ACS_RemoteControl* remote, const ACS_RemoteMeasurementMarker* shapeHandle);

    /** @brief Set the calc mask on the measurement area, affecting calculations. The measurement calculations will be shown on the camera image.
     * @remarks Error if shapeHandle does not refer to a correct/known/queried shape.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_MeasurementCalcMaskFlags* ACS_Remote_Measurements_calcMask(ACS_RemoteControl* remote, const ACS_RemoteMeasurementMarker* shapeHandle);

    /** @brief Defines a temperature value of the given value type inside the given active measurement tool.
     *
     * @note The shape ID must first have been queried, before accessing it's properties.
     * @remarks Error if shapeHandle does not refer to a correct/known/queried shape.
     * @param valueType @ref ACS_RemoteMeasurementValueType_
     * @relatesalso ACS_RemoteControl
     */
    ACS_API const ACS_Property_ThermalValue* ACS_Remote_Measurements_value(const ACS_RemoteControl* remote, const ACS_RemoteMeasurementMarker* shapeHandle, int valueType);

    /** @brief Defines a temperature value of the hottest spot inside the given active measurement tool.
     *
     * @note The shape ID must first have been queried, before accessing it's properties.
     * @see ACS_Remote_Measurements_value (use ACS_RemoteMeasurementValueType_max for hottest value).
     * @remarks Error if shapeHandle does not refer to a correct/known/queried shape.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API const ACS_Property_ThermalValue* ACS_Remote_Measurements_hotSpotValue(const ACS_RemoteControl* remote, ACS_MeasurementMarker* shapeHandle);

    /** @brief Defines a temperature value of the coldest spot inside the given active measurement tool.
     *
     * @note The shape ID must first have been queried, before accessing it's properties.
     * @see ACS_Remote_Measurements_value (use ACS_RemoteMeasurementValueType_min for coldest value).
     * @remarks Error if shapeHandle does not refer to a correct/known/queried shape.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API const ACS_Property_ThermalValue* ACS_Remote_Measurements_coldSpotValue(const ACS_RemoteControl* remote, ACS_MeasurementMarker* shapeHandle);
    

    /** @brief Defines an average temperature value of the given active measurement tool.
     *
     * @note The shape ID must first have been queried, before accessing it's properties.
     * @see ACS_Remote_Measurements_value (use ACS_RemoteMeasurementValueType_average for average value).
     * @remarks Error if shapeHandle does not refer to a correct/known/queried shape.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API const ACS_Property_ThermalValue* ACS_Remote_Measurements_averageValue(const ACS_RemoteControl* remote, ACS_MeasurementMarker* shapeHandle); 


    /** @brief Defines if the hot and cold spot markers are active and visible inside the given active measurement tool.
     *
     * @note The shape ID must first have been queried, before accessing it's properties.
     * @remarks Error if shapeHandle does not refer to a correct/known/queried shape.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Property_Bool* ACS_Remote_Measurements_markersActive(ACS_RemoteControl* remote, const ACS_MeasurementMarker* shapeHandle);

    /** @brief Get update status.
     * @remarks If failed, use @ref ACS_Remote_FirmwareUpdate_cancelUpdate_execute to cancel.
     * @returns @ref ACS_FirmwareUpdate_Status_
     * @relatesalso ACS_RemoteControl
     */
    ACS_API const ACS_Property_Int* ACS_Remote_FirmwareUpdate_updateStatus(ACS_RemoteControl* remote);
    
    /** @brief Command for starting a firmware update
     * Operation is asynchronous even when executed synchronously.
     * Use @ref ACS_Remote_FirmwareUpdate_updateStatus to check progress.
     * 
     * If there is an error, @ref updateStatus will be set to Status::failure, and the firmware update should be canceled using @ref cancelUpdate.
     * @param paths List of absolute file paths
     * @note The camera may need to be manually reset by the user if an update fails
     * @warning This operation is not thread safe and must be synchronized with other operations on the same Camera object.
     * @warning The @ref CameraEngine object (see @ref Camera) keeps track of the progress while disconnected, and must therefore not be destroyed before the update has completed.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API void ACS_Remote_FirmwareUpdate_updateFirmware_execute(ACS_RemoteControl* remote, const char** paths, size_t pathCount, ACS_OnCompletion onCompletion, ACS_OnError onError, void* context);

    /** @brief Command for starting a firmware update
     * Operation is asynchronous even when executed synchronously.
     * Use @ref ACS_Remote_FirmwareUpdate_updateStatus to check progress.
     * 
     * If there is an error, @ref updateStatus will be set to Status::failure, and the firmware update should be canceled using @ref cancelUpdate.
     * @param paths List of absolute file paths
     * @note The camera may need to be manually reset by the user if an update fails
     * @warning This operation is not thread safe and must be synchronized with other operations on the same Camera object.
     * @warning The @ref CameraEngine object (see @ref Camera) keeps track of the progress while disconnected, and must therefore not be destroyed before the update has completed.
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Error ACS_Remote_FirmwareUpdate_updateFirmware_executeSync(ACS_RemoteControl* remote, const char** paths, size_t pathCount);
    
    /**
     * @brief Command for canceling a firmware update
     *
     * This can be used when the camera is reporting errors during update.
     * This can also be called if the camera is physically disconnected during the update.
     * If the camera is defective or disconnected  this command may block for a timeout.
     * @note The camera may need to be manually reset by the user if an update fails
     * @warning This operation is not thread safe and must be synchronized with other operations on the same Camera object.
     * @warning Asynchronous operations on this Command might actually block for some cameras (like FLIR ONE)
     * @relatesalso ACS_RemoteControl
     */
    ACS_API void ACS_Remote_FirmwareUpdate_cancelUpdate_execute(ACS_RemoteControl* remote, ACS_OnCompletion onCompletion, ACS_OnError onError, void* context);

    /**
     * @brief Command for canceling a firmware update
     *
     * This can be used when the camera is reporting errors during update.
     * This can also be called if the camera is physically disconnected during the update.
     * If the camera is defective or disconnected  this command may block for a timeout.
     * @note The camera may need to be manually reset by the user if an update fails
     * @warning This operation is not thread safe and must be synchronized with other operations on the same Camera object.
     * @warning Asynchronous operations on this Command might actually block for some cameras (like FLIR ONE)
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Error ACS_Remote_FirmwareUpdate_cancelUpdate_executeSync(ACS_RemoteControl* remote);
    
    /**
     * @brief Command for rebooting a device to perform a firmware update or to switch it back to normal mode.
     *
     * This call returns immediately, the actual reboot happens in a background.
     * @param mode a mode to reboot device to
     * @note The camera may need to be manually reset by the user if an update fails or camera stays in upgrade mode
     * @note The camera needs to be in @ref ACS_OperatingMode_upgrade mode in order to perform the firmware upgrade using @ref updateFirmware
     * @returns @ref ACS_FirmwareUpdate_OperatingMode_
     * @relatesalso ACS_RemoteControl
     */
    ACS_API void ACS_Remote_FirmwareUpdate_rebootDeviceIntoMode_execute(ACS_RemoteControl* remote, int mode, ACS_OnCompletion onCompletion, ACS_OnError onError, void* context);

    /**
     * @brief Command for rebooting a device to perform a firmware update or to switch it back to normal mode.
     *
     * This call returns immediately, the actual reboot happens in a background.
     * @param mode a mode to reboot device to
     * @note The camera may need to be manually reset by the user if an update fails or camera stays in upgrade mode
     * @note The camera needs to be in @ref ACS_OperatingMode_upgrade mode in order to perform the firmware upgrade using @ref updateFirmware
     * @returns @ref ACS_FirmwareUpdate_OperatingMode_
     * @relatesalso ACS_RemoteControl
     */
    ACS_API ACS_Error ACS_Remote_FirmwareUpdate_rebootDeviceIntoMode_executeSync(ACS_RemoteControl* remote, int mode);


    /** @brief Retrieves the number of elements in the list.
     * @returns Number of elements in the list.
     * @relatesalso ACS_ListDisplayMode
     */
    ACS_API size_t ACS_ListDisplayMode_size(const ACS_ListDisplayMode* list);

    /** @brief Retrieves item at specified index if it exists.
     * @remarks Strongly recommended to check for errors after call.
     * @param index Index of object to retrieve.
     * @returns A value corresponding to a @ref ACS_DisplayMode_ if the index is valid, otherwise unspecified.
     * @relatesalso ACS_ListDisplayMode
     */
    ACS_API int ACS_ListDisplayMode_item(ACS_ListDisplayMode* list, size_t index);

    /** @brief Retrieves the number of elements in the list.
     * @returns Number of elements in the list.
     * @relatesalso ACS_ListRemotePalette
     */
    ACS_API size_t ACS_ListRemotePalette_size(const ACS_ListRemotePalette* list);

    /** @brief Retrieves item at specified index if it exists.
     * @param index Index of object to retrieve.
     * @returns A remote palette if the index is valid, otherwise NULL.
     * @relatesalso ACS_ListRemotePalette
     */
    ACS_API ACS_RemotePalette* ACS_ListRemotePalette_item(ACS_ListRemotePalette* list, size_t index);

    /** @brief Get the name for this palette.
     * @returns A const char pointer to Palette Name.
     *  @relatesalso ACS_RemotePalette
     */
    ACS_API const char* ACS_RemotePalette_getName(ACS_RemotePalette* palette);

    /**@brief Tests if a flag is set.
     * @param flag @ref ACS_MeasurementCalcMask_
     * @remarks Testing one flag at a time is assumed, the behavior of testing multiple flags simultaneously is not guaranteed by this API.
     * @returns true if set, otherwise false.
     * @relatesalso ACS_MeasurementCalcMaskFlags
     */
    ACS_API bool ACS_MeasurementCalcMaskFlags_isSet(ACS_MeasurementCalcMaskFlags* mask, int flag);

    /**@brief Sets a flag.
     * @param flag @ref ACS_MeasurementCalcMask_
     * @relates Multiple flags may be set at the same time.
     * @relatesalso ACS_MeasurementCalcMaskFlags
     */
    ACS_API void ACS_MeasurementCalcMaskFlags_set(ACS_MeasurementCalcMaskFlags* mask, int flag);

    /**@brief Resets a flag.
     * @param flag @ref ACS_MeasurementCalcMask_
     * @relates Multiple flags may be set at the same time.
     * @relatesalso ACS_MeasurementCalcMaskFlags
     */
    ACS_API void ACS_MeasurementCalcMaskFlags_reset(ACS_MeasurementCalcMaskFlags* mask, int flag);

    /** @brief Retrieves the number of elements in the list.
     * @returns Number of elements in the list.
     * @relatesalso ACS_ListRemoteMeasurementSpot
     */
    ACS_API size_t ACS_ListRemoteMeasurementSpot_size(ACS_ListRemoteMeasurementSpot* list);

    /** @brief Retrieves item at specified index if it exists.
     * @param index Index of object to retrieve.
     * @returns Object if it exists, otherwise NULL.
     * @relatesalso ACS_ListRemoteMeasurementSpot
     */
    ACS_API ACS_RemoteMeasurementSpot ACS_ListRemoteMeasurementSpot_item(ACS_ListRemoteMeasurementSpot* list, size_t index);

    /** @brief Retrieves the number of elements in the list.
     * @returns Number of elements in the list.
     * @relatesalso ACS_ListRemoteMeasurementRectangle
     */
    ACS_API size_t ACS_ListRemoteMeasurementRectangle_size(ACS_ListRemoteMeasurementRectangle* list);

    /** @brief Retrieves item at specified index if it exists.
     * @param index Index of object to retrieve.
     * @returns Object if it exists, otherwise NULL.
     * @relatesalso ACS_ListRemoteMeasurementRectangle
     */
    ACS_API ACS_RemoteMeasurementRectangle ACS_ListRemoteMeasurementRectangle_item(ACS_ListRemoteMeasurementRectangle* list, size_t index);

    /** @brief Retrieves the number of elements in the list.
     * @returns Number of elements in the list.
     * @relatesalso ACS_ListRemoteMeasurementCircle
     */
    ACS_API size_t ACS_ListRemoteMeasurementCircle_size(ACS_ListRemoteMeasurementCircle* list);

    /** @brief Retrieves item at specified index if it exists.
     * @param index Index of object to retrieve.
     * @returns Object if it exists, otherwise NULL.
     * @relatesalso ACS_ListRemoteMeasurementCircle
     */
    ACS_API ACS_RemoteMeasurementCircle ACS_ListRemoteMeasurementCircle_item(ACS_ListRemoteMeasurementCircle* list, size_t index);

    /** @brief Retrieves the number of elements in the list.
     * @returns Number of elements in the list.
     * @relatesalso ACS_ListRemoteMeasurementLine
     */
    ACS_API size_t ACS_ListRemoteMeasurementLine_size(ACS_ListRemoteMeasurementLine* list);

    /** @brief Retrieves item at specified index if it exists.
     * @param index Index of object to retrieve.
     * @returns Object if it exists, otherwise NULL.
     * @relatesalso ACS_ListRemoteMeasurementLine
     */
    ACS_API ACS_RemoteMeasurementLine ACS_ListRemoteMeasurementLine_item(ACS_ListRemoteMeasurementLine* list, size_t index);

    /** @brief Retrieves the number of elements in the list.
     * @returns Number of elements in the list.
     * @relatesalso ACS_ListTemperatureRange
     */
    ACS_API size_t ACS_ListTemperatureRange_size(const ACS_ListTemperatureRange* list);

    /** @brief Retrieves item at specified index if it exists.
     * @param index Index of object to retrieve.
     * @returns Object if it exists, otherwise NULL.
     * @relatesalso ACS_ListTemperatureRange
     */
    ACS_API ACS_TemperatureRange ACS_ListTemperatureRange_item(ACS_ListTemperatureRange* list, size_t index);

#ifdef __cplusplus
}
#endif


#endif // ACS_REMOTE_H
