/** @file
 * @brief ACS Image Buffer API
 * @author Teledyne FLIR
 * @copyright Copyright 2023: Teledyne FLIR
 */

#ifndef ACS_BUFFER_H
#define ACS_BUFFER_H

#include "common.h"

#ifdef __cplusplus
extern "C"
{
#endif
    /**@brief Function for changing an image.*/
    typedef void (*ACS_ImageMutator)(unsigned char*);

    /** @struct ACS_ImageBuffer
     * @brief Image buffer responsible for allocating and containing pixel data.
     */
    typedef struct ACS_ImageBuffer_ ACS_ImageBuffer;

    /** @brief Constructs a buffer object.
     * @relatesalso ACS_ImageBuffer
     */
    ACS_API ACS_OWN(ACS_ImageBuffer*, ACS_ImageBuffer_free) ACS_ImageBuffer_alloc(void);

    /** @brief Frees a buffer object.
     * @relatesalso ACS_ImageBuffer
     */
    ACS_API void ACS_ImageBuffer_free(const ACS_ImageBuffer* buffer);

    /** @brief Get a pointer to the pixel data in the buffer.
     * @relatesalso ACS_ImageBuffer
     */
    ACS_API const unsigned char* ACS_ImageBuffer_getData(const ACS_ImageBuffer* buffer);

    /** @brief Copies the content of a buffer to this buffer based on the dynamic types.
     * @relatesalso ACS_ImageBuffer
     */
    ACS_API ACS_ImageBuffer* ACS_ImageBuffer_copyData(const ACS_ImageBuffer* src);
    
    /** @brief Get buffer width
     * @relatesalso ACS_ImageBuffer
     */
    ACS_API int ACS_ImageBuffer_getWidth(const ACS_ImageBuffer* buffer);

    /** @brief Get buffer height
     * @relatesalso ACS_ImageBuffer
     */
    ACS_API int ACS_ImageBuffer_getHeight(const ACS_ImageBuffer* buffer);

    /** @brief Get distance in bytes between starts of consecutive lines in the buffer
     * @relatesalso ACS_ImageBuffer
     */
    ACS_API int ACS_ImageBuffer_getStride(const ACS_ImageBuffer* buffer);

    /** @brief Get the @ref ACS_ColorSpaceType_ "color space".
     * @relatesalso ACS_ImageBuffer
     */
    ACS_API int ACS_ImageBuffer_getColorSpace(const ACS_ImageBuffer* buffer);

    /** @brief Get the @ref ACS_PixelType_ "pixel type". */
    ACS_API int ACS_ImageBuffer_getPixelType(const ACS_ImageBuffer* buffer);
    
    /** @brief Get the bytes per pixel. */
    ACS_API int ACS_ImageBuffer_getBytesPerPixel(const ACS_ImageBuffer* buffer);
    
    /**@brief Performs mutating operation on image under lock.
     * @param buffer Image data that mutator operats on.
     * @param mutator Function which operates on image data.
     * @relatesalso ACS_ImageBuffer
     */
    ACS_API void ACS_ImageBuffer_with(ACS_ImageBuffer* buffer, ACS_ImageMutator mutator);

#ifdef __cplusplus
}
#endif


#endif // ACS_BUFFER_H
