#include <acs/acs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct programSettings
{
    bool printStats;
    bool printMeasurements;
    bool recordFromStream;
    bool printCameraInfo;
    bool enableCompression;
    unsigned int frameCount;
};

ACS_Identity* discoverCamera(int commInterface);
void onCameraFound(const ACS_DiscoveredCamera*, void*);
void onDiscoveryError(ACS_CommunicationInterface, ACS_Error, void*);
void onDisconnected(ACS_Error, void*);
void onError(ACS_Error, void*);
void onImageReceived(unsigned long*);
void withThermalImageHelper(ACS_ThermalImage*, void*);
ACS_Stream* findVisualStream(ACS_Camera* camera);
ACS_Stream* findThermalStream(ACS_Camera* camera);
void checkAcs(bool );
void checkAcsError(ACS_Error, bool);
struct programSettings settings;
void printImageCameraInformation(const ACS_Image_CameraInformation* cameraInfo);
void printStreamInformation(ACS_Camera* camera);

struct DiscoveryContext
{
    bool futureAlreadySet;  // flag to avoid calling ACS_Future_setValue more than once
    ACS_Future* futureIdentity;  // thread-safe channel to pass discovered camera identity from background thread to main thread
};

void initSettings(void)
{
    settings.printStats = false;
    settings.printMeasurements = false;
    settings.recordFromStream = true;
    settings.printCameraInfo = false;
    settings.enableCompression = false;
    settings.frameCount = 0;
}

typedef struct StreamingCallbackContext_
{
    ACS_ThermalSequenceRecorder* recorder;
} StreamingCallbackContext;

void printUsage(const char* cmd)
{
    printf("usage: %s [options]\n", cmd);
    printf("    option: --help : Shows this help\n");
    printf("\n");
    printf("Communication Options (default camera is emulator):\n");
    printf("    option: --usb          : Scan for a UVC camera\n");
    printf("    option: --network      : Scan for a network camera\n");
    printf("    option: --ip=<address> : Connect to a network camera at the specified address\n");
    printf("    option: --no-auth      : Skip authentication for network streams\n");
    printf("\n");
    printf("Stream Options (default stream is thermal (mono16)):\n");
    printf("    option: --colorized              : Select the colorized stream for display\n");
    printf("    option: --record=<filename>      : Record stream to the specified filename\n");
    printf("    option: --record-from-colorizer  : Record from the colorizer rather than directly from the stream\n");
    printf("    option: --compress               : Enable compression for the recording\n");
    printf("    option: --frame-count=<frames>   : Specify number of frames to receive before stopping (default 0: run till stopped)\n");
    printf("\n");
    printf("Misc Options:\n");
    printf("    option: --camInfo         : Print camera information\n");
    printf("    option: --stats           : Get and display image statistics per each frame\n");
    printf("    option: --measurements    : Set some spots and display their measurements\n");
    printf("    option: --printStreamInfo : Print info about available streams from camera\n");
    printf("\n");
    printf("Log Options:\n");
    printf("    option: --no-log    : Turn off logging (default)\n");
    printf("    option: --error-log : Only show errors\n");
    printf("    option: --warn-log  : Show errors and warnings\n");
    printf("    option: --info-log  : Warn plus informative logs\n");
    printf("    option: --debug-log : Internal only. Includes info level\n");
    printf("    option: --trace-log : Internal only. Includes debug level\n");
    printf("\n");
    exit(0);
}

