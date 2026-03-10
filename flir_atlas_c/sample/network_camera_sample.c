#include <math.h>
#include <acs/camera.h>
#include <acs/discovery.h>
#include <acs/thermal_image.h>
#include <acs/utility.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct DiscoveryContext
{
    bool futureAlreadySet;  // flag to avoid calling ACS_Future_setValue more than once
    ACS_Future* futureIdentity;  // thread-safe channel to pass discovered camera identity from background thread to main thread
};

ACS_Identity* discoverCamera(void);
void onCameraFound(const ACS_DiscoveredCamera* discoveredCamera, void* void_context);
void onDiscoveryError(ACS_CommunicationInterface cif, ACS_Error error, void* void_context);
void onCameraLost(const ACS_Identity* identity, void* void_context);
void onDiscoveryFinished(ACS_CommunicationInterface interface, void* void_context);
void onDisconnect(ACS_Error error, void* context);
void printCameraStatus(const ACS_RemoteControl* remoteControl);
void printAvailablePalettes(const ACS_RemoteControl* remoteControl);
ACS_ThermalImage* takeTemporarySnapshot(ACS_RemoteControl* remoteControl);
ACS_ThermalImage* takeSnapshot(ACS_RemoteControl* remoteControl, ACS_Camera* camera);
ACS_ThermalImage* openThermalImage(const char* path);
void printImageDiagnostics(const ACS_ThermalImage* image);
void focusDiagnostic(ACS_RemoteControl* remote);
void printFocusPosition(ACS_RemoteControl* remote);
void imagePropertiesDiagnostic(ACS_RemoteControl* remoteControl);
void updateThermalValueProperty(ACS_Property_ThermalValue* prop, double newValue, const char* propertyName);
void updateDoubleProperty(ACS_Property_Double* prop, double newValue, const char* propertyName);
void onCameraInformation(const ACS_Remote_CameraInformation* cameraInfo, void* context);
void onRequestError(ACS_Error error, void* context);
void onImportComplete(void* context);
void onImportError(ACS_Error error, void* context);
void onImportProgress(const ACS_FileReference* file, long long current, long long total, void* context);
void checkAcs(void);
void checkAcsError(ACS_Error error);
void convertThermalValueExample(void);
void logInvalidLoginIncident(void);

int main(int argc, char* argv[])
{
    int exitCode = EXIT_FAILURE;
    const char* ip = NULL;
  
    for (int i = 1; i < argc; i++)
    {
        char* pos = strchr(argv[i], '=');

        if (strstr(argv[i], "--ip") != NULL && pos != NULL)
            ip = pos + 1;
        else
            fprintf(stderr, "Unknown argument %s\n", argv[i]);
    }

    // Use IP given as commandline argument if available, otherwise find nearby cameras
    ACS_Identity* identity = (ip != NULL) ? ACS_Identity_fromIpAddress(ip) : discoverCamera();
    if (!identity)
    {
        fprintf(stderr, "Could not discover any camera\n");
        return exitCode;
    }

    // Connect to the first camera we found
    ACS_Camera* camera = ACS_Camera_alloc();
    checkAcs();

    // Use this to register with secure network cameras (e.g. A700)
    ACS_AuthenticationResponse response = ACS_Camera_authenticate(camera, identity, "./", "sample-app-cert", "network_sample_app", ACS_AUTHENTICATE_USE_DEFAULT_TIMEOUT);
    checkAcs();
    if (response.authenticationStatus != ACS_AuthenticationStatus_approved)
    {
        fprintf(stderr, "Unable to authenticate with camera - please check that the certificate is approved in the camera's UI\n");
    }


    ACS_Error error = ACS_Camera_connect(camera, identity, NULL, onDisconnect, NULL, NULL);
    if (ACS_getErrorCondition(error) == ACS_ERR_INVALID_LOGIN)
    {
        logInvalidLoginIncident();
    }
    checkAcsError(error);

    ACS_Identity_free(identity);

    // Get the camera's remote control interface so we can perform camera actions and query information
    ACS_RemoteControl* remoteControl = ACS_Camera_getRemoteControl(camera);
    if (!remoteControl)
    {
        fprintf(stderr, "Camera does not support remote control\n");
        goto close_camera;
    }

    convertThermalValueExample();

    // Print some basic information
    printCameraStatus(remoteControl);

    // Print available remote camera palettes
    printAvailablePalettes(remoteControl);

    // Perform a one-shot autofocus
    printf("Performing autofocus...");
    ACS_Remote_Focus_autofocus_executeSync(remoteControl);
    checkAcs();
    printf("Done\n");

    // Attempt to take a regular snapshot (which will work on cameras with an image storage)
    // If taking a regular snapshot is not supported, fallback to using a storage-less snapshot
    ACS_ThermalImage* importedImage = takeSnapshot(remoteControl, camera);
    if (!importedImage)
        importedImage = takeTemporarySnapshot(remoteControl);

    // The temperature unit of a thermal image can be inspected and changed
    if (ACS_ThermalImage_getTemperatureUnit(importedImage) != ACS_TemperatureUnit_celsius)
    {
        ACS_ThermalImage_setTemperatureUnit(importedImage, ACS_TemperatureUnit_celsius);
    }

    printImageDiagnostics(importedImage);
    ACS_ThermalImage_free(importedImage);

    // Control the camera's focus engine
    focusDiagnostic(remoteControl);

    // Read and modify camera's image parameters (e.g. emissivity)
    imagePropertiesDiagnostic(remoteControl);

    // Clean up and exit
    exitCode = 0;
close_camera:
    ACS_Camera_free(camera);
    return exitCode;
}

