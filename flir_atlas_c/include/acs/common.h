/** @file
 * @brief Core API
 * @author Teledyne FLIR
 * @copyright Copyright 2023: Teledyne FLIR
 */

#ifndef ACS_COMMON_H
#define ACS_COMMON_H

#if defined(WIN32) || defined(_WIN32) || defined(WINCE) || defined(__CYGWIN__)
# define ACS_PLATFORM_WINDOWS() 1
#else
# define ACS_PLATFORM_WINDOWS() 0
#endif

#if ACS_PLATFORM_WINDOWS() && defined ACS_EXPORTS
# define ACS_API __declspec(dllexport)
#elif defined __GNUC__ && __GNUC__ >= 4
# define ACS_API __attribute__ ((visibility ("default")))
#else
# define ACS_API
#endif

#ifndef __cplusplus
# include <stdbool.h>
#endif
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef ACS_DOC
#define ACS_OWN(x, free) /** @return The returned object must be freed with @ref free. */ x
#define ACS_BORROW(x) /** @return Do not free the returned object. */ x
#define ACS_BORROW_FROM(x, arg) /** @return Do not free the returned object, it's owned by @p arg. */ x
#else
#define ACS_OWN(x, free) x
#define ACS_BORROW(x) x
#define ACS_BORROW_FROM(x, arg) x
#endif

    /** @brief Error code, part of an @ref ACS_Error "error" description. */
    typedef int ACS_ErrorCode;

    /** @struct ACS_ErrorCategory
     * @brief Category (or "domain") of an @ref ACS_Error "error".
     *
     * Each specific category object defines the mapping between @ref ACS_ErrorCode "error codes" and their meaning. Category objects are "singletons" with static storage/lifetime.
     * Example categories: POSIX errors, libcurl errors.
     */
    typedef struct ACS_ErrorCategory_ ACS_ErrorCategory;

    /** @struct ACS_Error
     *  @brief Struct representing an arbitrary error */
    typedef struct ACS_Error_
    {
        /// Error code in the current @ref category. Value `0` means success (no error).
        ACS_ErrorCode code;

        /// Category (or "domain") of the error code. Examples: POSIX, libcurl, ...
        const ACS_ErrorCategory* category;
    } ACS_Error;

    /** @struct ACS_String
     * @brief Opaque character string type.
     */
    typedef struct ACS_String_ ACS_String;

    /** @struct ACS_NativeString
     * @brief Represents a character string with characters of native width for the current platform.
     * @remarks Primarily used for file paths.
     */
    typedef struct ACS_NativeString ACS_NativeString;

    /** @struct ACS_ByteBuffer
     * @brief Byte buffer class
     */
    typedef struct ACS_ByteBuffer_ ACS_ByteBuffer;

#if ACS_PLATFORM_WINDOWS()
    typedef wchar_t ACS_NativePathChar;
#else
    typedef char ACS_NativePathChar;