int main(int argc, char* argv[])
{
    int exitCode = EXIT_FAILURE;
    const char* ip = NULL;
    const char* recordingFilename = NULL;
    bool colorizedStreaming = false;

    bool thermalRecording = false;
    bool authenticateWithTheCamera = true;
    StreamingCallbackContext streamContext;
    streamContext.recorder = NULL;
    unsigned long callbacksReceived = 0;
    int commInterface = ACS_CommunicationInterface_emulator;

    bool printStreamInfo = false;
    bool freeRun = true;

    initSettings();

    ACS_Logger_setLevel(ACS_LogLevel_off);

    for (int i = 1; i < argc; i++)
    {
        char* pos = strchr(argv[i], '=');

        if (strstr(argv[i], "--help") != NULL)
        {
            printUsage(argv[0]);
        }
        else if (strstr(argv[i], "--ip") != NULL && pos != NULL)
            ip = pos + 1;
        else if (strstr(argv[i], "--colorized") != NULL)
            colorizedStreaming = true;
        else if (strstr(argv[i], "--frame-count") != NULL && pos != NULL)
        {
            if (atoi(pos + 1) <= 0)
            {
                fprintf(stderr, "Frame count must be > 0!\n");
                printUsage(argv[0]);
            }
            else
            {
                settings.frameCount = atoi(pos + 1);
                freeRun = false;
            }
        }
        else if (strstr(argv[i], "--record-from-colorizer") != NULL)
            settings.recordFromStream = false;
        else if (strstr(argv[i], "--record") != NULL && pos != NULL)
        {
            thermalRecording = true;
            recordingFilename = pos + 1;
        }
        else if (strstr(argv[i], "--no-auth") != NULL)
            authenticateWithTheCamera = false;
        else if (strstr(argv[i], "--camInfo") != NULL)
            settings.printCameraInfo = true;
        else if (strstr(argv[i], "--stats") != NULL)
            settings.printStats = true;
        else if (strstr(argv[i], "--measurements") != NULL)
            settings.printMeasurements = true;
        else if (strstr(argv[i], "--usb") != NULL)
            commInterface = ACS_CommunicationInterface_usb;
        else if (strstr(argv[i], "--network") != NULL)
            commInterface = ACS_CommunicationInterface_network;
        else if (strstr(argv[i], "--printStreamInfo") != NULL)
            printStreamInfo = true;
        else if (strstr(argv[i], "--no-log") != NULL)
            ACS_Logger_setLevel(ACS_LogLevel_off);
        else if (strstr(argv[i], "--error-log") != NULL)
            ACS_Logger_setLevel(ACS_LogLevel_error);
        else if (strstr(argv[i], "--warn-log") != NULL)
            ACS_Logger_setLevel(ACS_LogLevel_warn);
        else if (strstr(argv[i], "--info-log") != NULL)
            ACS_Logger_setLevel(ACS_LogLevel_info);
        else if (strstr(argv[i], "--debug-log") != NULL)
            ACS_Logger_setLevel(ACS_LogLevel_debug);
        else if (strstr(argv[i], "--trace-log") != NULL)
            ACS_Logger_setLevel(ACS_LogLevel_trace);
        else if (strstr(argv[i], "--compress") != NULL)
            settings.enableCompression = true;
        else
        {
            fprintf(stderr, "Unknown argument %s\n", argv[i]);
            printUsage(argv[0]);
        }
    }

    if (colorizedStreaming && thermalRecording)
        fprintf(stderr, "WARNING! Thermalrecorder cannot record visual stream. Stream will not be recorded.");

    // Use IP given as commandline argument if available, otherwise find nearby cameras
    ACS_Identity* identity = (ip != NULL) ? ACS_Identity_fromIpAddress(ip) : discoverCamera(commInterface);
    if (!identity)
    {
        fprintf(stderr, "Could not discover any camera\n");
        return exitCode;
    }

    ACS_Camera* camera = ACS_Camera_alloc();
    checkAcs(true);

    // Use this to register with secure network cameras (e.g. A700)
    if (authenticateWithTheCamera)
    {
        ACS_AuthenticationResponse response = ACS_Camera_authenticate(camera, identity, "./", "stream-sample-app", "stream_sample_app", ACS_AUTHENTICATE_USE_DEFAULT_TIMEOUT);
        checkAcs(true);
        if (response.authenticationStatus != ACS_AuthenticationStatus_approved)
        {
            fprintf(stderr, "Unable to authenticate with camera - please check that the certificate is approved in the camera's UI\n");
        }
    }

    // Connect to the first camera we found
    ACS_Error error = ACS_Camera_connect(camera, identity, NULL, onError, NULL, NULL);
    checkAcsError(error, true);
    ACS_Identity_free(identity);

    // 
    if (printStreamInfo)
    {
        printStreamInformation(camera);
        exit(0);
    }

    // Get the camera's stream interface
    ACS_Stream* stream;
    if(colorizedStreaming)
        stream = findVisualStream(camera);
    else
        stream = findThermalStream(camera);

    if (!stream)
    {
        if (colorizedStreaming)
            fprintf(stderr, "Camera does not support visual streaming\n");
        else
            fprintf(stderr, "Camera does not support thermal streaming\n");

        goto close_camera;
    }

    // Set up the frame grabber
    ACS_Streamer* streamer;
    ACS_ThermalStreamer* thermalStreamer = NULL;
    if (colorizedStreaming)
    {
        streamer = ACS_VisualStreamer_asStreamer(ACS_VisualStreamer_alloc(stream));
    }
    else
    {
        thermalStreamer = ACS_ThermalStreamer_alloc(stream);
        streamer = ACS_ThermalStreamer_asStreamer(thermalStreamer);

        if (thermalRecording)
        {
            streamContext.recorder = ACS_ThermalSequenceRecorder_alloc();
            ACS_NativeString* fileName = ACS_NativeString_createFrom(recordingFilename);
            ACS_ThermalSequenceRecorder_Settings_setEnableCompression(streamContext.recorder, settings.enableCompression);
            ACS_ThermalSequenceRecorder_start(streamContext.recorder, ACS_NativeString_get(fileName));
            ACS_NativeString_free(fileName);

            if (settings.recordFromStream)
            {
                ACS_Stream_attachRecorder(stream, streamContext.recorder);
                checkAcs(true); // Not all streams support recording (e.g. USB stream does (Windows only), RTSP stream doesn't yet)
            }
        }
    }

    checkAcs(true);
    ACS_Renderer* renderer = ACS_Streamer_asRenderer(streamer);
    ACS_Renderer_setOutputColorSpace(renderer, ACS_ColorSpaceType_rgb);
    checkAcs(true);

    // Start the stream! This involves network requests to the camera's stream server
    ACS_Stream_start(stream, (ACS_OnImageReceived)onImageReceived, onError, (ACS_CallbackContext) { .context = &callbacksReceived });
    checkAcs(true);

    printf("Stream is up and running\n");

    // Create a window and run the render loop
    ACS_DebugImageWindow* window = ACS_DebugImageWindow_alloc("C stream sample");
    unsigned long renderFrame = 0;
    while (ACS_DebugImageWindow_poll(window) && (freeRun || callbacksReceived < settings.frameCount))
    {
        if (callbacksReceived > renderFrame)
        {
            renderFrame = callbacksReceived;
            // printf("Rendering frame nr: %lu\n", renderFrame);
        }
        else
        {
            // We don't have a new frame yet, so just wait for the next one
            continue;
        }

        // Poll image from camera
        ACS_Renderer_update(renderer);
        checkAcs(false);
        const ACS_ImageBuffer* image = ACS_Renderer_getImage(renderer);

        // Skip if no valid framedata.
        if (!image)   
            continue;

        if (!colorizedStreaming)
        {
            // Set palette preset, do various prints (if enabled), and record thermal image (if not recording from stream):
            ACS_ThermalStreamer_withThermalImage(thermalStreamer, withThermalImageHelper, &streamContext);
        }
        
        // Display received image on screen
        ACS_DebugImageWindow_update(window, image);
        checkAcs(false);
    }
    checkAcs(true);

    if (thermalRecording)
    {
        ACS_ThermalSequenceRecorder_stop(streamContext.recorder);
        printf("Recorded %zd frames\n", ACS_ThermalSequenceRecorder_getFrameCounter(streamContext.recorder));
        printf("Lost %zd frames\n", ACS_ThermalSequenceRecorder_getLostFramesCounter(streamContext.recorder));
        printf("Recording saved to %s\n", recordingFilename);
        ACS_ThermalSequenceRecorder_free(streamContext.recorder);

        if (settings.recordFromStream)
        {
            ACS_Stream_detachRecorder(stream);
        }

        streamContext.recorder = NULL;
    }

    printf("Stopping after %lu frames\n", callbacksReceived);

    ACS_DebugImageWindow_free(window);
    ACS_Streamer_free(streamer);

    // Stop stream and clean up
    ACS_Stream_stop(stream);
    checkAcs(true);

    // Clean up and exit
    exitCode = 0;
close_camera:
    ACS_Camera_free(camera);
    return exitCode;
}