ACS_Identity* discoverCamera(void)
{
    ACS_Discovery* discovery = ACS_Discovery_alloc();
    checkAcs();

    // Start scanning for nearby cameras
    struct DiscoveryContext context = {
        .futureAlreadySet = false,
        .futureIdentity = ACS_Future_alloc()
    };
    checkAcs();
    ACS_Discovery_scan(discovery, ACS_CommunicationInterface_network, onCameraFound, onDiscoveryError, onCameraLost, onDiscoveryFinished, &context);
    checkAcs();

    // Block until a camera is discovered
    ACS_Identity* identity = ACS_Future_get(context.futureIdentity);
    checkAcs();
    ACS_Future_free(context.futureIdentity);
    ACS_Discovery_free(discovery);
    return identity;
}

void onCameraFound(const ACS_DiscoveredCamera* discoveredCamera, void* void_context)
{
    struct DiscoveryContext* context = void_context;
    const ACS_Identity* identity = ACS_DiscoveredCamera_getIdentity(discoveredCamera);
    if (context->futureAlreadySet)
    {
        printf("(ignored) Camera \"%s\" found at: %s\n", ACS_DiscoveredCamera_getDisplayName(discoveredCamera), ACS_Identity_getIpAddress(identity));
        return;
    }

    // Pass discovered camera identity to main thread (needs to be copied since `discoveredCamera` will go out of scope)
    printf("Camera \"%s\" found at: %s\n", ACS_DiscoveredCamera_getDisplayName(discoveredCamera), ACS_Identity_getIpAddress(identity));
    context->futureAlreadySet = true;
    ACS_Future_setValue(context->futureIdentity, ACS_Identity_copy(identity));
}

void onCameraLost(const ACS_Identity* identity, void* void_context)
{
    (void)void_context;
    printf("camera lost %s\n", ACS_Identity_getDeviceId(identity));
}

void onDiscoveryFinished(ACS_CommunicationInterface interface, void* void_context)
{
    (void)interface;
    (void)void_context;
    printf("discover finished\n");
}

void onDiscoveryError(ACS_CommunicationInterface cif, ACS_Error error, void* void_context)
{
    // Pass any error to the main thread
    struct DiscoveryContext* context = void_context;
    fprintf(stderr, "Discovery error on interface %u\n", cif);
    context->futureAlreadySet = true;
    ACS_Future_setError(context->futureIdentity, error);
}

// This is just a conceptual example
void attemptReconnect(void)
{
}

void onDisconnect(ACS_Error error, void* context)
{
    // Handle unexpected camera disconnection
    (void)context;
    fprintf(stderr, "Lost connection to camera\n");

    if (ACS_getErrorCondition(error) == ACS_ERR_CONNECTION_TIME_OUT)
    {
        // Conceptual example - does nothing
        attemptReconnect();
    }

    checkAcsError(error);
}

