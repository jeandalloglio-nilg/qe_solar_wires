#include <stdlib.h>
#include <stdio.h>

#include <acs/buffer.h>
#include <acs/common.h>
#include "acs/framepacer.h"
#include <acs/palette.h>
#include <acs/thermal_image.h>
#include <acs/thermal_sequence_player.h>
#include <string.h>

#include "acs/renderer.h"


void checkAcs(void);
void checkAcsError(ACS_Error error);
void setColorDistributionMode(ACS_ThermalImage* image, int mode);

void printUsage(const char* cmd)
{
    printf("usage: %s [option=value ...]\n", cmd);
    printf("Run the colorizer on an IR sequence/image\n");
    printf("option:  --help : Shows this help\n");
    printf("option:  --mode : Set agc/color distribution mode:\n           0 = temperatureLinear\n           1 = histogramEqualization\n           2 = signalLinear\n           3 = plateauHistogramEqualization\n           4 = dde\n           5 = entropy\n           6 = ade\n           7 = fsx\n");
    printf("option:  --file : ir sequence or image the colorizer will have as input\n");
    printf("\n");
    printf("Example: ./%s --mode=2 --file=<full_path_to_image_or_sequence>\n", cmd);
}


int main(int argc, char* argv[])
{
    const char* irFilename = NULL;

    // Decrease log level in order to make frame rate logging more readable
    ACS_Logger_setLevel(ACS_LogLevel_info);

    ACS_ColorDistributionMode colorDistributionMode = ACS_ColorDistribution_signalLinear;

    for (int i = 1; i < argc; i++)
    {
        char* pos = strchr(argv[i], '=');

        if (strstr(argv[i], "--file") != NULL && pos != NULL)
           irFilename = pos + 1;
        if (strstr(argv[i], "--mode") != NULL && pos != NULL)
           colorDistributionMode = (ACS_ColorDistributionMode)strtol(pos + 1, NULL, 10);
        else if (strstr(argv[i], "--help") != NULL)
            printUsage(argv[0]);
        else
            fprintf(stderr, "Unknown argument %s\n", argv[i]);
    }

    if (!irFilename)
    {
        fprintf(stderr, "No file argument given, exiting program.\n");
        return -1;
    }
    ACS_NativeString* fileName = ACS_NativeString_createFrom(irFilename);
    ACS_ThermalSequencePlayer* player = ACS_ThermalSequencePlayer_alloc(ACS_NativeString_get(fileName));
    ACS_NativeString_free(fileName);
    checkAcs();

    ACS_ThermalImage* image = ACS_ThermalSequencePlayer_getCurrentFrame(player);
    checkAcs();
    
    setColorDistributionMode(image, colorDistributionMode);

    ACS_ImageColorizer* imageColorizer = ACS_ImageColorizer_alloc(image);
    checkAcs();

    ACS_Colorizer* colorizer = ACS_ImageColorizer_asColorizer(imageColorizer);
    
    ACS_Colorizer_setAutoScale(colorizer, true);
    ACS_Colorizer_setIsStreaming(colorizer, true);              

    ACS_Renderer* renderer = ACS_Colorizer_asRenderer(colorizer);
    ACS_Renderer_setOutputColorSpace(renderer, ACS_ColorSpaceType_bgra);

    ACS_ThermalImage_setPalettePreset(image, ACS_PalettePreset_arctic);
    
    size_t totalFrames = ACS_ThermalSequencePlayer_frameCount(player);
    double framerate = ACS_ThermalSequencePlayer_getPlaybackRate(player);
    printf("Loaded %s:\n", irFilename);
    printf("Frame count:%zu \n", totalFrames);
    printf("Frame rate :%.2f\n", framerate);

    // Create a window and run the render loop
    ACS_DebugImageWindow* window = ACS_DebugImageWindow_alloc("C stream sample");
    checkAcs();

    ACS_FramePacer* framePacer = ACS_FramePacer_alloc(framerate ? framerate : 9, true, 30); // Frame rate is expected to be set in sequence, use default otherwise;
    checkAcs();

    while (ACS_DebugImageWindow_poll(window))
    {   
        setColorDistributionMode(image, colorDistributionMode);

        ACS_Renderer_update(renderer);

        // Display received image on screen
        ACS_DebugImageWindow_update(window, ACS_Renderer_getImage(renderer));
        checkAcs();

        if (!ACS_ThermalSequencePlayer_next(player))
            ACS_ThermalSequencePlayer_first(player);

        ACS_FramePacer_frameSync(framePacer, ACS_FrameSynchronizationStrategy_ThreadSleep);
    }

    ACS_FramePacer_free(framePacer);
    ACS_DebugImageWindow_free(window);
    checkAcs();

    return 0;
}

void setColorDistributionMode(ACS_ThermalImage* image, int mode)
{
    switch (mode)
    {
        case ACS_ColorDistribution_temperatureLinear:
            ACS_ThermalImage_setTemperatureLinearSettings(image);
            break;

        case ACS_ColorDistribution_histogramEqualization:
            ACS_ThermalImage_setHistogramEqualizationSettings(image);
            break;   
        case ACS_ColorDistribution_signalLinear:
            ACS_ThermalImage_setSignalLinearSettings(image);
            break;
        case ACS_ColorDistribution_plateauHistogramEqualization:
            ACS_ThermalImage_setPlateauHistogramEqSettings(image, NULL);
            break;
        case ACS_ColorDistribution_dde:
            ACS_ThermalImage_setDdeSettings(image, NULL);
            break;
        case ACS_ColorDistribution_entropy:
            ACS_ThermalImage_setEntropySettings(image, NULL);
            break;
        case ACS_ColorDistribution_ade:
            ACS_ThermalImage_setAdeSettings(image, NULL);
            break;
        case ACS_ColorDistribution_fsx:
            ACS_ThermalImage_setFsxSettings(image, NULL);
            break;
    }

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
