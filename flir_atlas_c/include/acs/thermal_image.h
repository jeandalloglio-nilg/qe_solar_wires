/** @file
 * @brief ACS Thermal Image API
 * @author Teledyne FLIR
 * @copyright Copyright 2023: Teledyne FLIR
 */

#ifndef ACS_THERMAL_IMAGE_H
#define ACS_THERMAL_IMAGE_H

#include "enum.h"
#include "common.h"
#include "buffer.h"
#include "isotherms.h"
#include "measurements.h"
#include "thermal_value.h"

#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif
    /** @enum ACS_LineOrientation_
     *  @brief Defines a line orientation in 2D environment.
     */
    enum ACS_LineOrientation_
    {
        ACS_LineOrientation_horizontal,
        ACS_LineOrientation_vertical
    };

    /** @struct ACS_Line
      * @brief Represents a line as a coordinate and orientation pair.
      * @note For horizontal line the coordinate will be Y position, for vertical line the coordinate will be X position.
      * @note The line is limited by the surrounding image, thus the start end end position can be calculated from the size of surrounding image.
      * @ref ACS_LineOrientation_
      */
    typedef struct ACS_Line_
    {
        int coordinate;
        int orientation;
    } ACS_Line;

    /** @struct ACS_Scale
     *
     * @brief Represents the temperature scale of a radiometric Thermal image.
     */
    typedef struct ACS_Scale_ ACS_Scale;

    /** @struct ACS_ThermalImage
     *
     * @brief This type represents a radiometric Thermal image.
     */
    typedef struct ACS_ThermalImage_ ACS_ThermalImage;

    /** @struct ACS_Image_CameraInformation
     *
     * @brief This type represents a radiometric Thermal image camera information.
     */
    typedef struct ACS_Image_CameraInformation_ ACS_Image_CameraInformation;

    /** @struct ACS_ImageStatistics
     *
     * @brief Gets the image results.
     */
    typedef struct ACS_ImageStatistics_ ACS_ImageStatistics;

    /** @struct ACS_CPUBufferView
     *
     * @brief This type represents the thermal signal data in a thermal image.
     */
    typedef struct ACS_CPUBufferView_ ACS_CPUBufferView;

    /** @brief Specifies the modes supported for ColorDistributionSettings */
    typedef enum ACS_ColorDistributionMode_
    {
        /** @brief The color information in the image is distributed linear to the temperature values of the image.
        * @remarks The palette colors used for each pixel is mapped in a linear fashion between the min and max temperatures of the image.
        * Pixels above and below min and max will contain a color defined in the @ref ACS_Palette.
        */
        ACS_ColorDistribution_temperatureLinear,

        /** @brief The colors are evenly distributed over the existing temperatures of the image and enhance the contrast.
         * @remarks This color distribution can be particularly successful when the image contains few peaks of very low or
         * high temperature's values.
         */
        ACS_ColorDistribution_histogramEqualization,

        /** @brief The color information in the image is distributed linear to the signal values of the image.
        * @remarks The palette colors used for each pixel is mapped in a linear fashion between the min and max signal values of the image.
        * Pixels above and below min and max will contain a color defined in the @ref ACS_Palette.
        */
        ACS_ColorDistribution_signalLinear,

        /** @brief The colors are evenly distributed over the existing temperatures of the image and will preserve brightness while enhancing the contrast.
         * @remarks This color distribution can be particularly successful when the image contains few peaks of very low or
         * high temperature's values.
         */
        ACS_ColorDistribution_plateauHistogramEqualization,

        /** @brief Perform digital detail enhancement which enhance details and/or suppress fixed pattern noise.
         * @remarks In this mode one color might not map to a specific temperature. Which means that the output
         * scale is not radiometric accurate.
         */
        ACS_ColorDistribution_dde,

        /** @brief Entropy modes reserve more shades of gray/colors for areas with more entropy by assigning areas with lower entropy lesser gray shades.
         *  @remarks In this mode one color might not map to a specific temperature. Which means that the output
         *  scale is not radiometric accurate.
         */
        ACS_ColorDistribution_entropy,

        /** @brief Adaptive detail enhancement which is using a edge-preserving and noise-reducing smoothing filter to enhance the contrast in the image.
         * @remarks In this mode one color might not map to a specific temperature. Which means that the output
         * scale is not radiometric accurate.
         */
        ACS_ColorDistribution_ade,

        /** @brief Perform digital detail enhancement which enhance details and/or suppress fixed pattern noise.
         */
        ACS_ColorDistribution_fsx
    } ACS_ColorDistributionMode;

    
    /** @brief Indicates the altitude used as the reference altitude in the GPS information/data.
     *  @see ACS_GpsInformation
     */
    enum ACS_AltitudeReference_
    {
        /** @brief Indicates the altitude value is above sea level.
         */
        ACS_AltitudeReference_seaLevel = 0,
        /** @brief Indicates the altitude value is below sea level and the altitude is indicated as an absolute value in altitude.
         */
        ACS_AltitudeReference_belowSeaLevel
    };

    /** @brief Specifies the voice annotation/comment format. */
    enum ACS_VoiceAnnotationFormat_
    {
        /** @brief unknown sound format - however it is possible that the voice annotation will be playable anyway. */
        ACS_VoiceAnnotationFormat_unknown = 0,

        /** @brief MP3 sound format. */
        ACS_VoiceAnnotationFormat_mp3 = 1,

        /** @brief WAV sound format. */
        ACS_VoiceAnnotationFormat_wav = 2,

        /** @brief invalid sound format - probably the voice annotation will be unreadable. */
        ACS_VoiceAnnotationFormat_invalid,
    };

    /** @brief Type of gas leak, used in gas quantification. */
    enum ACS_GasLeakType_
    {
        ACS_GasLeakType_undefined = -1,
        ACS_GasLeakType_point,
        ACS_GasLeakType_diffused
    };

    /** @brief Wind speed conditions, used in gas quantification*/
    enum ACS_WindSpeed_
    {
        ACS_WindSpeed_undefined = -1,
        ACS_WindSpeed_calm,
        ACS_WindSpeed_normal,
        ACS_WindSpeed_high
    };

    /** @struct ACS_TemperatureLinearSettings
      * @brief Specifies the settings for ColorDistributionMode TemperatureLinearSettings
      */
    typedef struct ACS_TemperatureLinearSettings_
    {
        /** @brief ColorDistribution mode used for this setting */
        ACS_ColorDistributionMode mode;
    } ACS_TemperatureLinearSettings;

    /** @struct ACS_HistogramEqualizationSettings
      * @brief Specifies the settings for ColorDistributionMode HistogramEqualizationSettings
      */
    typedef struct ACS_HistogramEqualizationSettings_
    {
        /** @brief ColorDistribution mode used for this setting */
        ACS_ColorDistributionMode mode;
    } ACS_HistogramEqualizationSettings;

    /** @struct ACS_SignalLinearSettings
      * @brief Specifies the settings for ColorDistributionMode SignalLinearSettings
      */
    typedef struct ACS_SignalLinearSettings_
    {
        /** @brief ColorDistribution mode used for this setting */
        ACS_ColorDistributionMode mode;
    } ACS_SignalLinearSettings;

    /** @struct ACS_PlateauHistogramEqSettings
      * @brief Specifies the settings for ColorDistributionMode PlateauHistogramEqSettings
      */
    typedef struct ACS_PlateauHistogramEqSettings_
    {
        /** @brief ColorDistribution mode used for this setting */
        ACS_ColorDistributionMode mode;

        /** @brief Limits the maximum slope of the mapping function. */
        float maxGain;

        /** @brief Limits the population of any single histogram bin. */
        float percentPerBin;

        /** @brief Increasing values of Linear Percent more accurately preserves the visual representation of an object */
        float linearPercent;

        /** @brief Determines the percentage of the histogram tails which are not ignored when generating the mapping function.*/
        float outlierPercent;

        /** @brief Used to adjust the perceived brightness of the image. */
        float gamma;

    } ACS_PlateauHistogramEqSettings;

    /** @struct ACS_DdeSettings
      * @brief Specifies the settings for ColorDistributionMode Digital Detail Enhancement (DDE) mode.
      */
    typedef struct ACS_DdeSettings_
    {
        /** @brief ColorDistribution mode used for this setting */
        ACS_ColorDistributionMode mode;

        /** @copydoc ACS_PlateauHistogramEqSettings */
        ACS_PlateauHistogramEqSettings plateauHistogramEqSettings;

        /** @brief Detail to background ratio. */
        float detailToBackground;

        /** @brief DDE smoothing factor */
        float smoothingFactor;

        /** @brief Headroom for detail at dynamic range extremes */
        float detailHeadroom;
    } ACS_DdeSettings;

    /** @struct ACS_EntropySettings
      * @brief Specifies the settings for ColorDistributionMode Entropy mode.
      */
    typedef struct ACS_EntropySettings_
    {
        /** @brief ColorDistribution mode used for this setting */
        ACS_ColorDistributionMode mode;

        /** @copydoc ACS_PlateauHistogramEqSettings */
        ACS_PlateauHistogramEqSettings plateauHistogramEqSettings;

        /** @brief Detail to background ratio. */
        float detailToBackground;

        /** @brief DDE smoothing factor */
        float smoothingFactor;

        /** @brief Headroom for detail at dynamic range extremes */
        float detailHeadroom;
    } ACS_EntropySettings;

    /** @struct ACS_AdeSettings
      * @brief Specifies the settings for ColorDistributionMode Adaptive Detail Enhancement (ADE) mode.
      */
    typedef struct ACS_AdeSettings_
    {
        /** @brief ColorDistribution mode used for this setting */
        ACS_ColorDistributionMode mode;

        /** @brief Noise amplification limit (LF) (Low value=Disable this feature) */
        float alphaNoise;

        /** @brief Edge preserving limit (LF); Too avoid halos around sharp edges (High value=Disable this feature). */
        float betaLf;

        /** @brief Edge preserving limit (HF); Too avoid halos around sharp edges (High value=Disable this feature). */
        float betaHf;

        /** @brief LF/HF crossover level; Low value=Only HF enhancement;High value=Only LF enhancement. */
        float betaMix;

        /** @brief Amount of the high pass filter that should be used. */
        float hpBlendingAmount;

        /** @brief Low part of the histogram that is discarded. */
        float lowLimit;

        /** @brief High part of the histogram that is discarded. */
        float highLimit;

        /** @brief Headroom for details at dynamic range extremes. */
        float headRoom;

        /** @brief Footroom for details at dynamic range extremes. */
        float footRoom;

        /** @brief Limits the maximum slope of the mapping function. */
        float gain;

        /** @brief Linear portion used for mapping the colors. */
        float linearMix;
    } ACS_AdeSettings;

    /** @struct ACS_FsxSettings
      * @brief Specifies the settings for ColorDistributionMode FSX/Digital Detail Enhancement (DDE 2.0).
      */
    typedef struct ACS_FsxSettings_
    {
        /** @brief ColorDistribution mode used for this setting */
        ACS_ColorDistributionMode mode;

        /** @brief Maximum allowed difference in the bilateral filtering. */
        float sigmaR;

        /** @brief The weight factor applied to image details. */
        unsigned short alpha;
    } ACS_FsxSettings;

    /** @struct ACS_GasQuantificationInput
      * @brief Input used for running a Gas quantification analysis.
      */
    typedef struct ACS_GasQuantificationInput_
    {
        /** @brief Temperature of surrounding environment.*/
        ACS_ThermalValue ambientTemperature;

        /** @brief Name of the gas.*/
        char gas[32];

        /** @brief Type of leak.
         * @remarks @ref ACS_GasLeakType_
         */
        int leakType;

        /** @brief Wind speed.
         * @remarks @ref ACS_WindSpeed_
         */
        int windSpeed;

        /** @brief Distance to object in meters. */
        int distance;

        /**@brief Detection threshold temperature difference*/
        ACS_ThermalValue thresholdDeltaTemperature;

        /**@brief True if gas is warmer than the surrounding environment.*/
        bool emissive;  
    } ACS_GasQuantificationInput;

    /** @struct ACS_GasQuantificationResult
      * @brief Result from a gas quantification analysis.
      */
    typedef struct ACS_GasQuantificationResult_
    {
        /**@brief Amount of gas flow. */
        double flow;

        /**@brief Gas concentration. */
        double concentration;
    } ACS_GasQuantificationResult;

    /** @struct ACS_GpsInformation
      * @brief Provides basic fields to handle GPS information.
      */
    typedef struct ACS_GpsInformation_
    {
        /** @brief A value indicating whether this gps information is valid */
        bool isValid;

        /** @brief Gps DOP (data degree of precision) */
        float dop;

        /** @brief The altitude.
         * @remarks If the altitude reference is below sea level, the altitude is indicated as an absolute value in altitude. The unit is meters.
         */
        float altitude;

        /** @brief Altitude reference (@ref ACS_AltitudeReference_) indicates if the altitude value is below or above see level. */
        int altitudeRef;

        /** @brief The latitude. */
        double latitude;

        /** @brief Latitude reference
         *
         * @remarks Indicates whether the latitude is north or south latitude.
         *          The value 'N' indicates north latitude, and 'S' is south latitude.
         */
        char latitudeRef;

        /** @brief The longitude */
        double longitude;

        /** @brief Longitude reference.
        *
        * @remarks Indicates whether the longitude is east or west longitude.
        *          The value 'E' indicates east longitude, and 'W' is west longitude.
        */
        char longitudeRef;

        /** @brief Map datum
        *
        * @remarks Indicates the geodetic survey data used by the GPS receiver.
        */
        char mapDatum[20];

        /** @brief The satellites
        *
        * @remarks Indicates the GPS satellites used for measurements.
        *          This tag can be used to describe the number of satellites,
        *          their ID number, angle of elevation, azimuth, SNR and other information in ASCII notation.
        */
        char satellites[20];

        /** @brief The time
        *
        *  @remarks Indicates the time as UTC (Coordinated Universal Time), at second precision.
        */
        time_t timeStamp;
    } ACS_GpsInformation;

    /** @struct ACS_DisplaySettings
      * @brief The DisplaySettings exposes a range of visual attributes which specify how the image is displayed. They also specify how image was displayed when it was initially taken by a camera.
      * @remarks DisplaySettings allows user to specify a "subsection" of interest, while still keeping the entire image available for context.
      * For example, taking an image of a car engine, but "zoom-in" to a faulty spark-plug, while keeping the entire image of the photographed engine for a context.
      * The DisplaySettings effects both the IR and visual image.
      */
    typedef struct ACS_DisplaySettings_
    {
        /** @brief Defines the image zoom factor, where value 1.0 means no zoom. */
        float zoomFactor;
        /** @brief Defines the image zoom pan/offset in X axis, where value 0 means centered. */
        int zoomPanX;
        /** @brief Defines the image zoom pan/offset in Y axis, where value 0 means centered. */
        int zoomPanY;
        /** @brief Defines if the image is flipped.  See @ref ACS_FlipType_ */
        int flip;
    } ACS_DisplaySettings;

    /** @struct ACS_CompassInformation
      * @brief CompassInfo provides compass information.
      */
    typedef struct ACS_CompassInformation_
    {
        int degrees;   /**< @brief Compass direction in degrees, [0, 359]. */
        int roll;      /**< @brief The terminal's rotation in degrees around its own longitudinal axis, [-180, 180]. */
        int pitch;     /**< @brief Sensor pitch in degrees, [-90, 90]. */
        int tilt;      /**< @brief Compass tilt in degrees, [0, 360]. */
    } ACS_CompassInformation;

    /** @struct ACS_VoiceAnnotation
      * @brief This class represents an voice annotation/comment embedded within a Thermal Image.
      */
    typedef struct ACS_VoiceAnnotation_ ACS_VoiceAnnotation;

    /** @struct ACS_ThermalParameters
     * @brief   Defines the parameters of the environment where the thermal image was taken.
     * @remarks These parameters define how to interpret the thermal data. They have impact on the measured temperature
     *          and should be defined very carefully. Setting the right value of parameters defined in this class is the
     *          first step to get the right measurements on thermal image. The most important are reflected temperature
     *          and emissivity. They define how much heat object emits - the reflected temperature should be measured first,
     *          then emissivity.
     */
    typedef struct ACS_ThermalParameters_ ACS_ThermalParameters;

    /** @brief Construction of a thermal image object.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API ACS_OWN(ACS_ThermalImage*, ACS_ThermalImage_free) ACS_ThermalImage_alloc(void);

    /** @brief Construction of a thermal image object with a specific size.
     * @param width Image width.
     * @param height Image height;
     * @relatesalso ACS_ThermalImage
     */
    ACS_API ACS_OWN(ACS_ThermalImage*, ACS_ThermalImage_free) ACS_ThermalImage_create(int width, int height);

    /** @brief Destruction of the thermal image
     * @relatesalso ACS_ThermalImage
     */
    ACS_API void ACS_ThermalImage_free(const ACS_ThermalImage* image);

    /** @brief Check if the file contains thermal data.
     *
     * This function will only check the file header, use @ref ACS_ThermalImage_openFromFile to verify that the specified file is not corrupt.
     *
     * @param path Path to file.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API bool ACS_ThermalImage_isThermalImageFromFile(const char* path);

    /** @brief Check if the file contains thermal data.
     *
     * @remarks This function will only check the file header, use @ref ACS_ThermalImage_openFromMemory to verify that the specified file is not corrupt.
     *
     * @param image Data buffer containing an image.
     * @param size Size of data buffer @p image.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API bool ACS_ThermalImage_isThermalImageFromMemory(const unsigned char* image, size_t size);

    /** @brief Opens a Thermal Image from the specified file.
     *
     * @param thermalImage Thermal image object to open.
     * @param fileName Path to file.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API void ACS_ThermalImage_openFromFile(ACS_ThermalImage* image, const ACS_NativePathChar* fileName);

    /** @brief Opens a Thermal Image from the specified memory location.
     *
     * @param thermalImage Thermal image object to open.
     * @param image Memory buffer containing the thermal image file.
     * @param size Size of data buffer @p image.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API void ACS_ThermalImage_openFromMemory(ACS_ThermalImage* image, const unsigned char* buffer, size_t size);

    /** @brief Save a Thermal Image to the specified file.
     *
     * @param filePath Path to file
     * @param fileFormat @ref ACS_FileFormat_
     * @relatesalso ACS_ThermalImage
     */
    ACS_API void ACS_ThermalImage_saveAs(ACS_ThermalImage* image, const ACS_NativePathChar* filePath, int fileFormat);

    /** @brief Save a Thermal Image to the specified file.
     *
     * @param filePath Path to file
     * @param overlayImage Overlay image buffer @ref ACS_ImageBuffer
     * @param fileFormat @ref ACS_FileFormat_
     * @relatesalso ACS_ThermalImage
     */
    ACS_API void ACS_ThermalImage_saveAsWithOverlay(ACS_ThermalImage* image, const ACS_NativePathChar* filePath, ACS_ImageBuffer* overlayImage, int fileFormat);

    /** @brief Get the image file name.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API const ACS_NativePathChar* ACS_ThermalImage_getFilePath(const ACS_ThermalImage*);

    /** @brief Gets a camera image information object.
     * 
     * @note CameraInformation in a thermal image is optional; in the case of no data, a NULL pointer is returned.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API ACS_OWN(ACS_Image_CameraInformation*, ACS_Image_CameraInformation_free) ACS_ThermalImage_getCameraInformation(const ACS_ThermalImage* image);

    /** @brief Destruction of the camera image information object.
     * @relatesalso ACS_Image_CameraInformation
     */
    ACS_API void ACS_Image_CameraInformation_free(const ACS_Image_CameraInformation* cameraInformation);

    /** @brief Get camera model name.
     * @relatesalso ACS_Image_CameraInformation
     */
    ACS_API const char* ACS_Image_CameraInformation_getModelName(const ACS_Image_CameraInformation* cameraInfo);

    /** @brief Get filter information.
     * @relatesalso ACS_Image_CameraInformation
     */
    ACS_API const char* ACS_Image_CameraInformation_getFilter(const ACS_Image_CameraInformation* cameraInfo);

    /** @brief Get lens information.
     * @relatesalso ACS_Image_CameraInformation
     */
    ACS_API const char* ACS_Image_CameraInformation_getLens(const ACS_Image_CameraInformation* cameraInfo);

    /** @brief Get camera serial number information.
     * @relatesalso ACS_Image_CameraInformation
     */
    ACS_API const char* ACS_Image_CameraInformation_getSerialNumber(const ACS_Image_CameraInformation* cameraInfo);

    /** @brief Get camera program version.
    * @relatesalso ACS_Image_CameraInformation
    */
    ACS_API const char* ACS_Image_CameraInformation_getProgramVersion(const ACS_Image_CameraInformation* cameraInfo);

    /** @brief Get camera article number.
    * @relatesalso ACS_Image_CameraInformation
    */
    ACS_API const char* ACS_Image_CameraInformation_getArticleNumber(const ACS_Image_CameraInformation* cameraInfo);

    /** @brief Get camera calibration title.
    * @relatesalso ACS_Image_CameraInformation
    */
    ACS_API const char* ACS_Image_CameraInformation_getCalibrationTitle(const ACS_Image_CameraInformation* cameraInfo);

    /** @brief Get lens serial number.
    * @relatesalso ACS_Image_CameraInformation
    */
    ACS_API const char* ACS_Image_CameraInformation_getLensSerialNumber(const ACS_Image_CameraInformation* cameraInfo);

    /** @brief Get arc file version.
    * @relatesalso ACS_Image_CameraInformation
    */
    ACS_API const char* ACS_Image_CameraInformation_getArcFileVersion(const ACS_Image_CameraInformation* cameraInfo);

    /** @brief Get arc date/time.
    * @relatesalso ACS_Image_CameraInformation
    */
    ACS_API const char* ACS_Image_CameraInformation_getArcDateTime(const ACS_Image_CameraInformation* cameraInfo);

    /** @brief Get arc signature.
    * @relatesalso ACS_Image_CameraInformation
    */
    ACS_API const char* ACS_Image_CameraInformation_getArcSignature(const ACS_Image_CameraInformation* cameraInfo);

    /** @brief Get camera country code.
    * @relatesalso ACS_Image_CameraInformation
    */
    ACS_API const char* ACS_Image_CameraInformation_getCountryCode(const ACS_Image_CameraInformation* cameraInfo);

    /** @brief Get lower limit of the camera's measurement range.
     * @relatesalso ACS_Image_CameraInformation
     */
    ACS_API ACS_ThermalValue ACS_Image_CameraInformation_getRangeMin(const ACS_Image_CameraInformation* cameraInfo);

    /** @brief Get upper limit of the camera's measurement range.
     * @relatesalso ACS_Image_CameraInformation
     */
    ACS_API ACS_ThermalValue ACS_Image_CameraInformation_getRangeMax(const ACS_Image_CameraInformation* cameraInfo);

    /** @brief Get horizontal field of view in degrees.
     * @relatesalso ACS_Image_CameraInformation
     */
    ACS_API unsigned short ACS_Image_CameraInformation_getHorizontalFoV(const ACS_Image_CameraInformation* cameraInfo);

    /** @brief Get horizontal field of view in degrees.
     * @relatesalso ACS_Image_CameraInformation
     */
    ACS_API float ACS_Image_CameraInformation_getFocalLength(const ACS_Image_CameraInformation* cameraInfo);

    /** @brief Gets the thermal image distance unit.
     * @returns Distance unit (see @ref ACS_DistanceUnit) currently used for image. 
     * @relatesalso ACS_ThermalImage
     */
    ACS_API int ACS_ThermalImage_getDistanceUnit(const ACS_ThermalImage* image);

    /** @brief Sets the thermal image distance unit.
     * @remarks Distance unit (see @ref ACS_DistanceUnit) currently used for image.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API void ACS_ThermalImage_setDistanceUnit(ACS_ThermalImage* image, int distanceUnit);

    /** @brief Gets the thermal image temperature unit.
     * @returns Temperature unit (see @ref ACS_TemperatureUnit) currently used for image.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API int ACS_ThermalImage_getTemperatureUnit(const ACS_ThermalImage* image);

    /** @brief Sets the thermal image temperature unit.
     * @param image Thermal image (@ref ACS_ThermalImage).
     * @param temperatureUnit The (@ref ACS_TemperatureUnit) unit which should be used.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API void ACS_ThermalImage_setTemperatureUnit(ACS_ThermalImage* image, int temperatureUnit);

    /** @brief Gets a temperature value from a point(x,y) in the Thermal Image.
     * @note valid values for x are [0..w) where w is the width of the image.
     * @note valid values for y are [0..h) where h is the height of the image.
     * @return The temperature value in current unit (see @ref ACS_ThermalImage_getTemperatureUnit).
     * @relatesalso ACS_ThermalImage
     */
    ACS_API ACS_ThermalValue ACS_ThermalImage_getValueAt(const ACS_ThermalImage* image, int x, int y);

    /** @brief Gets the width of a thermal image in pixels.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API int ACS_ThermalImage_getWidth(const ACS_ThermalImage* image);

    /** @brief Gets the height of a thermal image in pixels.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API int ACS_ThermalImage_getHeight(const ACS_ThermalImage* image);

    /** @brief Retrieves all temperature values from \p thermalImage for a region defined by \p rectangle into a user-allocated buffer \p valueBuffer.
     * @param valueBuffer is a one-dimensional user-allocated buffer which will contain the retrieved temperature values after function call completes.
     * @param bufferSize is the size of the user-allocated buffer \p valueBuffer in bytes.
     * @param rectangle defines the area to be retrieved.
     * @note `valueBuffer[n]` will correspond to the value at `x + (y * width)` of the rectangle (NOT the width of the ThermalImage)
     * @relatesalso ACS_ThermalImage
     */
    ACS_API void ACS_ThermalImage_getValues(const ACS_ThermalImage* image, double* valueBuffer, size_t bufferSize, const ACS_Rectangle* rectangle);

    /** @brief Gets an image buffer of the raw image pixels.
     *
     * @returns The "raw IR pixels", 16-bit signal data, as stored in the thermal image file (or as received from a camera that streams temperature values).
     * They can be used for measuring etc., and are used during colorization (but doesn't really make sense to render to screen as is).
     * @relatesalso ACS_ThermalImage
     */
    ACS_API ACS_ImageBuffer* ACS_ThermalImage_getSignalData(const ACS_ThermalImage* image);

    /** @brief Gets a specified subarea image buffer of the raw image pixels. 
     *
     * @param x: starting x-axis pixel location of the subarea.
     * @param y: starting y-axis pixel location of the subarea.
     * @param width: width of the subarea.
     * @param height: height of the subarea.
     * 
     * @returns The "raw IR pixels", 16-bit signal data, as stored in the thermal image file (or as received from a camera that streams temperature values).
     * They can be used for measuring etc., and are used during colorization (but doesn't really make sense to render to screen as is).
     * @relatesalso ACS_ThermalImage
     */
    ACS_API ACS_ImageBuffer* ACS_ThermalImage_getSignalDataSubArea(const ACS_ThermalImage* image, int x, int y, int width, int height);

    /** @brief Gets the maximum signal value of the image.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API unsigned short ACS_ThermalImage_getMaxSignalValue(const ACS_ThermalImage* image);

    /** @brief Gets the minimum signal value of the image.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API unsigned short ACS_ThermalImage_getMinSignalValue(const ACS_ThermalImage* image);

    /** @brief Gets a temperature value from a signal.
    *
    * @param signal Signal value.
    * @return Temperature value in current ACS_TemperatureUnit.
    * @relatesalso ACS_ThermalImage
    */
    ACS_API ACS_ThermalValue ACS_ThermalImage_getValueFromSignal(const ACS_ThermalImage* image, unsigned short signal);

    /** @brief Get a non-owning pointer to the scale object for a thermal image.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API ACS_Scale* ACS_ThermalImage_getScale(ACS_ThermalImage* image);

    /** @brief Sets the minimum and maximum scale values.
     *
     * @param min  The minimum scale value.
     * @param max  The maximum scale value.
     *
     * @relatesalso ACS_Scale
     */
    ACS_API void ACS_Scale_setScale(ACS_Scale* scale, ACS_ThermalValue min, ACS_ThermalValue max);

    /** @brief Gets the current minimum scale value.
     *  @relatesalso ACS_Scale
     */
    ACS_API ACS_ThermalValue ACS_Scale_getScaleMin(ACS_Scale* scale);

    /** @brief Gets the current maximum scale value.
     *  @relatesalso ACS_Scale
     */
    ACS_API ACS_ThermalValue ACS_Scale_getScaleMax(ACS_Scale* scale);

    /** @brief Gets the mode of the Color Distribution.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API ACS_ColorDistributionMode ACS_ThermalImage_getColorDistributionMode(const ACS_ThermalImage* image);

    /** @brief Sets the mode of the Color Distribution.
     * @relatesalso ACS_ColorDistributionMode_
     * @relatesalso ACS_ThermalImage
     */
    ACS_API void ACS_ThermalImage_setColorDistributionMode(ACS_ThermalImage* image, int mode);

    /** @brief Gets the color distributions settings for the case of TemperatureLinear mode.
     * @relatesalso ACS_ColorDistributionMode_
     * @relatesalso ACS_ThermalImage
     */
    ACS_API ACS_TemperatureLinearSettings ACS_ThermalImage_getTemperatureLinearSettings(const ACS_ThermalImage* image);

    /** @brief Sets the color distributions settings for the case of TemperatureLinear mode.
     * @relatesalso ACS_ColorDistributionMode_
     * @relatesalso ACS_ThermalImage
     */
    ACS_API void ACS_ThermalImage_setTemperatureLinearSettings(ACS_ThermalImage* image);

    /** @brief Gets the color distributions settings for the case of HistogramEqualization mode.
     * @relatesalso ACS_ColorDistributionMode_
     * @relatesalso ACS_ThermalImage
     */
    ACS_API ACS_HistogramEqualizationSettings ACS_ThermalImage_getHistogramEqualizationSettings(const ACS_ThermalImage* image);

    /** @brief Sets the color distributions settings for the case of HistogramEqualization mode.
     * @relatesalso ACS_ColorDistributionMode_
     */
    ACS_API void ACS_ThermalImage_setHistogramEqualizationSettings(ACS_ThermalImage* image);

    /** @brief Gets the color distributions settings for the case of SignalLinear mode.
     * @relatesalso ACS_ColorDistributionMode_
     * @relatesalso ACS_ThermalImage
     */
    ACS_API ACS_SignalLinearSettings ACS_ThermalImage_getSignalLinearSettings(const ACS_ThermalImage* image);

    /** @brief Sets the color distributions settings for the case of SignalLinear mode.
     * @relatesalso ACS_ColorDistributionMode_
     * @relatesalso ACS_ThermalImage
     */
    ACS_API void ACS_ThermalImage_setSignalLinearSettings(ACS_ThermalImage* image);

    /** @brief Gets the color distributions settings for the case of PlateauHistogramEqualization mode.
     * @relatesalso ACS_ColorDistributionMode_
     * @relatesalso ACS_ThermalImage
     */
    ACS_API ACS_PlateauHistogramEqSettings ACS_ThermalImage_getPlateauHistogramEqSettings(const ACS_ThermalImage* image);

    /** @brief Sets the color distributions settings for the case of PlateauHistogramEqualization mode,
     * using either the supplied parameter values, or default values if the settings pointer is null.
     * @relatesalso ACS_ColorDistributionMode_
     * @relatesalso ACS_ThermalImage
     */
    ACS_API void ACS_ThermalImage_setPlateauHistogramEqSettings(ACS_ThermalImage* image, const ACS_PlateauHistogramEqSettings* settings);

    /** @brief Gets the color distributions settings for the case of DDE mode.
     * @relatesalso ACS_ColorDistributionMode_
     * @relatesalso ACS_ThermalImage
     */
    ACS_API ACS_DdeSettings ACS_ThermalImage_getDdeSettings(const ACS_ThermalImage* image);

    /** @brief Sets the color distributions settings for the case of DDE mode,
     * using either the supplied parameter values, or default values if the settings pointer is null.
     * @relatesalso ACS_ColorDistributionMode_
     * @relatesalso ACS_ThermalImage
     */
    ACS_API void ACS_ThermalImage_setDdeSettings(ACS_ThermalImage* image, ACS_DdeSettings* settings);

    /** @brief Gets the color distributions settings for the case of Entropy mode.
     * @relatesalso ACS_ColorDistributionMode_
     * @relatesalso ACS_ThermalImage
     */
    ACS_API ACS_EntropySettings ACS_ThermalImage_getEntropySettings(const ACS_ThermalImage* image);

    /** @brief Sets the color distributions settings for the case of Entropy mode,
     * using either the supplied parameter values, or default values if the settings pointer is null.
     * @relatesalso ACS_ColorDistributionMode_
     * @relatesalso ACS_ThermalImage
     */
    ACS_API void ACS_ThermalImage_setEntropySettings(ACS_ThermalImage* image, ACS_EntropySettings* settings);

    /** @brief Gets the color distributions settings for the case of ADE mode.
     * @relatesalso ACS_ColorDistributionMode_
     * @relatesalso ACS_ThermalImage
     */
    ACS_API ACS_AdeSettings ACS_ThermalImage_getAdeSettings(const ACS_ThermalImage* image);

    /** @brief Sets the color distributions settings for the case of ADE mode,
     * using either the supplied parameter values, or default values if the settings pointer is null.
     * @relatesalso ACS_ColorDistributionMode_
     * @relatesalso ACS_ThermalImage
     */
    ACS_API void ACS_ThermalImage_setAdeSettings(ACS_ThermalImage* image, const ACS_AdeSettings* settings);

    /** @brief Gets the color distributions settings for the case of FSX mode.
     * @relatesalso ACS_ColorDistributionMode_
     * @relatesalso ACS_ThermalImage
     */
    ACS_API ACS_FsxSettings ACS_ThermalImage_getFsxSettings(const ACS_ThermalImage* image);

    /** @brief Sets the color distributions settings for the case of FSX mode,
     * using either the supplied parameter values, or default values if the settings pointer is null.
     * @relatesalso ACS_ColorDistributionMode_
     * @relatesalso ACS_ThermalImage
     */
    ACS_API void ACS_ThermalImage_setFsxSettings(ACS_ThermalImage* image, const ACS_FsxSettings* settings);

    /** @brief Gets the isotherms collection.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API ACS_Isotherms* ACS_ThermalImage_getIsotherms(ACS_ThermalImage* image);

    /** @brief Gets the measurements collection.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API ACS_Measurements* ACS_ThermalImage_getMeasurements(ACS_ThermalImage* image);

    /** @brief Rotate (relative to current image) the image.
     *
     * @param angle Rotation angle, see @ref ACS_RotationAngle_. This is relative rotation, meaning that @ref ACS_RotationAngle_degrees0 is always a no-op.
     *
     * @note The image pixels will be rotated instantly, so this function may incur heavy computation.
     * @note For streamed images we do not recommend using rotate() due to possible performance degradation (a rotation can consume lot of processing power), it can be done without issue on a file-based ThermalImage.
     * If you want to rotate a live stream we recommend rotating the colorized pixels, which were acquired by using ACS_Colorizer_asRenderer() function.
     * Important: When rotating only the colorized pixels, adding new measurements will require you to handle the difference in the displayed image and the actual ThermalImage i.e. calculate the correct positions and redraw measurement and this can become really complicated.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API void ACS_ThermalImage_setRotationAngle(ACS_ThermalImage* image, int angle);

    /** @brief Flip the image.
     *
     * @param flipType Axis to flip around, see @ref ACS_FlipType_
     *
     * @note The image pixels will be flipped instantly, so this function may incur heavy computation.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API void ACS_ThermalImage_setFlipType(ACS_ThermalImage* image, int flipType);

    /** @brief Crop the image.
     *
     * @param zoomFactorX: factor by which the the width of the image is cropped/zoomed. 1.0x is no-op, <1.0 is unsupported.
     * @param zoomFactorY: factor by which the the height of the image is cropped/zoomed. 1.0x is no-op, <1.0 is unsupported.
     * @param panX: factor by which the the image is panned. 0.0x is no-op, >0.5 or < -0.5 is unsupported.
     * @param panY: factor by which the the image is panned. 0.0x is no-op, >0.5 or < -0.5 is unsupported.
     * @note The image pixels will be cropped instantly, so this function may incur heavy computation.
     *
     * Example operations:
     * - To crop the center of the image, at exactly half the original width and height, use the parameters: crop(2.0, 2.0, 0.0, 0.0).
     * - To crop the left half of the original image, use the parameters: crop(2.0, 1.0, -0.25, 0.0)
     *
     * Note that panning factor limits are related the to the zoom factor used. Maximum pan factor can be calculated as (1 - 1/zoomFactor) / 2.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API void ACS_ThermalImage_setCrop(ACS_ThermalImage* image, double zoomX, double zoomY, double panX, double panY);

    /** @brief Gets the GPS information from the image.
     * @remarks Always check for errors after calling this function, if the error flag is set the returned value is invalid.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API ACS_GpsInformation ACS_ThermalImage_getGpsInformation(const ACS_ThermalImage* image);

    /** @brief Sets the GPS information in the image.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API void ACS_ThermalImage_setGpsInformation(ACS_ThermalImage* image, const ACS_GpsInformation* gps);

    /** @brief Gets the gas quantification information from the image.  
     * @remarks Always check for errors after calling this function, if the error flag is set the returned value is invalid.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API ACS_GasQuantificationInput ACS_ThermalImage_getGasQuantificationInput(const ACS_ThermalImage* image);

    /** @brief Gets the gas quantification result from the image.  
     * @remarks Always check for errors after calling this function, if the error flag is set the returned value is invalid.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API ACS_GasQuantificationResult ACS_ThermalImage_getGasQuantificationResult(const ACS_ThermalImage* image);

    /** @brief Gets the compass information of the image.
     * @remarks Always check for errors after calling this function, if the error flag is set the returned value is invalid.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API ACS_CompassInformation ACS_ThermalImage_getCompassInformation(const ACS_ThermalImage* image);

    /** @brief sets the compass information of the image.
     * @relatesalso ACS_ThermalImage
     */ 
    ACS_API void ACS_ThermalImage_setCompassInformation(ACS_ThermalImage* image, const ACS_CompassInformation* compassInformation);

    /** @brief Get voice annotation from image.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API ACS_OWN(ACS_VoiceAnnotation*, ACS_VoiceAnnotation_free) ACS_ThermalImage_getVoiceAnnotation(ACS_ThermalImage* image);

    /** @brief Release memory allocated for voice annotation.
     * @relatesalso ACS_VoiceAnnotation
     */
    ACS_API void ACS_VoiceAnnotation_free(const ACS_VoiceAnnotation* voiceAnnotation);

    /** @brief Get format for voice annotation @ref ACS_VoiceAnnotationFormat_ 
     * @relatesalso ACS_VoiceAnnotation
     */
    ACS_API int ACS_VoiceAnnotation_format(const ACS_VoiceAnnotation* voiceAnnotation);

    /** @brief Get voice annotation data.
     * @relatesalso ACS_VoiceAnnotation
     */
    ACS_API char* ACS_VoiceAnnotation_data(ACS_VoiceAnnotation* voiceAnnotation);

    /**@brief Get size of voice annotation data.
     * @relatesalso ACS_VoiceAnnotation
     */
    ACS_API size_t ACS_VoiceAnnotation_dataSize(const ACS_VoiceAnnotation* voiceAnnotation);

    /** @brief Gets the display settings for the image, with information on pan and zoom.
     * @remarks Always check for errors after calling this function, if the error flag is set the returned value is invalid.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API ACS_DisplaySettings ACS_ThermalImage_getDisplaySettings(const ACS_ThermalImage* image);

    /** @brief Sets the display settings for the image, with information on pan and zoom.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API void ACS_ThermalImage_setDisplaySettings(ACS_ThermalImage* image, const ACS_DisplaySettings* displaySettings);

    /** @brief Gets the statistics of the calculated image values.
     * @relatesalso ACS_ThermalImage
     */
    ACS_API ACS_ImageStatistics* ACS_ThermalImage_getStatistics(const ACS_ThermalImage* image);

    /** @brief Get calculated min value.
     * @relatesalso ACS_ImageStatistics
     */
    ACS_API ACS_ThermalValue ACS_ImageStatistics_getMin(const ACS_ImageStatistics* stats);

    /** @brief Get calculated max value.
     * @relatesalso ACS_ImageStatistics
     */
    ACS_API ACS_ThermalValue ACS_ImageStatistics_getMax(const ACS_ImageStatistics* stats);

    /** @brief Get calculated average value.
     * @relatesalso ACS_ImageStatistics
     */
    ACS_API ACS_ThermalValue ACS_ImageStatistics_getAverage(const ACS_ImageStatistics* stats);

    /** @brief Get calculated standard deviation value.
     * @relatesalso ACS_ImageStatistics
     */
    ACS_API ACS_ThermalDelta ACS_ImageStatistics_getStandardDeviation(const ACS_ImageStatistics* stats);

    /** @brief Get ACS_Point (x,y-coordinate) to the pixel with the highest temperature.
     * @relatesalso ACS_ImageStatistics
     */
    ACS_API ACS_Point ACS_ImageStatistics_getHotSpot(const ACS_ImageStatistics* stats);

    /** @brief Get ACS_Point (x,y-coordinate) to the pixel with the lowest temperature.
     * @relatesalso ACS_ImageStatistics
     */
    ACS_API ACS_Point ACS_ImageStatistics_getColdSpot(const ACS_ImageStatistics* stats);

    /** @brief Defines the parameters of the environment where the thermal image was taken.
     * @remarks These parameters define how to interpret the thermal data. They have impact on the measured temperature
     *          and should be defined very carefully. Setting the right value of parameters defined in this class is the
     *          first step to get the right measurements on thermal image. The most important are reflected temperature
     *          and emissivity. They define how much heat object emits - the reflected temperature should be measured first,
     *          then emissivity.
     * @relatesalso ACS_ThermalParameters
     */
    ACS_API ACS_ThermalParameters* ACS_ThermalImage_getThermalParameters(ACS_ThermalImage* image);

    /** @brief Gets the distance from camera to the focused object.
     * @remarks Distance in distance unit (see @ref ACS_DistanceUnit).
     * @note    Distance in distance unit compared to fixed meter distance in @ref ACS_Remote_ThermalParameters_objectDistance.
     * @relatesalso ACS_ThermalParameters
     */
    ACS_API double ACS_ThermalParameters_getObjectDistance(const ACS_ThermalParameters* thermalParams);

    /** @brief Sets the distance from camera to the focused object.
     * @param distance   Distance in distance unit (see @ref ACS_DistanceUnit).
     * @note Distance in distance unit compared to fixed meter distance in @ref ACS_Remote_ThermalParameters_objectDistance.
     * @relatesalso ACS_ThermalParameters
     */
    ACS_API void ACS_ThermalParameters_setObjectDistance(ACS_ThermalParameters* thermalParams, double distance);

    /** @brief Gets the emissivity for the focused object.
     * @remarks Emissivity is a value between 0.001 and 1.0 that specifies how much radiation an object emits
     *          compared to the radiation of a theoretical reference object of the same temperature (called a 'blackbody').
     * @relatesalso ACS_ThermalParameters
     */
    ACS_API double ACS_ThermalParameters_getObjectEmissivity(const ACS_ThermalParameters* thermalParams);

    /** @brief Sets the emissivity for the focused object.
     * @param emissivity   Emissivity value.
     * @note Emissivity is a value between 0.001 and 1.0 that specifies how much radiation an object emits
     *       compared to the radiation of a theoretical reference object of the same temperature (called a 'blackbody').
     * @relatesalso ACS_ThermalParameters
     */
    ACS_API void ACS_ThermalParameters_setObjectEmissivity(ACS_ThermalParameters* thermalParams, double emissivity);

    /** @brief Gets the reflected temperature for the focused object.
     * @remarks This parameter is used to compensate for the radiation reflected in the object.
     *          If the emissivity is low and the object temperature relatively far from that of
     *          the reflected it will be important to set and compensate for the reflected apparent temperature correctly.
     * @remarks Temperature in temperature unit (see ACS_TemperatureUnit).
     * @relatesalso ACS_ThermalParameters
     */
    ACS_API ACS_ThermalValue ACS_ThermalParameters_getObjectReflectedTemperature(const ACS_ThermalParameters* thermalParams);

    /** @brief Sets the reflected temperature for the focused object.
     * @param reflectedTemperature   Reflected temperature value in temperature unit (see ACS_TemperatureUnit).
     * @remarks This parameter is used to compensate for the radiation reflected in the object.
     *          If the emissivity is low and the object temperature relatively far from that of
     *          the reflected it will be important to set and compensate for the reflected apparent temperature correctly.
     * @relatesalso ACS_ThermalParameters
     */
    ACS_API void ACS_ThermalParameters_setObjectReflectedTemperature(ACS_ThermalParameters* thermalParams, ACS_ThermalValue reflectedTemperature);

    /** @brief Gets the relative humidity.
     * @remarks Relative humidity is a term used to describe the amount of water vapor that exists
     *          in a gaseous mixture of air and water. 
     * @note    Percentage range compared to fraction range in @ref ACS_Remote_ThermalParameters_relativeHumidity.
     * @relatesalso ACS_ThermalParameters
     */
    ACS_API double ACS_ThermalParameters_getRelativeHumidity(const ACS_ThermalParameters* thermalParams);

    /** @brief Sets the relative humidity.
     * @param relativeHumidity   Relative humidity value in percentage.
     * @remarks Relative humidity is a term used to describe the amount of water vapor that exists
     *          in a gaseous mixture of air and water.
     * @note Percentage range compared to fraction range in @ref ACS_Remote_ThermalParameters_relativeHumidity.
     * @relatesalso ACS_ThermalParameters
     */
    ACS_API void ACS_ThermalParameters_setRelativeHumidity(ACS_ThermalParameters* thermalParams, double relativeHumidity);

    /** @brief Gets the atmospheric temperature.
     * @remarks The atmosphere is the medium between the object being measured and the camera, normally air.
     * @remarks Temperature in temperature unit (see ACS_TemperatureUnit).
     * @relatesalso ACS_ThermalParameters
     */
    ACS_API ACS_ThermalValue ACS_ThermalParameters_getAtmosphericTemperature(const ACS_ThermalParameters* thermalParams);

    /** @brief Sets the atmospheric temperature.
     * @param atmosphericTemperature   Atmospheric temperature value in temperature unit (see ACS_TemperatureUnit).
     * @remarks The atmosphere is the medium between the object being measured and the camera, normally air.
     * @relatesalso ACS_ThermalParameters
     */
    ACS_API void ACS_ThermalParameters_setAtmosphericTemperature(ACS_ThermalParameters* thermalParams, ACS_ThermalValue atmosphericTemperature);

    /** @brief Gets the atmospheric transmission.
     * @note Transmission coefficient of the atmosphere between the scene and the camera, normally air.
     * @relatesalso ACS_ThermalParameters
     */
    ACS_API double ACS_ThermalParameters_getAtmosphericTransmission(const ACS_ThermalParameters* thermalParams);

    /** @brief Sets the atmospheric transmission.
     * @param transmission   Atmospheric transmission value.
     * @note Transmission coefficient of the atmosphere between the scene and the camera, normally air.
     *       If this value is set to 0, the atmospheric transmission will be calculated automatically.
     * @relatesalso ACS_ThermalParameters
     */
    ACS_API void ACS_ThermalParameters_setAtmosphericTransmission(ACS_ThermalParameters* thermalParams, double transmission);

    /** @brief Gets the external optics temperature.
     * @remarks External optics temperature is a temperature of an external accessory attached to the Thermal camera
     *          Used to compensate for optical accessory, such as a heat shield or a macro lenses.
     *          Specify the temperature of, e.g., an external lens or heat shield.
     *          External optics will absorb some of the radiation.
     * @remarks Temperature in temperature unit (see ACS_TemperatureUnit).
     * @relatesalso ACS_ThermalParameters
     */
    ACS_API ACS_ThermalValue ACS_ThermalParameters_getExternalOpticsTemperature(const ACS_ThermalParameters* thermalParams);

    /** @brief Sets the external optics temperature.
     * @param opticsTemperature   External optics temperature value in temperature unit (see ACS_TemperatureUnit).
     * @remarks Used to compensate for optical accessory, such as a heat shield or a macro lenses.
     *          Specify the temperature of, e.g., an external lens or heat shield.
     *          External optics will absorb some of the radiation.
     * @relatesalso ACS_ThermalParameters
     */
    ACS_API void ACS_ThermalParameters_setExternalOpticsTemperature(ACS_ThermalParameters* thermalParams, ACS_ThermalValue opticsTemperature);

    /** @brief Gets the external optics transmission.
     * @note Used to compensate for optical accessory, such as a heat shield or a macro lenses.
     *       Specify the transmission of, e.g., an external lens or heat shield.
     *       External optics will absorb some of the radiation.
     * @relatesalso ACS_ThermalParameters
     */
    ACS_API double ACS_ThermalParameters_getExternalOpticsTransmission(const ACS_ThermalParameters* thermalParams);

    /** @brief Sets the external optics transmission.
     * @param opticsTransmission   External optics transmission value.
     * @note Used to compensate for optical accessory, such as a heat shield or a macro lenses.
     *       Specify the transmission of, e.g., an external lens or heat shield.
     *       External optics will absorb some of the radiation.
     * @relatesalso ACS_ThermalParameters
     */
    ACS_API void ACS_ThermalParameters_setExternalOpticsTransmission(ACS_ThermalParameters* thermalParams, double opticsTransmission);

#ifdef __cplusplus
}
#endif


#endif // ACS_THERMAL_IMAGE_H