void printCameraStatus(const ACS_RemoteControl* remoteControl)
{
    // Fetch camera information asynchronously
    ACS_Future* cameraInfoFuture = ACS_Future_alloc();
    checkAcs();
    ACS_Property_CameraInformation_get(ACS_Remote_cameraInformation(remoteControl), onCameraInformation, onRequestError, cameraInfoFuture);
    checkAcs();

    // Wait until async request finishes (not really necessary)
    ACS_Future_get(cameraInfoFuture);
    checkAcs();
    ACS_Future_free(cameraInfoFuture);
}

void printAvailablePalettes(const ACS_RemoteControl* remoteControl)
{
    const ACS_Property_ListRemotePalette* propPalettes = ACS_Remote_Palette_availablePalettes(remoteControl);
    checkAcs();

    const ACS_ListRemotePalette* palettes = ACS_Property_ListRemotePalette_getSync(propPalettes);
    int paletteCount = (int) ACS_ListRemotePalette_size(palettes);
    for (int i = 0; i < paletteCount; i++)
    {
        ACS_RemotePalette* palette = ACS_ListRemotePalette_item((ACS_ListRemotePalette*) palettes, i);
        checkAcs();
        printf("Palette %d: , %s\n", i, ACS_RemotePalette_getName(palette));
    }
}

ACS_ThermalImage* takeSnapshot(ACS_RemoteControl* remoteControl, ACS_Camera* camera)
{
    const char* importFilePath = "./latest_snapshot.jpg";
    const bool doOverwrite = true;

    ACS_Importer* importer = ACS_Camera_getImporter(camera);

    ACS_StoredImage* image = ACS_Remote_Storage_snapshot_executeSync(remoteControl);
    if (ACS_getErrorCondition(ACS_getLastError()) == ACS_ERR_MISSING_STORAGE)
        return NULL;

    // Depending on the camera, there may also be a visual image available.
    // If it exists, a reference to this visual image can be retrieved using ACS_StoredImage_getVisualImage.
    // In this example the visual image is ignored.
    const ACS_FileReference* thermalImageRef = ACS_StoredImage_getThermalImage(image);

    ACS_Future* fileImportFuture = ACS_Future_alloc();
    checkAcs();
    ACS_Importer_importFileAs(importer, thermalImageRef, importFilePath, doOverwrite, onImportComplete, onImportError, onImportProgress, fileImportFuture);
    checkAcs();

    ACS_Future_get(fileImportFuture);
    checkAcs();

    // Open the snapshot image
    ACS_ThermalImage* thermalImage = openThermalImage(importFilePath);

    printf("Done - imported as \"%s\"\n", importFilePath);

    ACS_Future_free(fileImportFuture);
    ACS_StoredImage_free(image);
    return thermalImage;
}

ACS_ThermalImage* takeTemporarySnapshot(ACS_RemoteControl* remoteControl)
{
    // Set up snapshot file format
    ACS_Property_Int_setSync(ACS_Remote_Storage_fileFormat(remoteControl), ACS_Storage_FileFormat_jpeg);
    checkAcs();

    // Perform the snapshot
    printf("Taking a snapshot...\n");
    ACS_StoredLocalImage* localImage = ACS_Remote_Storage_snapshotToLocalFile_executeSync(remoteControl, "./latest_snapshot.jpg", NULL);
    checkAcs();

    // Open the snapshot image
    ACS_ThermalImage* importedImage = openThermalImage(ACS_StoredLocalImage_getThermalImage(localImage));

    // Clean up and return
    printf("Done - imported as \"%s\"\n", ACS_StoredLocalImage_getThermalImage(localImage));
    ACS_StoredLocalImage_free(localImage);
    return importedImage;
}

ACS_ThermalImage* openThermalImage(const char* path)
{
    ACS_ThermalImage* thermalImage = ACS_ThermalImage_alloc();
    checkAcs();
    ACS_NativeString* fileName = ACS_NativeString_createFrom(path);
    ACS_ThermalImage_openFromFile(thermalImage, ACS_NativeString_get(fileName));
    ACS_NativeString_free(fileName);
    checkAcs();

    return thermalImage;
}

