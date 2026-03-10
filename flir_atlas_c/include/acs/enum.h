/** @file
 * @brief ACS Enumerations
 * @author Teledyne FLIR
 * @copyright Copyright 2023: Teledyne FLIR
 */

#ifndef ACS_ENUM_H
#define ACS_ENUM_H

// Image

/** @enum ACS_FlipType_
 *  @brief Flip types supported.
 */
enum ACS_FlipType_
{
    /** No flip. */
    ACS_FlipType_none,

    /** Flip against the vertical axis (i.e. mirror). */
    ACS_FlipType_vertical,

    /** Flip against the horizontal axis (i.e. upside down). */
    ACS_FlipType_horizontal,

    /** Flip against the vertical and horizontal axis (same as ACS_Rotation_degrees180). */
    ACS_FlipType_verticalHorizontal
};

/** @brief Rotation constants (always clockwise). */
enum ACS_RotationAngle_
{
    /** No rotation. */
    ACS_RotationAngle_degrees0,

    /** Rotate 90 degrees clockwise. */
    ACS_RotationAngle_degrees90,

    /** Rotate 180 degrees (same as ACS_FlipType_verticalHorizontal. */
    ACS_RotationAngle_degrees180,

    /** Rotate 270 degrees clockwise (same as 90 degrees counterclockwise). */
    ACS_RotationAngle_degrees270
};

 //live


/** @brief Types of FLIR cameras.
 */
enum ACS_CameraType_
{
    /// Any generic FLIR camera.
    ACS_CameraType_generic = 0x01,
    /// FLIR ONE pluggable camera.
    ACS_CameraType_flirOne = 0x02,
    /// FLIR ONE Edge wireless camera.
    ACS_CameraType_flirOneEdge = 0x04,
    /// FLIR ONE Edge Pro wireless camera.
    ACS_CameraType_flirOneEdgePro = 0x08,
    ACS_CameraType_flirOneEdge5GHz = 0x10,
    /// FLIR ONE Edge Pro wireless camera.
    ACS_CameraType_flirOneEdgePro5GHz = 0x20,
    /// FLIR Leia camera
    ACS_CameraType_internal_leia = 0x80,
    /// FLIR Luke camera
    ACS_CameraType_luke = 0x100,
    /// Unknown type of camera.
    ACS_CameraType_unknown = 0x200
};


 //Live Remote

 //Battery

enum ACS_ChargingState_
{
    /// Indicates that the battery is not charging because the camera is in "developer mode".
    ACS_ChargingState_developer,
    /// Indicates that the battery is not charging because the camera is not connected to an external power supply.
    ACS_ChargingState_noCharging,
    /// Indicates that the battery is charging from external power.
    ACS_ChargingState_managedCharging,

    /** @brief Indicates that the battery is charging the iPhone.
    *
    * @warning This field indicates that IF there is an iPhone plugged in, it will
    *  be receiving power from the RBPDevice battery. However, it is still possible
    *  for the RBPDevice to be in this "mode" EVEN IF THERE IS NO IPHONE ATTACHED.
    */
    ACS_ChargingState_chargingSmartPhone,

    /// Indicates that a charging fault occurred (overheat, etc.)
    ACS_ChargingState_fault,
    /// Indicates that a charging heat fault occured
    ACS_ChargingState_faultHeat,
    /// Indicates that a charging fault occured due to low current from the charging source
    ACS_ChargingState_faultBadCharger,
    /// Indicates that a charging fault exists but the iPhone is being charged
    ACS_ChargingState_chargingSmartPhoneFaultHeat,
    /// Indicates that the device is in charge-only mode
    ACS_ChargingState_managedChargingOnly,
    /// Indicates that the device is in phone-charging-only mode
    ACS_ChargingState_chargingSmartPhoneOnly,
    /// Indicates that a valid battery charging state was not available.
    ACS_ChargingState_bad
};


 //Calibration


enum ACS_CameraState_
{
    ACS_CameraState_notReady,
    ACS_CameraState_cooling,
    ACS_CameraState_ready
};


 //Focus

/** @brief Autofocus modes.
  */
enum ACS_AutoFocusMode_
{
    /// In normal focus mode, camera does not trigger automatic focus.
    ACS_AutoFocusMode_normal,
    /// Camera automatically and continuously manages focus.
    ACS_AutoFocusMode_continuous
};


 //Fusion controller

/** @brief Calculation mask for measurement area
 */