ACS_Identity* discoverCamera(int commInterface)
{
    ACS_Discovery* discovery = ACS_Discovery_alloc();
    checkAcs(true);

    // Start scanning for nearby cameras
    struct DiscoveryContext context = {
        .futureAlreadySet = false,
        .futureIdentity = ACS_Future_alloc()
    };
    checkAcs(true);

    printf("Scanning for cameras\n");
    ACS_Discovery_scan(discovery, commInterface, onCameraFound, onDiscoveryError, NULL, NULL, &context);
    checkAcs(true);

    // Block until a camera is discovered
    ACS_Identity* identity = ACS_Future_get(context.futureIdentity);
    checkAcs(true);
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
        printf("(ignored) Camera \"%s\" found", ACS_DiscoveredCamera_getDisplayName(discoveredCamera));
        if (ACS_Identity_getIpAddress(identity))
            printf(" at: %s\n", ACS_Identity_getIpAddress(identity));
        else
            printf("\n");
        return;
    }

    // Pass discovered camera identity to main thread (needs to be copied since `discoveredCamera` will go out of scope)
    printf("Camera \"%s\" found", ACS_DiscoveredCamera_getDisplayName(discoveredCamera));
    if (ACS_Identity_getIpAddress(identity))
        printf(" at: %s\n", ACS_Identity_getIpAddress(identity));
    else
        printf("\n");
    context->futureAlreadySet = true;
    ACS_Future_setValue(context->futureIdentity, ACS_Identity_copy(identity));
}