void printImageDiagnostics(const ACS_ThermalImage* image)
{
    printf("Image resolution: %dx%d\n", ACS_ThermalImage_getWidth(image), ACS_ThermalImage_getHeight(image));
    ACS_String* formatted = ACS_ThermalValue_format(ACS_ThermalImage_getValueAt(image, 0, 0));
    checkAcs();
    printf("First pixel temperature: %s\n", ACS_String_get(formatted));
    ACS_String_free(formatted);
}

void focusDiagnostic(ACS_RemoteControl* remote)
{
    printf("Focus distance diagnostic start:\n");
    printFocusPosition(remote);

    // Start and stop "focus far"
    ACS_Remote_Focus_distanceStartIncrease_executeSync(remote, ACS_FOCUS_SPEED_SLOW);
    ACS_Remote_Focus_distanceStop_executeSync(remote);
    printFocusPosition(remote);

    // Start and stop "focus near"
    ACS_Remote_Focus_distanceStartDecrease_executeSync(remote, ACS_FOCUS_SPEED_SLOW);
    ACS_Remote_Focus_distanceStop_executeSync(remote);
    printFocusPosition(remote);

    // Set focus to 1 metre
    ACS_Property_Double_setSync(ACS_Remote_Focus_distance(remote), 1.0);
    printFocusPosition(remote);

    // Set focus position using camera-specific units
    ACS_Property_Int* positionProp = ACS_Remote_Focus_position(remote);
    const int focusPositionTargetValue = 2500;
    const int focusPositionEpsilon = 10;
    ACS_Property_Int_setSync(positionProp, focusPositionTargetValue);

    // Poll until we've reached expected focus position
    int focusPosition = -1;
    while ((focusPosition < focusPositionTargetValue - focusPositionEpsilon) || (focusPosition > focusPositionTargetValue + focusPositionEpsilon))
    {
        focusPosition = ACS_Property_Int_getSync(positionProp);
        printf("Focus position: %d\n", focusPosition);
    }

    printf("Focus distance diagnostic end\n");
}

void printFocusPosition(ACS_RemoteControl* remote)
{
    const ACS_Property_Double* distanceProp = ACS_Remote_Focus_distance(remote);
    const ACS_Property_Int* positionProp = ACS_Remote_Focus_position(remote);
    double focusDistance = ACS_Property_Double_getSync(distanceProp);
    int focusPosition = ACS_Property_Int_getSync(positionProp);
    printf("Focus distance: %g m, position: %d\n", focusDistance, focusPosition);
}

void imagePropertiesDiagnostic(ACS_RemoteControl* remoteControl)
{
    ACS_Property_Double* distance = ACS_Remote_ThermalParameters_objectDistance(remoteControl);
    ACS_Property_Double* emissivity = ACS_Remote_ThermalParameters_objectEmissivity(remoteControl);
    ACS_Property_ThermalValue* reflectedTemperature = ACS_Remote_ThermalParameters_objectReflectedTemperature(remoteControl);
    ACS_Property_Double* relativeHumidity = ACS_Remote_ThermalParameters_relativeHumidity(remoteControl);
    ACS_Property_ThermalValue* atmosphericTemperature = ACS_Remote_ThermalParameters_atmosphericTemperature(remoteControl);
    ACS_Property_Double* atmosphericTransmission = ACS_Remote_ThermalParameters_atmosphericTransmission(remoteControl);
    ACS_Property_ThermalValue* externalOpticsTemperature = ACS_Remote_ThermalParameters_externalOpticsTemperature(remoteControl);
    ACS_Property_Double* externalOpticsTransmission = ACS_Remote_ThermalParameters_externalOpticsTransmission(remoteControl);

    updateDoubleProperty(distance, 5.0, "distance");
    updateDoubleProperty(emissivity, 0.95, "emissivity");
    updateThermalValueProperty(reflectedTemperature, 293.15, "reflectedTemperature");
    updateDoubleProperty(relativeHumidity, 0.5, "relativeHumidity");
    updateThermalValueProperty(atmosphericTemperature, 293.15, "atmosphericTemperature");
    updateDoubleProperty(atmosphericTransmission, 0.0, "atmosphericTransmission");
    updateThermalValueProperty(externalOpticsTemperature, 293.15, "externalOpticsTemperature");
    updateDoubleProperty(externalOpticsTransmission, 1, "externalOpticsTransmission");
}