enum ACS_MeasurementCalcMask_
{
    ACS_MeasurementCalcMask_temp = 1U << 1,      ///< Single value temperature/signal, e.g. a spot value
    ACS_MeasurementCalcMask_max = 1U << 2,       ///< Maximum temperature/signal, e.g a area max value
    ACS_MeasurementCalcMask_maxpos = 1U << 3,    ///< Position of maximum temperature
    ACS_MeasurementCalcMask_min = 1U << 4,       ///< Minimum temperature/signal
    ACS_MeasurementCalcMask_minpos = 1U << 5,    ///< Position of minimum temperature
    ACS_MeasurementCalcMask_avg = 1U << 6,       ///< Average temperature/signal
    ACS_MeasurementCalcMask_sdev = 1U << 7,      ///< Standard deviation temperature/signal
    ACS_MeasurementCalcMask_median = 1U << 8,    ///< Median temperature/signal
    ACS_MeasurementCalcMask_iso = 1U << 10,      ///< Isotherm coloring or coverage depending on function
    ACS_MeasurementCalcMask_dimension = 1U << 13,///< Dimension of bounded object, e.g area of box
};

/** @brief Color space type supported. */
enum ACS_ColorSpaceType_
{
    /** Undefined colorspace. */
    ACS_ColorSpaceType_undefined,

    /** 24-bit YCbCr BT.601 Y 16:235, Cb/Cr 16:240. */
    ACS_ColorSpaceType_ycbcr601,

    /** 24-bit YCbCr JFIF Y/Cb/Cr 0:255. */
    ACS_ColorSpaceType_ycbcrJfif,

    /** 24-bit RGB, R/G/B 0:255. */
    ACS_ColorSpaceType_rgb,

    /** 32-bit RGBA, R/G/B/A 0:255. */
    ACS_ColorSpaceType_rgba,

    /** 24-bit BGR, B/G/R 0:255. */
    ACS_ColorSpaceType_bgr,

    /** 32-bit BGRA, B/G/R/A 0:255. */
    ACS_ColorSpaceType_bgra,

    /** 24-bit HSV, H/S/V 0:255.*/
    ACS_ColorSpaceType_hsv,

    /** 8-bit gray scale. */
    ACS_ColorSpaceType_gray,

    /** YUY2/(YUV 4:2:2) format, | Y0U0Y1V0 Y2U1Y3V1 ... |. Y 16:235, U/V 16:240. */
    ACS_ColorSpaceType_yuy2,

    /** YUY2/(YUV 4:2:2) format, | U0Y0V0Y1 U1Y2V1Y3 ... |. Y 16:235, U/V 16:240. */
    ACS_ColorSpaceType_uyvy,

    /** Interleaved YUV 4:2:0 format. Y 16:235, U/V 16:240. */
    ACS_ColorSpaceType_yuvI420
};

/** @brief Describes how images are stored. */
enum ACS_FileFormat_
{
    ACS_FileFormat_jpeg, ///< Image stored as JPEG (or RJPEG (Radiometric JPEG) if it contains measurable thermal data).
    ACS_FileFormat_fff,  ///< Image stored in FLIR File Format.
};

/** @brief Pixel type */
enum ACS_PixelType_
{
    /** Undefined pixel type */
    ACS_PixelType_undefined = 0,

    /** 1 channel 16-bit unsigned data */
    ACS_PixelType_u16c1 = (1 << 0),

    /** 2 channel 16-bit unsigned data */
    ACS_PixelType_u16c2 = (1 << 1),

    /** 1 channel 8-bit unsigned data */
    ACS_PixelType_u8c1 = (1 << 2),

    /** 2 channel 8-bit unsigned data */
    ACS_PixelType_u8c2 = (1 << 3),

    /** 3 channels 8-bit unsigned data */
    ACS_PixelType_u8c3 = (1 << 4),

    /** 4 channels 8-bit unsigned data */
    ACS_PixelType_u8c4 = (1 << 5),

    /** 1 channel 32-bit float data */
    ACS_PixelType_f32c1 = (1 << 6),

    /** 2 channels 32-bit float data */
    ACS_PixelType_f32c2 = (1 << 7),

    /** 3 channels 32-bit float data */
    ACS_PixelType_f32c3 = (1 << 8),

    /** 4 channel 32-bit float data */
    ACS_PixelType_f32c4 = (1 << 9),

    /** 1 channel 16-bit signed short data */
    ACS_PixelType_s16c1 = (1 << 10),

    /** 1 channel 32-bit signed int data */
    ACS_PixelType_s32c1 = (1 << 11),

    /** 1 channel 16-bit float data */
    ACS_PixelType_f16c1 = (1 << 12)
};
#endif //ACS_ENUM_H
