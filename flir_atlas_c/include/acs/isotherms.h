/** @file
 * @brief ACS Isotherms API
 * @author Teledyne FLIR 
 * @copyright Copyright 2023: Teledyne FLIR 
 */

#ifndef ACS_ISOTHERMS_H
#define ACS_ISOTHERMS_H

#include <acs/common.h>
#include <acs/thermal_value.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** @brief Color described by Luminance (Y), Chrominance blue (Cb) and Chrominance red (Cr). */
    typedef struct ACS_Ycbcr_
    {
        unsigned char y;    //!< Luminance          unit = 1/255
        unsigned char cb;   //!< Chrominance (blue) unit = 1/255
        unsigned char cr;   //!< Chrominance (red)  unit = 1/255
    } ACS_Ycbcr;

    /**@brief YCbCr  65,  99, 211  RGB 191,   0,   0 */
    ACS_API ACS_Ycbcr ACS_Isotherms_Color_red(); 

    /**@brief YCbCr 112,  72,  57, RGB   0, 191,   0 */
    ACS_API ACS_Ycbcr ACS_Isotherms_Color_green();

    /**@brief YCbCr  34, 211, 114, RGB   0,   0, 191 */
    ACS_API ACS_Ycbcr ACS_Isotherms_Color_blue();

    /**@brief YCbCr 161,  44, 141, RGB 191, 191,   0 */
    ACS_API ACS_Ycbcr ACS_Isotherms_Color_yellow();

    /**@brief YCbCr 131, 156,  44, RGB   0, 191, 191 */
    ACS_API ACS_Ycbcr ACS_Isotherms_Color_cyan();

    /**@brief YCbCr  83, 183, 198, RGB 191,   0, 191 */
    ACS_API ACS_Ycbcr ACS_Isotherms_Color_magenta();

    /**@brief YCbCr 126, 128, 128, RGB 128, 128, 128 */
    ACS_API ACS_Ycbcr ACS_Isotherms_Color_gray();

    /** @brief Represent how the isotherm is combined with the image.
     */
    enum ACS_BlendingMode_
    {
        /** @brief Use a single color for the isotherm. */
        ACS_BlendingMode_solid,

        /** @brief Use a transparent isotherm. */
        ACS_BlendingMode_transparent,

        /** @brief Use the Y (in YCbCr) value from the palette underneath the isotherm. */
        ACS_BlendingMode_followY,

        /** @brief Link several isotherms by their Y value (in YCbCr). */
        ACS_BlendingMode_linkedY,
    };

    /** @brief Represents the different types of isotherms.
     */
    enum ACS_IsothermTypes_
    {
        /** @brief Invalid value. */
        ACS_IsothermTypes_unsupported,

        /** @brief Above type isotherm. */
        ACS_IsothermTypes_above,

        /** @brief Below type isotherm. */
        ACS_IsothermTypes_below,

        /** @brief Interval type isotherm. */
        ACS_IsothermTypes_interval,

        /** @brief Humidity type isotherm. */
        ACS_IsothermTypes_humidity,

        /** @brief Insulation type isotherm. */
        ACS_IsothermTypes_insulation,
    };

    /** @brief Represents color fill modes for isotherms.
     */
    enum ACS_FillModes_
    {
        /** @brief Blended color. */
        ACS_FillModes_blendedColor,

        /** @brief Palette containing one or more colors. */
        ACS_FillModes_palette,
    };

    /** @struct ACS_Isotherms
     * @brief Represents an opaque collection of isotherm objects.
     */
    typedef struct ACS_Isotherms_ ACS_Isotherms;
    
    /** @struct ACS_Isotherm
     * @brief A proxy representing an Isotherm object.
     */
    typedef struct ACS_Isotherm_ ACS_Isotherm;

    /** @struct ACS_ListIsotherm
     * @brief Represents a list of Isotherm objects.
     */
    typedef struct ACS_ListIsotherm_ ACS_ListIsotherm;

    /** @struct ACS_ListYcbcr
     * @brief Represents a list of Ycbcr colors.
     * @relatesalso ACS_Ycbcr 
     */
    typedef struct ACS_ListYcbcr_ ACS_ListYcbcr;

    /** @brief An isotherm that is covering an area starting at the cutoff and above that temperature. */
    typedef struct ACS_Isotherm_Above_
    {
        ACS_ThermalValue cutoff;
    } ACS_Isotherm_Above;

    /** @brief An isotherm that is covering an area starting at the cutoff and below that temperature. */
    typedef struct ACS_Isotherm_Below_
    {
        ACS_ThermalValue cutoff;
    } ACS_Isotherm_Below;

    /** @brief An isotherm that is covering an area between the defined min and max temperature. */
    typedef struct ACS_Isotherm_Interval_
    {
        /**@brief Lower bound*/
        ACS_ThermalValue min;

        /**@brief Upper bound*/
        ACS_ThermalValue max;
    } ACS_Isotherm_Interval;

    /** @brief The humidity isotherm can detect areas where there is a risk of mold growing, or where there is a risk of the humidity falling out as liquid water (i.e., the dew point). */
    typedef struct ACS_Isotherm_Humidity_
    {
        float airHumidity;
        float airHumidityAlarmLevel;

        ACS_ThermalValue atmosphericTemperature;
        ACS_ThermalValue dewPointTemperature;
        ACS_ThermalValue thresholdTemperature;
    } ACS_Isotherm_Humidity;

    /** @brief The insulation isotherm can detect areas where there may be an insulation deficiency in the building. 
     * @remarks It will trigger when the insulation level falls below a preset value of the energy leakage through the 
     * building structure - the so-called thermal index/insulation factor. Different building codes recommend different values 
     * for the thermal index/insulation factor, but typical values are 60 - 80% for new buildings. 
     * Refer to your national building code for recommendations. 
     */
    typedef struct ACS_Isotherm_Insulation_
    {
        ACS_ThermalValue indoorAirTemperature;
        ACS_ThermalValue outdoorAirTemperature;
        float insulationFactor;
        ACS_ThermalValue insulationTemperature;
    } ACS_Isotherm_Insulation;

    /** @brief Union of structs representing different types of isotherms.
     *  @relatesalso ACS_Isotherm_Type
     */
    typedef union ACS_Isotherm_TypeValue_
    {
        /**@brief Contains data-members for the case of an Above type isotherm.*/
        ACS_Isotherm_Above above;

        /**@brief Contains data-members for the case of a Below type isotherm.*/
        ACS_Isotherm_Below below;

        /**@brief Contains data-members for the case of an Interval type isotherm.*/
        ACS_Isotherm_Interval interval;

        /**@brief Contains data-members for the case of a Humidity type isotherm.*/
        ACS_Isotherm_Humidity humidity;

        /**@brief Contains data-members for the case of an Insulation type isotherm.*/
        ACS_Isotherm_Insulation insulation;
    } ACS_Isotherm_TypeValue;


    /** @brief Tagged union used for unifying Isotherm interfaces.
     * @remarks User must ensure that type and value match. 
     */
    typedef struct ACS_Isotherm_Type_ 
    {
        /**@brief @ref ACS_IsothermTypes_ */
        int type;

        /**@brief The isotherm value corresponding to the type. Must be valid for correct usage.
         * @remarks Inconsistent usage is invalid. 
         */
        ACS_Isotherm_TypeValue value;
    } ACS_Isotherm_Type;


    /** @brief Palette used with a @ref ACS_Isotherm.
     * @remarks When this object is retrieved from @ref ACS_Isotherm_getFillMode the user gets ownership of the palette, which is a dynamically allocated opaque object.
     */
    typedef struct ACS_FillMode_Palette_
    {
        /** @brief YCbCr palette colors.
         *  @see ACS_ListYcbcr_alloc
         *  @see ACS_ListYcbcr_free
         */
        ACS_ListYcbcr* colors;
    } ACS_FillMode_Palette;


    /** @brief Defines the use of a blended color. */
    typedef struct ACS_FillMode_BlendedColor_
    {
        /**@brief @ref ACS_BlendingMode_ */
        int blendingMode;

        /**@brief Ycbcr color*/
        ACS_Ycbcr color;
    } ACS_FillMode_BlendedColor;

    /** @brief Represents a value for a fill mode which can be used with a @ref ACS_Isotherm. */
    typedef union ACS_FillModeType_
    {
        ACS_FillMode_Palette palette;
        ACS_FillMode_BlendedColor color;
    } ACS_FillModeType;

    /** @brief Settings for coloring an isotherm. */
    typedef struct ACS_Isotherm_FillMode_
    {
        /**@brief @ref ACS_FillModes_ */
        int type;

        /**@brief The fill mode member object corresponding to the type should be valid for correct usage.
         * @remarks Inconsistent usage is invalid.
         */
        ACS_FillModeType value;
    } ACS_Isotherm_FillMode;


    /**@brief Get a @ref ACS_Isotherm_Type with default values for a specified isotherm type.
     *
     * @param isothermType Must correspond to a valid @ref ACS_IsothermTypes_ value.
     */
    ACS_API ACS_Isotherm_Type ACS_Isotherm_Type_getDefault(int isothermType);

    /** @brief  Creates a new isotherm
     *
     *  @param type Isotherm type (e.g. Above, Interval) and threshold value(s)
     *  @param fillMode Color blending and palette options
     *  @returns a non-owning pointer to the new isotherm
     *
     *  @remarks Error will occur when adding another isotherm would exceed the limit of isotherms allowed.
     *  @remarks Any usage with ACS_IsothermTypes_unsupported is invalid.
     *
     * @relatesalso ACS_Isotherms
     */ 
    ACS_API ACS_Isotherm* ACS_Isotherms_add(ACS_Isotherms* isotherms, const ACS_Isotherm_Type* type, const ACS_Isotherm_FillMode* fillMode);


    /** @brief  Removes an isotherm if it exists, otherwise does nothing.
     * @param id isotherm identifier
     * @relatesalso ACS_Isotherms
     */
    ACS_API void ACS_Isotherms_remove(ACS_Isotherms* isotherms, size_t id);

    /** @brief  Search for an existing isotherm
     *  @param id isotherm identifier
     *  @returns A non-owning pointer to an isotherm if it exists, otherwise nullptr
     * @relatesalso ACS_Isotherms
     */  
     ACS_API ACS_Isotherm* ACS_Isotherms_find(ACS_Isotherms* isotherms, size_t id);
    
    /** @brief Remove all isotherms.
     * @relatesalso ACS_Isotherms
     */
    ACS_API void ACS_Isotherms_clear(ACS_Isotherms* isotherms);

    /** @brief Check if any isotherms exist.
     * @relatesalso ACS_Isotherms
     */
    ACS_API bool ACS_Isotherms_empty(const ACS_Isotherms* isotherms);

    /** @brief Returns the number of isotherms.
     * @relatesalso ACS_Isotherms
     */
    ACS_API size_t ACS_Isotherms_size(const ACS_Isotherms* isotherms);

    /** @brief Get the isotherms id
     * @relatesalso ACS_Isotherm
     */
    ACS_API size_t ACS_Isotherm_getId(const ACS_Isotherm* isotherm);

    /** @brief Get the isotherm type (e.g. above, interval).
     * @returns @ref ACS_IsothermTypes_
     * @relatesalso ACS_Isotherm
     */
    ACS_API ACS_Isotherm_Type ACS_Isotherm_getType(const ACS_Isotherm* isotherm);

    /** @brief Set the isotherm type (e.g. above, interval) and its value(s).
     *  @param newType must not be @ref ACS_IsothermTypes_unsupported and must be internally consistent.
     *  @relatesalso ACS_Isotherm
     */
    ACS_API void ACS_Isotherm_setType(ACS_Isotherm* isotherm, const ACS_Isotherm_Type* newType);

    /** @brief Get the fill mode (i.e. blending and color options) of the isotherm.
     * @remarks When the FillMode is a palette, the user get ownership of the palette which is a dynamically allocated opaque object.
     * @relatesalso ACS_Isotherm
     */
    ACS_API ACS_Isotherm_FillMode ACS_Isotherm_getFillMode(const ACS_Isotherm* isotherm);

    /** @brief Set the fill mode (i.e. blending and color options) of the isotherm.
     * @param fillmode The new fillmode to use.
     * @relatesalso ACS_Isotherm
     */
    ACS_API void ACS_Isotherm_setFillMode(ACS_Isotherm* isotherm, const ACS_Isotherm_FillMode* fillmode);

    
    /**@brief Get palette color 1.
     * @relatedalso ACS_Isotherm
     */
    ACS_API ACS_Ycbcr ACS_Isotherms_getPaletteColor1(const ACS_Isotherms* isotherms);

    /**@brief Get palette color 2.
     * @relatedalso ACS_Isotherm
     */
    ACS_API ACS_Ycbcr ACS_Isotherms_getPaletteColor2(const ACS_Isotherms* isotherms);

    /** @brief Gets all active isotherms
     * @remarks The user has ownership of the list, but NOT the contained elements. Therefor, only the list itself needs to be freed.
     * @returns Non-owning pointers to all active isotherms
     * @relatesalso ACS_Isotherms
     */
    ACS_API ACS_OWN(ACS_ListIsotherm*, ACS_ListIsotherm_free) ACS_Isotherms_getAll(ACS_Isotherms* isotherms);

    /** @brief Frees a list of isotherms.
     * @remarks Isotherms pointed to by the list are not freed as they are not necessarily owned by the owner of the list.
     * @relatesalso ACS_ListIsotherm
     */
    ACS_API void ACS_ListIsotherm_free(const ACS_ListIsotherm* list);

    /** @brief Retrieves the number of elements in the list.
     * @returns Number of elements in the list.
     * @relatesalso ACS_ListIsotherm
     */
    ACS_API size_t ACS_ListIsotherm_size(ACS_ListIsotherm* list);

    /** @brief Retrieves item at specified index if it exists.
     * @param index Index of object to retrieve.
     * @returns Object if it exists, otherwise NULL.
     * @relatesalso ACS_ListIsotherm
     */
    ACS_API ACS_Isotherm* ACS_ListIsotherm_item(ACS_ListIsotherm* list, size_t index);

    /** @brief Retrieves the number of elements in the list.
     * @returns Number of elements in the list.
     * @relatesalso ACS_ListYcbcr
     */
    ACS_API size_t ACS_ListYcbcr_size(const ACS_ListYcbcr* list);


    /**@brief */
    ACS_API ACS_OWN(ACS_ListYcbcr*, ACS_ListYcbcr_free) ACS_ListYcbcr_alloc();

    /**@brief Frees a list of colors */
    ACS_API void ACS_ListYcbcr_free(const ACS_ListYcbcr* list);

    /** @brief Retrieves item at specified index if it exists.
     * @param index Index of object to retrieve.
     * @remarks returned object is only valid if the index is in range.
     * @returns Valid object if index was in range, otherwise a garbage value.
     * @relatesalso ACS_ListYcbcr
     */
    ACS_API ACS_Ycbcr ACS_ListYcbcr_item(ACS_ListYcbcr* list, size_t index);

    /** @brief Adds a new Ycbcr color to the list.
     * @relatesalso ACS_ListYcbcr
     */
    ACS_API void ACS_ListYcbcr_addItem(ACS_ListYcbcr* list, ACS_Ycbcr newItem);

    /** @brief Removes an item from the list
     * @relatesalso ACS_ListYcbcr
     */
    ACS_API void ACS_ListYcbcr_removeItem(ACS_ListYcbcr* list, ACS_Ycbcr item);

    /** @brief Removes all items from the list.
     * @relatesalso ACS_ListYcbcr
     */
    ACS_API void ACS_ListYcbcr_clear(ACS_ListYcbcr* list);


#ifdef __cplusplus
}
#endif

#endif // ACS_ISOTHERMS_H