#endif

    /** @brief Callback type for error events.
     * @param err Error information describing why an operation failed.
     * @param context User-provided data.
     */
    typedef void(*ACS_OnError)(ACS_Error err, void* context);

    /** @brief Callback type for successfully completed asynchronous events.
     * @param context User-provided data.
     */
    typedef void(*ACS_OnCompletion)(void* context);

    /** @brief Type for providing a callback context together with a callback.
     */
    typedef struct ACS_CallbackContext_
    {
        void* context;  ///< User-provided data.
        void(*deleter)(void*);  ///< Deallocation function for the user-provided data. Called when ACS does no longer hold any references to @p context.
    } ACS_CallbackContext;

    /** @brief Represents an ordered pair of integer x- and y-coordinates that defines a point in a two-dimensional plane. */
    typedef struct ACS_Point_
    {
        int x;    //!< The x-coordinate of this Point.
        int y;    //!< The y-coordinate of this Point.

    } ACS_Point;

    /** @brief Represents a rectangle using top-left corner and size. */
    typedef struct ACS_Rectangle_
    {
        int x;      //!< The x-coordinate of the upper-left corner of the Rectangle structure.
        int y;      //!< The y-coordinate of the upper-left corner of the Rectangle structure.
        int width;  //!< The width of Rectangle structure.
        int height; //!< The height of Rectangle structure.
    } ACS_Rectangle;

    /** @brief Represents a circle using center point and radius. */
    typedef struct ACS_Circle_
    {
        int x;      //!< The x-coordinate of the center of the Ellipse structure.
        int y;      //!< The y-coordinate of the center of the Ellipse structure.
        int radius; //!< The radius of the Circle structure.
    } ACS_Circle;

    /** @brief Represents an ellipse using center point and radii. */
    typedef struct ACS_Ellipse_
    {
        int x;      //!< The x-coordinate of the center of the Ellipse structure.
        int y;      //!< The y-coordinate of the center of the Ellipse structure.
        int radiusX; //!< The horizontal radius of the Ellipse structure.
        int radiusY; //!< The vertical radius of the Ellipse structure.
    } ACS_Ellipse;

    ACS_API bool ACS_Point_equals(ACS_Point lhs, ACS_Point rhs);

    /** @brief Specifies the supported distance units */
    enum ACS_DistanceUnit_
    {
        ACS_DistanceUnit_meter,
        ACS_DistanceUnit_feet
    };

    /** @enum ACS_ErrorCondition
     *  @brief Error conditions.
     */
    enum ACS_ErrorCondition
    {
        /// @brief Successful operation result. Indicates the absence of errors.
        ACS_SUCCESS,


        // LIVE (CAMERA)

        /// @brief Connection timed out. This is a terminating state.
        ACS_ERR_CONNECTION_TIME_OUT,

        /// @brief A invalid login or password was used to connect
        ACS_ERR_INVALID_LOGIN,

        /// @brief The Identity was invalid
        ACS_ERR_INVALID_IDENTITY,

        /// @brief Operation failed since the Camera is not connected.
        ACS_ERR_NOT_CONNECTED,

        /// @brief Operation failed since a stream is already active.
        ACS_ERR_ALREADY_STREAMING,

        /// @brief The camera type is unsupported.
        ACS_ERR_UNSUPPORTED_CAMERA_TYPE,

        /// @brief Operation failed since the specified palette cannot be set for the particular camera.
        ACS_ERR_PALETTE_NOT_SUPPORTED_FOR_THIS_CAMERA,

        /// @brief Operation failed since the number of remote measurements of a type has reached maximum.
        ACS_ERR_MAX_REMOTE_MEASUREMENTS_REACHED,

        /// @brief Operation failed since once added remote measurement line cannot change it's orientation.
        ACS_ERR_MEASUREMENT_LINE_ORIENTATION_CHANGE_NOT_ALLOWED,

        /// @brief Operation failed since the particular type of remote measurement is not supported by the camera.
        ACS_ERR_REMOTE_MEASUREMENT_TYPE_NOT_SUPPORTED,


        // RENDER
        
        /// @brief Operation failed because render could not be performed.
        ACS_ERR_COULD_NOT_RENDER,
        

        // IMPORT

        /// @brief Operation was canceled.
        ACS_ERR_CANCELED,


        // DISCOVERY

        /// @brief Specified CommunicationInterface is not supported
        ACS_ERR_INTERFACE_NOT_SUPPORTED,
        /// @brief A scan is already active on the specified CommunicationInterface. This is merely an informative error code, it's not terminating the current scan.
        ACS_ERR_ALREADY_SCANNING,


        // LIVE IMPORT

        /// @brief unable to recognize the image as a correct image
        ACS_ERR_INVALID_IMAGE,


        // LIVE REMOTE

        /// @brief Specified Property is not supported by the Camera
        ACS_ERR_PROPERTY_NOT_SUPPORTED,
        /// @brief Specified Property is read-only
        ACS_ERR_READONLY_PROPERTY,
        /// @brief Specified Property is not subscribable
        ACS_ERR_PROPERTY_NOT_SUBSCRIBABLE,
        /// @brief Property is already subscribed
        ACS_ERR_ALREADY_SUBSCRIBED,
        /// @brief Remote does not have a storage device
        ACS_ERR_MISSING_STORAGE,
        /// @brief Unsupported or unexpected property state
        ACS_ERR_INVALID_PROPERTY_STATE,
        /// @brief Property is not eligible for changing while dual stream is active
        ACS_ERR_CANNOT_SET_WHILE_STREAMING,


        // LIVE NETWORK UTILITY

        /** failed to select IPv4 and/or IPv6 or selection is not supported */
        ACS_ERR_INVALID_IP_FAMILY,
        /** a temporary problem occurred */
        ACS_ERR_TRY_AGAIN,
        /** invalid hint flag */
        ACS_ERR_INVALID_HINT_FLAG,
        /** failed to allocate memory */
        ACS_ERR_FAILED_MEMORY_ALLOC,
        /** host not found */
        ACS_ERR_HOST_NOT_FOUND,
        /** error received from name resolution server  */
        ACS_ERR_NAME_SERVER_ERROR,


        // FORMATS

        /// @brief Operation failed because image does not contain any frames.
        ACS_ERR_IMAGE_WITHOUT_FRAMES,

        /// @brief Operation failed because file format was not valid.
        ACS_ERR_INVALID_FILE_FORMAT,

        /// @brief Operation failed because pixel format was not valid.
        ACS_ERR_INVALID_PIXEL_FORMAT,

        /// @brief Operation failed because pixel data was not valid.
        ACS_ERR_INVALID_PIXEL_DATA,

        /// @brief Some unspecified internal error inside Atlas occurred.
        ACS_ERR_INTERNAL_ERROR,  

        ACS_ERR_UNKNOWN,

        // SYSTEM ERRORS
        ACS_ERR_CONNECTION_REFUSED,

        // LIVE (CAMERA)
        /// @brief Received frame is corrupt.
        ACS_ERR_CORRUPT_FRAME,

        /// @brief A NUC/FFC is in progress.
        ACS_ERR_NUC_IN_PROGRESS,
    };

    /** @brief Get a value corresponding to a named error condition if error was due to a known error condition
     * @note if the error is not related to a known error condition a value corresponding to ACS_ERR_UNKNOWN is returned
     * @returns an integer corresponding to a @ref ACS_Error
     * @relatesalso ACS_Error
     */
    ACS_API int ACS_getErrorCondition(ACS_Error err);

    /** @brief Get a descriptive message from last error on current thread.
     *
     * @returns A pointer to thread-local storage.
     * @relatesalso ACS_Error
     */
    ACS_API ACS_BORROW(const char*) ACS_getLastErrorMessage(void);

    /** @brief Get the code of last error on current thread.
     *
     * @note Equivalent to ACS_getLastError().code
     * @relatesalso ACS_Error
     */
    ACS_API ACS_ErrorCode ACS_getLastErrorCode(void);

    /** @brief Get the category of last error on current thread.
     *
     * @note Equivalent to ACS_getLastError().category
     * @relatesalso ACS_Error
     */
    ACS_API ACS_BORROW(const ACS_ErrorCategory*) ACS_getLastErrorCategory(void);

    /** @brief Get the last error on current thread.
     *
     * @relatesalso ACS_Error
     */
    ACS_API ACS_Error ACS_getLastError(void);

    /** @brief Get error message from ACS_Error.
     *  @relatesalso ACS_Error
     */
    ACS_API ACS_OWN(ACS_String*, ACS_String_free) ACS_getErrorMessage(ACS_Error error);

    /** @brief Create a ACS_String object from null-terminated string.
     * @relatesalso ACS_String
     */
    ACS_API ACS_OWN(ACS_String*, ACS_String_free) ACS_String_createFrom(const char* chars);

    /** @brief Free a string object.
     * @relatesalso ACS_String
     */
    ACS_API void ACS_String_free(const ACS_String* string);

    /** @brief Get pointer to null-terminated `char` string.
     * @relatesalso ACS_String
     */
    ACS_API ACS_BORROW_FROM(const char*, string) ACS_String_get(const ACS_String* string);

    /** @brief Create a ACS_String object from null-terminated string.
     * @relatesalso ACS_NativeString
     */
    ACS_API ACS_OWN(ACS_NativeString*, ACS_NativeString_free) ACS_NativeString_createFrom(const char* chars);

    /** @brief Free a string object.
     * @relatesalso ACS_NativeString
     */
    ACS_API void ACS_NativeString_free(const ACS_NativeString*);

    /** @brief Get pointer to null-terminated `char` string.
     * @relatesalso ACS_NativeString
     */
    ACS_API ACS_BORROW_FROM(const ACS_NativePathChar*, atlas::AnyPath::native_string) ACS_NativeString_get(const ACS_NativeString* string);

    /** @brief Create a ACS_ByteBuffer object from null-terminated string.
     * @relatesalso ACS_ByteBuffer
     */
    ACS_API ACS_OWN(ACS_ByteBuffer*, ACS_ByteBuffer_free) ACS_ByteBuffer_createFrom(const unsigned char* data, size_t size);

    /** @brief Free a byte buffer object.
     * @relatesalso ACS_ByteBuffer
     */
    ACS_API void ACS_ByteBuffer_free(const ACS_ByteBuffer*);

    /** @brief Get pointer to the bytes.
     * @see ACS_ByteBuffer_getSize
     * @relatesalso ACS_ByteBuffer
     */
    ACS_API ACS_BORROW_FROM(const unsigned char*, buffer) ACS_ByteBuffer_getData(const ACS_ByteBuffer* buffer);

    /** @brief Get number of bytes in the buffer.
     * @see ACS_ByteBuffer_getData
     * @relatesalso ACS_ByteBuffer
     */
    ACS_API size_t ACS_ByteBuffer_getSize(const ACS_ByteBuffer* buffer);


    /** @brief Available logging levels
     *
     * Default log level for Release build is off.
     */
    enum ACS_LogLevel_
    {
        ACS_LogLevel_off,   ///< No logging.
        ACS_LogLevel_error, ///< Only logs errors.
        ACS_LogLevel_warn,  ///< Logs errors and warnings.
        ACS_LogLevel_info,  ///< warn plus informative logs.
        ACS_LogLevel_debug, ///< Internal only. Includes info level
        ACS_LogLevel_trace, ///< Internal only. Includes debug level.
    };
    /** @brief See @ref ACS_LogLevel_. */
    typedef int ACS_LogLevel;

    /** @brief Callback type for log messages
     *
     * @param message The message string, NOT null-terminated
     * @param length Length of the message string
     * @param context User-provided data.
     */
    typedef void(*ACS_LogCallback)(const char* message, size_t length, void* context);

    /** @brief Get current logging level
     */
    ACS_API ACS_LogLevel ACS_Logger_getLevel(void);

    /** @brief Set logging level, will take effect immediately
     */
    ACS_API void ACS_Logger_setLevel(ACS_LogLevel level);

    /** @brief Replace the internal log sink with a custom function.
     *
     * @warning This method is not thread safe. It must therefore be used before any other call into ACS.
     *
     * @param callback A callback to receive every log message from ACS.
     * @param context Custom data to be passed to the callback.
     */
    ACS_API void ACS_Logger_setSink(ACS_LogCallback callback, void* context);

#ifdef __cplusplus
}
#endif


#endif // ACS_COMMON_H