void onDiscoveryError(ACS_CommunicationInterface cif, ACS_Error error, void* void_context)
{
    // Pass any error to the main thread
    struct DiscoveryContext* context = void_context;
    fprintf(stderr, "Discovery error on interface %u\n", cif);
    context->futureAlreadySet = true;
    ACS_Future_setError(context->futureIdentity, error);
}

void onDisconnected(ACS_Error error, void* context)
{
    // Handle unexpected camera disconnection
    (void)context;
    fprintf(stderr, "Lost connection to camera\n");
    checkAcsError(error, true);
}

void onError(ACS_Error error, void* context)
{
    // Handle camera stream error
    (void)context;
    if (ACS_getErrorCondition(error) != ACS_ERR_NUC_IN_PROGRESS)
    {
        checkAcsError(error, true);
    }
}

void onImageReceived(unsigned long* counter)
{
    (*counter)++;
}

void withThermalImageHelper(ACS_ThermalImage* thermalImage, void* context)
{
    if (thermalImage)
    {
        StreamingCallbackContext* streamContext = context;
        ACS_ThermalImage_setPalettePreset(thermalImage, ACS_PalettePreset_iron);
        if (streamContext->recorder && !settings.recordFromStream)
        {
            ACS_ThermalSequenceRecorder_addImage(streamContext->recorder, thermalImage);
        }

        if (settings.printCameraInfo)
        {
            ACS_Image_CameraInformation* camInfo = ACS_ThermalImage_getCameraInformation(thermalImage);
            if (camInfo)
            {
                printImageCameraInformation(camInfo);
            }
            // Print once
            settings.printCameraInfo = false;
        }

        if (settings.printStats)
        {
            //auto stats = image.getStatistics().value();
            ACS_ImageStatistics* stats = ACS_ThermalImage_getStatistics(thermalImage);
            if (stats)
            {
                printf("Stats: avg=%lf, min=%lf, max=%lf\n", ACS_ImageStatistics_getAverage(stats).value,
                    ACS_ImageStatistics_getMin(stats).value,
                    ACS_ImageStatistics_getMax(stats).value);
                ACS_Point coldSpot = ACS_ImageStatistics_getColdSpot(stats);
                ACS_Point hotSpot = ACS_ImageStatistics_getHotSpot(stats);
                printf("coldSpot(x,y)=%d,%d, hotSpot(x,y)=%d,%d\n", coldSpot.x, coldSpot.y, hotSpot.x, hotSpot.y);
            }
            else
            {
                printf("Image statistics unavailable\n");
            }
        }
        
        if (settings.printMeasurements)
        {
            ACS_Measurements* measurements = ACS_ThermalImage_getMeasurements(thermalImage);
            if (measurements)
            {
                ACS_ListMeasurementSpot* spotList = ACS_Measurements_getAllSpots(measurements);
                size_t numSpots = ACS_ListMeasurementSpot_size(spotList);
                if (numSpots < 3)
                {
                    int width = ACS_ThermalImage_getWidth(thermalImage);
                    int height = ACS_ThermalImage_getHeight(thermalImage);
                    ACS_Measurements_addSpot(measurements, width / 3, height / 3);
                    ACS_Measurements_addSpot(measurements, width / 2, height / 2);
                    ACS_Measurements_addSpot(measurements, width * 2/3, height * 2/3);
                }
                for (size_t i = 0; i < numSpots; i++)
                {
                    ACS_MeasurementSpot* item = ACS_ListMeasurementSpot_item(spotList, i);
                    printf("Spot: id=%zd, pos(x,y)=%d,%d, value=%lf\n", \
                        i, ACS_MeasurementSpot_getPosition(item).x, ACS_MeasurementSpot_getPosition(item).y, \
                        ACS_MeasurementSpot_getValue(item).value);
                }
            }
            else
            {
                printf("Image measurements unavailable\n");
            }
        }
    }
}

