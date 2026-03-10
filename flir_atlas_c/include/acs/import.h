/** @file
 * @brief ACS Import API
 * @author Teledyne FLIR
 * @copyright Copyright 2023: Teledyne FLIR
 */

#ifndef ACS_IMPORT_H
#define ACS_IMPORT_H

#include <time.h>

#include "common.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** @struct ACS_Importer
     * @brief Imports image files from a camera.
     * @see ACS_Camera_getImporter
     */
    typedef struct ACS_Importer_ ACS_Importer;

    /** @struct ACS_Exporter
     * @brief Exports image files to a camera.
     * @see ACS_Camera_getExporter
     */
    typedef struct ACS_Exporter_ ACS_Exporter;

    /** @brief Describes different possible file locations in the cameras filesystem. */
    enum ACS_Location_
    {
        ACS_Location_unknown,      ///< Unknown root.
        ACS_Location_active,       ///< The active camera folder used for storing images
        ACS_Location_imageBase,    ///< Base folder for images (e.g. "DCIM" on the SD card)
    };

    /** @struct ACS_FileReference
     * @brief Abstract file path on camera's filesystem.
     * @see ACS_Importer
     */
    typedef struct ACS_FileReference_ ACS_FileReference;

    /** @struct ACS_FolderReference
     * @brief Abstract folder path on camera's filesystem.
     * @see ACS_Importer
     */
    typedef struct ACS_FolderReference_ ACS_FolderReference;

    /** @struct ACS_ListFolderInfo
     * @brief List of @ref ACS_FolderInfo "folder info" objects.
     */
    typedef struct ACS_ListFolderInfo_ ACS_ListFolderInfo;

    /** @struct ACS_FolderInfo
     * @brief Folder meta data, like location and modified time
     *
     * Info objects does not contain any files by themselves, however files can be queried via @ref ACS_Importer.
     */
    typedef struct ACS_FolderInfo_ ACS_FolderInfo;

    typedef void(*ACS_OnReceivedListFolderInfo)(const ACS_ListFolderInfo*, void*);
    typedef void(*ACS_OnReceivedFolderInfo)(const ACS_FolderInfo*, void*);

    /** @struct ACS_ListFileInfo
     * @brief List of @ref ACS_FileInfo "file info" objects.
     */
    typedef struct ACS_ListFileInfo_ ACS_ListFileInfo;

    /** @struct ACS_FileInfo
      * @brief File meta data, like location and size
      *
      * Does not contain any file content, see @ref ACS_Importer_importFile.
      */
    typedef struct ACS_FileInfo_ ACS_FileInfo;

    typedef void(*ACS_OnReceivedListFileInfo)(const ACS_ListFileInfo*, void*);


    /** @brief Callback for reporting download progress.
     * @param file Reference to the file currently being downloaded. NOTE: Use @ref ACS_FileReference_equal instead of pointer comparison.
     * @param current Number of bytes downloaded of current file.
     * @param total File size in number of bytes of current file. This may be unknown (0) for some requests.
     * @param context User-provided data.
     */
    typedef void(*ACS_OnProgress)(const ACS_FileReference* file, long long current, long long total, void* context);
    typedef void(*ACS_OnReceivedFileInfo)(const ACS_FileInfo*, void*);


    /** @brief List images (JPEG, CSQ, SEQ, FFF) in (and optionally recursively from) the specified path from the camera.
     * @param folder Base folder for listing images.
     * @param recursive Indicates whether files should be listed recursively.
     * @param onReceived Called from a background thread on success with the result (will only be invoked once).
     * @param onError Called from a background thread on any failure (will only be invoked once).
     * @param context User-provided data.
     * @relatesalso ACS_Importer
     */
    ACS_API void ACS_Importer_listImages(ACS_Importer* importer, const ACS_FolderReference* folder, bool recursive, ACS_OnReceivedListFileInfo onReceived, ACS_OnError onError, void* context);

    /** @brief List folders in the specified path from the camera.
     * @param rootFolder Base folder for listing files.
     * @param recursive Indicates whether files should be listed recursively.
     * @param onReceived Called from a background thread on success with the result (will only be invoked once).
     * @param onError Called from a background thread on any failure (will only be invoked once).
     * @param context User-provided data.
     * @relatesalso ACS_Importer
     */
    ACS_API void ACS_Importer_listWorkFolders(ACS_Importer* importer, const ACS_FolderReference* rootFolder, bool recursive, ACS_OnReceivedListFolderInfo onReceived, ACS_OnError onError, void* context);

    /**
    * @brief Import (download) a file to a local drive.
    * @pre @p destinationFolder must exist.
    * @param file The file to download.
    * @param destinationFolder Path to the directory where the file will be stored (example: "/home/user/images/").
    * @param overwrite Behaviour for handling already existing files.
    * @param onCompletion Called from a background thread when import is completed (will only be invoked once).
    * @param onError Called from a background thread on any failure (will only be invoked once).
    * @param onProgress Called continuously from a background thread to report transmission progress.
    * @param context User-provided data.
    * @relatesalso ACS_Importer
    */
    ACS_API void ACS_Importer_importFile(ACS_Importer* importer, const ACS_FileReference* file, const char* destinationFolder, bool overwrite, ACS_OnCompletion onCompletion, ACS_OnError onError, ACS_OnProgress onProgress, void* context);

    /**
    * @brief Import (download) a file to a local drive.
    * @pre The path part of @p destinationFile must exist.
    * @param file The file to download.
    * @param destinationFile The name of the file stored in the local file system, may include relative or absolute paths.
    * @param overwrite Behaviour for handling already existing files.
    * @param onCompletion Called from a background thread when import is completed (will only be invoked once).
    * @param onError Called from a background thread on any failure (will only be invoked once).
    * @param onProgress Called continuously from a background thread to report transmission progress.
    * @param context User-provided data.
    * @relatesalso ACS_Importer
    */
    ACS_API void ACS_Importer_importFileAs(ACS_Importer* importer, const ACS_FileReference* file, const char* destinationFile, bool overwrite, ACS_OnCompletion onCompletion, ACS_OnError onError, const ACS_OnProgress onProgress, void* context);

    /** @brief Export (upload) a file to camera.
     *
     * @param localFile Filename (including path) of the file to upload.
     * @param destinationFolder Path to the directory where the file will be stored (example: "/FlashFS/system/").
     * @param onCompletion Called from a background thread when import is completed (will only be invoked once).
     * @param onError Called from a background thread on any failure (will only be invoked once).
     * @param onProgress Called continuously from a background thread to report upload progress.
     * @param context User-provided data.
     * @remarks Existing files on the camera will be overwritten by this call.
     *
     */
    ACS_API void ACS_Importer_exportFile(ACS_Exporter* exporter, const char* localFile, const ACS_FolderReference* destinationFolder, ACS_OnCompletion onCompletion, ACS_OnError onError, ACS_OnProgress onProgress, ACS_CallbackContext context);

    /** @brief Export (upload) a file to camera.
     *
     * @param localFile The file to upload.
     * @param destinationFile Filename including path where the file will be stored (example: "/FlashFS/system/bootlogo_customer.bmp").
     * @param onCompletion Called from a background thread when import is completed (will only be invoked once).
     * @param onError Called from a background thread on any failure (will only be invoked once).
     * @param onProgress Called continuously from a background thread to report upload progress.
     * @param context User-provided data.
     * @remarks Existing files on the camera will be overwritten by this call.
     *
     */
    ACS_API void ACS_Importer_exportFileAs(ACS_Exporter* exporter, const char* localFile, const ACS_FileReference* destinationFile, ACS_OnCompletion onCompletion, ACS_OnError onError, ACS_OnProgress onProgress, ACS_CallbackContext context);

    /** @brief Immediately cancel all requests and active file transmissions.
     * @relatesalso ACS_Importer
     */
    ACS_API void ACS_Importer_cancelAll(ACS_Importer* importer);

    /** @brief Deletes a file on the camera
     * @param fileToDelete File (on the camera) to delete.
     * @param onCompletion Called when delete is completed.
     * @param onError Called on any failure.
     * @param context User-provided data.
     */
    ACS_API void ACS_Importer_deleteFile(ACS_Exporter* exporter, ACS_FileReference* fileToDelete, ACS_OnCompletion onCompletion, ACS_OnError onError, ACS_CallbackContext context);

    /** @brief Create a file reference object.
     *
     * @param location The @ref ACS_Location_ "root location".
     * @param path Relative path to @p location.
     * @relatesalso ACS_FileReference
     */
    ACS_API ACS_OWN(ACS_FileReference*, ACS_FileReference_free) ACS_FileReference_alloc(int location, const char* path);

    /** @brief Copy a file reference object.
     * @relatesalso ACS_FileReference
     */
    ACS_API ACS_OWN(ACS_FileReference*, ACS_FileReference_free) ACS_FileReference_copy(const ACS_FileReference* fileRef);

    /** @brief Destroy a file reference object.
     * @relatesalso ACS_FileReference
     */
    ACS_API void ACS_FileReference_free(const ACS_FileReference* fileRef);

    /** @brief Get @ref ACS_Location_ "base part" of the file path.
     * @relates ACS_FileReference
     */
    ACS_API int ACS_FileReference_getLocation(const ACS_FileReference* fileRef);

    /** @brief Get relative path to @ref ACS_FileReference_getLocation "location".
     *
     * Example: Both "image.jpg" and "/image.jpg" would refer to an image file in the folder of @ref ACS_FileReference_getLocation.
     * @note The path is expressed in a camera-compatible format using forward slashes `/` as the separator. "/image.jpg" would be
     *       a valid path even if Thermal SDK runs on a Windows system.
     * @relates ACS_FileReference
     */
    ACS_API const char* ACS_FileReference_getPath(const ACS_FileReference* fileRef);

    /** @brief Compare if two file references are equal.
    *
    * Two references are considered equal if their location and paths match.
    * @relates ACS_FileReference
    */
    ACS_API bool ACS_FileReference_equal(const ACS_FileReference* lhs, const ACS_FileReference* rhs);


    /** @brief Create a folder reference object.
     *
     * @param location The @ref ACS_Location_ "root location".
     * @param path Relative path to @p location.
     * @relatesalso ACS_FolderReference
     */
    ACS_API ACS_OWN(ACS_FolderReference*, ACS_FolderReference_free) ACS_FolderReference_alloc(int location, const char* path);

    /** @brief Copy a folder reference object.
     * @relatesalso ACS_FolderReference
     */
    ACS_API ACS_OWN(ACS_FolderReference*, ACS_FolderReference_free) ACS_FolderReference_copy(const ACS_FolderReference* folderRef);

    /** @brief Destroy a folder reference object.
     * @relatesalso ACS_FolderReference
     */
    ACS_API void ACS_FolderReference_free(const ACS_FolderReference* folderRef);

    /** @brief Get @ref ACS_Location_ "base part" of the folder path.
     * @relates ACS_FolderReference
     */
    ACS_API int ACS_FolderReference_getLocation(const ACS_FolderReference* folderRef);

    /** @brief Get relative path to @ref ACS_FolderReference_getLocation "location".
     *
     * Example: Both "" and "/" would refer to the folder of @ref ACS_FolderReference_getLocation.
     * @note The path is expressed in a camera-compatible format using forward slashes `/` as the separator. "/some-folder/" would be
     *       a valid path even if Thermal SDK runs on a Windows system.
     * @relates ACS_FolderReference
     */
    ACS_API const char* ACS_FolderReference_getPath(const ACS_FolderReference* folderRef);

    /** @brief Compare if two folder references are equal.
    *
    * Two references are considered equal if their location and paths match.
    * @relates ACS_FolderReference
    */
    ACS_API bool ACS_FolderReference_equal(const ACS_FolderReference* lhs, const ACS_FolderReference* rhs);


    /** @brief Get name of the folder, without the path (like Unix's basename).
    * @relates ACS_FolderInfo
    */
    ACS_API const char* ACS_FolderInfo_getName(const ACS_FolderInfo* folderInfo);

    /** @brief Get abstract path to the folder.
     * @relates ACS_FolderInfo
     */
    ACS_API ACS_BORROW_FROM(const ACS_FolderReference*, folderInfo) ACS_FolderInfo_getReference(const ACS_FolderInfo* folderInfo);

    /** @brief The time this folder was last changed.
     * @relates ACS_FolderInfo
     */
    ACS_API struct tm ACS_FolderInfo_getTime(const ACS_FolderInfo* folderInfo);

    /** @brief Get size of the list (i.e. file count).
     * @relates ACS_ListFileInfo
     */
    ACS_API size_t ACS_ListFileInfo_getSize(const ACS_ListFileInfo* list);

    /** @brief Get @ref ACS_FileInfo "file info" object at specified index.
     * @relates ACS_ListFileInfo
     */
    ACS_API const ACS_FileInfo* ACS_ListFileInfo_getFileInfo(const ACS_ListFileInfo* list, size_t index);

    /** @brief Get @ref ACS_FolderInfo "folder info" object at specified index.
    * @relates ACS_ListFileInfo
    */
    ACS_API const ACS_FolderInfo* ACS_ListFolderInfo_getFolderInfo(const ACS_ListFolderInfo* list, size_t index);

    /** @brief Get size of the list (i.e. folder count).
    * @relates ACS_ListFolderInfo
    */
    ACS_API size_t ACS_ListFolderInfo_getSize(const ACS_ListFolderInfo* list);

    /** @brief Get name of the file, without the path (like Unix's basename).
    * @relates ACS_FileInfo
    */
    ACS_API const char* ACS_FileInfo_getName(const ACS_FileInfo* info);

    /** @brief Get file size in bytes.
     * @relates ACS_FileInfo
     */
    ACS_API long long ACS_FileInfo_getSize(const ACS_FileInfo* info);

    /** @brief The time this file was last changed.
     * @relates ACS_FileInfo
     */
    ACS_API struct tm ACS_FileInfo_getTime(const ACS_FileInfo* info);

    /** @brief Get abstract path to the file.
     * @relates ACS_FileInfo
     */
    ACS_API ACS_BORROW_FROM(const ACS_FileReference*, fileInfo) ACS_FileInfo_getReference(const ACS_FileInfo* info);

    /** @brief Check if this file info object represents a directory.
     * @relates ACS_FileInfo
     */
    ACS_API bool ACS_FileInfo_isDirectory(const ACS_FileInfo* info);

#ifdef __cplusplus
}
#endif


#endif // ACS_IMPORT_H