void onCameraInformation(const ACS_Remote_CameraInformation* cameraInfo, void* context)
{
    // Print camera info
    printf("Received camera name: %s, resolution: %dx%d\n",
        ACS_Remote_CameraInformation_getName(cameraInfo),
        ACS_Remote_CameraInformation_getResolutionWidth(cameraInfo),
        ACS_Remote_CameraInformation_getResolutionHeight(cameraInfo));

    // Notify main thread
    if (context)
        ACS_Future_setValue(context, NULL);
}

void onRequestError(ACS_Error error, void* context)
{
    // Print request failure reason
    ACS_String* errorString = ACS_getErrorMessage(error);
    fprintf(stderr, "Request failed: %s, details: %s\n", ACS_String_get(errorString), ACS_getLastErrorMessage());
    ACS_String_free(errorString);

    // Notify main thread
    if (context)
        ACS_Future_setError(context, error);
}

void onImportComplete(void* context)
{
    // Notify main thread
    if (context)
        ACS_Future_setValue(context, NULL);
}

void onImportError(ACS_Error error, void* context)
{
    // Notify main thread
    if (context)
        ACS_Future_setError(context, error);
}

void onImportProgress(const ACS_FileReference* file, long long current, long long total, void* context)
{
    (void)context; // ignore context
    printf("Importing file %s,  %lld of %lld bytes \r", ACS_FileReference_getPath(file), current, total);
}

void updateDoubleProperty(ACS_Property_Double* prop, double newValue, const char* propertyName)
{
    const double epsilon = 0.00001;

    // Update the camera property
    ACS_Property_Double_setSync(prop, newValue);
    checkAcs();

    // Read immediately after write
    const double valueAfterSet = ACS_Property_Double_getSync(prop);
    checkAcs();

    // Check that the property was correctly updated
    printf("Test: %s : %s\n", propertyName, fabs(valueAfterSet - newValue) < epsilon ? "success" : "failure");
}

void updateThermalValueProperty(ACS_Property_ThermalValue* prop, double newValue, const char* propertyName)
{
    const double epsilon = 0.00001;
    const int unit = ACS_TemperatureUnit_kelvin;

    // Update the camera property
    ACS_ThermalValue tv = { newValue, unit, ACS_ThermalValueState_ok };
    ACS_Property_ThermalValue_setSync(prop, tv);
    checkAcs();

    // Read immediately after write
    const ACS_ThermalValue valueAfterSet = ACS_Property_ThermalValue_getSync(prop);
    checkAcs();

    // Check that the property was correctly updated
    if (valueAfterSet.unit == unit)
        printf("Test: %s : %s\n", propertyName, fabs(valueAfterSet.value - newValue) < epsilon ? "success" : "failure");
    else
        printf("Unable to compare thermal values, need to do unit conversion first\n");
}

void convertThermalValueExample(void)
{
    ACS_ThermalValue tv = {0.0, ACS_TemperatureUnit_celsius, ACS_ThermalValueState_ok};
    ACS_ThermalValue tvk = ACS_ThermalValue_asKelvin(tv);
    ACS_ThermalValue tvf = ACS_ThermalValue_asFahrenheit(tv);
    
    printf("0.0 C is equivalent to %g K, %gF\n", tvk.value, tvf.value);
}

void logInvalidLoginIncident(void)
{
    printf("Invalid login incident logged (This is just an example)\n");
}

void checkAcs(void)
{
    // Check if last call to a ACS_* function failed
    checkAcsError(ACS_getLastError());
}

void checkAcsError(ACS_Error error)
{
    // Non-zero means error, zero is success
    if (error.code)
    {
        // Print error message plus extra details
        ACS_String* errorString = ACS_getErrorMessage(error);
        fprintf(stderr, "ACS failed: %s, details: %s\n", ACS_String_get(errorString), ACS_getLastErrorMessage());
        ACS_String_free(errorString);

        // Exit program on error. Do NOT do this in production code, e.g. throw an exception if C++ is used would be more appropriate.
        exit(EXIT_FAILURE);
    }
}