ACS_Stream* findVisualStream(ACS_Camera* camera)
{
    for (size_t i = 0; i < ACS_Camera_getStreamCount(camera); ++i)
    {
        ACS_Stream* stream = ACS_Camera_getStream(camera, i);
        if (!ACS_Stream_isThermal(stream))
            return stream;
    }
    return NULL;
}

ACS_Stream* findThermalStream(ACS_Camera* camera)
{
    for (size_t i = 0; i < ACS_Camera_getStreamCount(camera); ++i)
    {
        ACS_Stream* stream = ACS_Camera_getStream(camera, i);
        if (ACS_Stream_isThermal(stream))
            return stream;
    }
    return NULL;
}

void checkAcs(bool exitOnError)
{
    // Check if last call to a ACS_* function failed
    checkAcsError(ACS_getLastError(), exitOnError);
}

void checkAcsError(ACS_Error error, bool exitOnError)
{
    // Non-zero means error, zero is success
    if (error.code)
    {
        // Print error message plus extra details
        fprintf(stderr, "ACS Error Code: %d\n", ACS_getErrorCondition(error));
        ACS_String* errorString = ACS_getErrorMessage(error);
        fprintf(stderr, "ACS Error String: %s\n", ACS_String_get(errorString));
        ACS_String_free(errorString);

        if (exitOnError)
            exit(EXIT_FAILURE);
    }
}

void printImageCameraInformation(const ACS_Image_CameraInformation* cameraInfo)
{
    if (cameraInfo)
    {
        printf("Model Name: %s\n", ACS_Image_CameraInformation_getModelName(cameraInfo));
        printf("Filter: %s\n", ACS_Image_CameraInformation_getFilter(cameraInfo));
        printf("Lens: %s\n", ACS_Image_CameraInformation_getLens(cameraInfo));
        printf("Serial Number: %s\n", ACS_Image_CameraInformation_getSerialNumber(cameraInfo));
        printf("Program version: %s\n", ACS_Image_CameraInformation_getProgramVersion(cameraInfo));
        printf("Article number: %s\n", ACS_Image_CameraInformation_getArticleNumber(cameraInfo));
        printf("Calibration title: %s\n", ACS_Image_CameraInformation_getCalibrationTitle(cameraInfo));
        printf("Lens serial number: %s\n", ACS_Image_CameraInformation_getLensSerialNumber(cameraInfo));
        printf("Arc file version: %s\n", ACS_Image_CameraInformation_getArcFileVersion(cameraInfo));
        printf("Arc date and time: %s\n", ACS_Image_CameraInformation_getArcDateTime(cameraInfo));
        printf("Arc signature: %s\n", ACS_Image_CameraInformation_getArcSignature(cameraInfo));
        printf("Country code: %s\n", ACS_Image_CameraInformation_getCountryCode(cameraInfo));
        printf("RangeMin: %.2f\n", ACS_Image_CameraInformation_getRangeMin(cameraInfo).value);
        printf("RangeMax: %.2f\n", ACS_Image_CameraInformation_getRangeMax(cameraInfo).value);
        printf("Horizonal FoV: %d\n", ACS_Image_CameraInformation_getHorizontalFoV(cameraInfo));
        printf("Focal Length: %.2f\n", ACS_Image_CameraInformation_getFocalLength(cameraInfo));
    }
}

void printStreamInformation(ACS_Camera* camera)
{
    size_t streamCount = ACS_Camera_getStreamCount(camera);
    for (size_t i = 0; i < streamCount; i++)
    {
        printf("Stream id:%zd, ", i);
        ACS_Stream* stream = ACS_Camera_getStream(camera, i);
        if (ACS_Stream_isThermal(stream))
        {
            printf("Thermal Stream\n");
        }
        else
        {
            printf("Colorized Stream\n");
        }
    }

}

