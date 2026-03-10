#include <acs/thermal_image.h>
#include <acs/renderer.h>
#include <acs/palette.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void checkAcs(void);
void printImageCameraInformation(const ACS_Image_CameraInformation* cameraInfo);
void printImageParameters(const ACS_ThermalParameters* imageParams);
void openImageWithAreaMeasure(const char* filename);
void printAreaDimensions(const ACS_MeasurementArea* area);
void printEllipse(const ACS_MeasurementEllipse* ellipse);
void printEllipseFancy(ACS_MeasurementEllipse* ellipse, const char* message);
void printQuantificationInput(const ACS_GasQuantificationInput* qi);
void printQuantificationResult(const ACS_GasQuantificationResult* qr);
void printWindspeed(int ws);
void printLeakType(int lt);

void printUsage(const char* cmd)
{
    printf("usage: %s [option] <full_path_to_image>\n", cmd);
    printf("option:  --help : Shows this help\n");
    printf("option:  --area : display image measurements\n");
    printf("\n");
}

int main(int argc, char* argv[])
{
    bool openAreaMeasure = false;
    const char* filename = NULL;

    for (int i = 1; i < argc; i++)
    {
        if (strstr(argv[i], "--area") != NULL)
            openAreaMeasure = true;
        else if (strstr(argv[i], "--help") != NULL)
            printUsage(argv[0]);
        else
            filename = argv[i];
    }

    if (!filename)
    {
        fprintf(stderr, "No file argument given, exiting program.\n");
        printUsage(argv[0]); 
        return -1;
    }

    if (openAreaMeasure)
        openImageWithAreaMeasure(filename);

    ACS_NativeString* fileNameNative = ACS_NativeString_createFrom(filename);

    ACS_ThermalImage* image = ACS_ThermalImage_alloc();
    ACS_ThermalImage_openFromFile(image, ACS_NativeString_get(fileNameNative));
    ACS_NativeString_free(fileNameNative);
    checkAcs();

    ACS_ThermalImage_setPalettePreset(image, (int)ACS_PalettePreset_rainbow);
    
    const ACS_Palette* palette = ACS_ThermalImage_getPalette(image);
    const char* palette_name = ACS_Palette_getName(palette);
    printf("Palette name: %s\n", palette_name);

    // get cameraInformation from image and print it out
    ACS_Image_CameraInformation* cameraInfo = ACS_ThermalImage_getCameraInformation(image);
    checkAcs();
    printImageCameraInformation(cameraInfo);

    // get thermal parameters from image and print it out
    ACS_ThermalParameters* imageParams = ACS_ThermalImage_getThermalParameters(image);
    checkAcs();
    printImageParameters(imageParams);

    printf("Distance unit: %d\n", ACS_ThermalImage_getDistanceUnit(image));

    ACS_GasQuantificationInput quantificationInformation = ACS_ThermalImage_getGasQuantificationInput(image);
    if (ACS_getErrorCondition(ACS_getLastError()) == ACS_SUCCESS)
        printQuantificationInput(&quantificationInformation);
    else
        printf("No Gas Quantification Input data in image.\n");

    ACS_GasQuantificationResult quantificationResult = ACS_ThermalImage_getGasQuantificationResult(image);
    if (ACS_getErrorCondition(ACS_getLastError()) == ACS_SUCCESS)
        printQuantificationResult(&quantificationResult);
    else
        printf("No Gas Quantification Result data in image.\n");

    // Set up the colorizer
    ACS_ImageColorizer* colorizer = ACS_ImageColorizer_alloc(image);
    checkAcs();
    ACS_Renderer* renderer = ACS_Colorizer_asRenderer(ACS_ImageColorizer_asColorizer(colorizer));

    // Colorize the image
    ACS_Renderer_update(renderer);
    checkAcs();

    // Display original colorized image in a window
    ACS_DebugImageWindow* window = ACS_DebugImageWindow_alloc("Original colorizer setting");
    checkAcs();
    ACS_DebugImageWindow_update(window, ACS_Renderer_getImage(renderer));
    checkAcs();

    // Change image to Entropy settings and display
    ACS_ThermalImage_setEntropySettings(image, NULL);
    ACS_Renderer_update(renderer);
    checkAcs();

    ACS_DebugImageWindow* window_entropy = ACS_DebugImageWindow_alloc("Entropy Default Settings colorizer setting");
    checkAcs();
    ACS_DebugImageWindow_update(window_entropy, ACS_Renderer_getImage(renderer));
    checkAcs();

    // Change image to Plateau HistEq settings and display
    ACS_ThermalImage_setPlateauHistogramEqSettings(image, NULL);
    ACS_Renderer_update(renderer);
    checkAcs();

    ACS_DebugImageWindow* window_platheq = ACS_DebugImageWindow_alloc("Plateau HistEq Default Settings colorizer setting");
    checkAcs();
    ACS_DebugImageWindow_update(window_platheq, ACS_Renderer_getImage(renderer));
    checkAcs();

    // Change image to ADE settings and display
    ACS_ThermalImage_setAdeSettings(image, NULL);
    ACS_Renderer_update(renderer);
    checkAcs();

    ACS_DebugImageWindow* window_ade = ACS_DebugImageWindow_alloc("ADE Default Settings colorizer setting");
    checkAcs();
    ACS_DebugImageWindow_update(window_ade, ACS_Renderer_getImage(renderer));
    checkAcs();

    // Change image to optimized ADE settings and display
    ACS_AdeSettings adeSettings = ACS_ThermalImage_getAdeSettings(image);
    adeSettings.alphaNoise = 150.0F;
    adeSettings.betaLf = 200.0F;
    adeSettings.betaHf = 5000.0F;
    adeSettings.betaMix = 160.0F;
    adeSettings.hpBlendingAmount = 300.0F;
    adeSettings.lowLimit = 0.015F;
    adeSettings.highLimit = 0.9975F;
    adeSettings.headRoom = 0.8F;
    adeSettings.footRoom = 0.05F;
    adeSettings.gain = 6000.0F;
    adeSettings.linearMix = 0.0F;

    ACS_ThermalImage_setAdeSettings(image, &adeSettings);
    ACS_Renderer_update(renderer);
    checkAcs();

    ACS_DebugImageWindow* window_ade_opt = ACS_DebugImageWindow_alloc("ADE Optimized Settings colorizer setting");
    checkAcs();
    ACS_DebugImageWindow_update(window_ade_opt, ACS_Renderer_getImage(renderer));
    checkAcs();

    ACS_Image_CameraInformation_free(cameraInfo);
    // Clean up thermal image and colorizer
    ACS_ImageColorizer_free(colorizer);
    ACS_ThermalImage_free(image);

    // Show window until keyboard is pressed
    while (ACS_DebugImageWindow_poll(window))
    {
    }
    ACS_DebugImageWindow_free(window);
    ACS_DebugImageWindow_free(window_entropy);
    ACS_DebugImageWindow_free(window_platheq);
    ACS_DebugImageWindow_free(window_ade);
    ACS_DebugImageWindow_free(window_ade_opt);

    return 0;
}

void checkAcs(void)
{
    // Check if last call to a ACS_* function failed
    ACS_Error error = ACS_getLastError();

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

void printImageParameters(const ACS_ThermalParameters* imageParams)
{
    if (imageParams)
    {
        printf("Distance: %f\n", ACS_ThermalParameters_getObjectDistance(imageParams));
        printf("Emissivity: %f\n", ACS_ThermalParameters_getObjectEmissivity(imageParams));
        printf("Reflected temperature: %f\n", ACS_ThermalParameters_getObjectReflectedTemperature(imageParams).value);
        printf("Relative humidity: %f\n", ACS_ThermalParameters_getRelativeHumidity(imageParams));
        printf("Atmospheric temperature: %f\n", ACS_ThermalParameters_getAtmosphericTemperature(imageParams).value);
        printf("Transmission: %f\n", ACS_ThermalParameters_getAtmosphericTransmission(imageParams));
        printf("External optics temperature: %f\n", ACS_ThermalParameters_getExternalOpticsTemperature(imageParams).value);
        printf("External optics transmission: %f\n", ACS_ThermalParameters_getExternalOpticsTransmission(imageParams));
    }
}

void openImageWithAreaMeasure(const char* filename)
{
    ACS_NativeString* fileName = ACS_NativeString_createFrom(filename);
    ACS_ThermalImage* image = ACS_ThermalImage_alloc();
    ACS_ThermalImage_openFromFile(image, ACS_NativeString_get(fileName));
    ACS_NativeString_free(fileName);
    checkAcs();

    
    printf("============================================\n");
    printf("Width: %d, Height: %d\n", ACS_ThermalImage_getWidth(image), ACS_ThermalImage_getHeight(image));
    ACS_Measurements* measurements = ACS_ThermalImage_getMeasurements(image);
    if (!measurements)
    {
        printf("openImageWithAreaMeasure: Measurements not found in image\n");
        return;
    }

    ACS_ListMeasurementSpot* spots = ACS_Measurements_getAllSpots(measurements);
    for (size_t i = 0; i != ACS_ListMeasurementSpot_size(spots); i++)
    {
        const ACS_MeasurementShape* sh = ACS_MeasurementSpot_asMeasurementShape(ACS_ListMeasurementSpot_item(spots, i));
        ACS_String* label = ACS_MeasurementShape_getLabel(sh);
        printf("Spot %d: label = %s\n", ACS_MeasurementShape_getId(sh), ACS_String_get(label));
        ACS_String_free(label);
    }

    ACS_ListMeasurementRectangle* rectangles = ACS_Measurements_getAllRectangles(measurements);
    for (size_t i = 0; i != ACS_ListMeasurementRectangle_size(rectangles); i++)
    {
        const ACS_MeasurementShape* sh = ACS_MeasurementRectangle_asMeasurementShape(ACS_ListMeasurementRectangle_item(rectangles, i));
        ACS_String* label = ACS_MeasurementShape_getLabel(sh);
        printf("Rectangle %d: label = %s\n", ACS_MeasurementShape_getId(sh), ACS_String_get(label));
        ACS_String_free(label);
    }

    ACS_ListMeasurementEllipse* ellipses = ACS_Measurements_getAllEllipses(measurements);
    size_t ellipseCount = ACS_ListMeasurementEllipse_size(ellipses);
    for (size_t i = 0; i != ellipseCount; i++)
    {
        ACS_MeasurementEllipse* ellipse = ACS_ListMeasurementEllipse_item(ellipses, i);
        const ACS_MeasurementShape* sh = ACS_MeasurementEllipse_asMeasurementShape(ellipse);
        ACS_String* label = ACS_MeasurementShape_getLabel(sh);
        printf("Ellipse %d: label = %s\n", ACS_MeasurementShape_getId(sh), label ? ACS_String_get(label) : "<n/a>");
        ACS_String_free(label);

        printAreaDimensions(ACS_MeasurementEllipse_asMeasurementArea(ellipse));
    }

    ACS_ListMeasurementSpot_free(spots);
    ACS_ListMeasurementRectangle_free(rectangles);
    
    if (ellipseCount == 0)
    {
        ACS_ListMeasurementEllipse_free(ellipses);
        return;    
    }

    ACS_MeasurementEllipse* c0 = ACS_ListMeasurementEllipse_item(ellipses, 0);
    
    ACS_Point pos = ACS_MeasurementEllipse_getPosition(c0);
    int radiusX = ACS_MeasurementEllipse_getRadiusX(c0);
    int radiusY = ACS_MeasurementEllipse_getRadiusY(c0);
    const int offset = 20;

    printEllipseFancy(c0, "INITIAL ELLIPSE");

    // move the ellipse, without changing the size
    ACS_MeasurementEllipse_setEllipse(c0, pos.x + offset, pos.y + offset, radiusX, radiusY);
    printEllipseFancy(c0, "MOVED ELLIPSE");

    // move the ellipse back to initial position and resize it
    ACS_MeasurementEllipse_setEllipse(c0, pos.x, pos.y, radiusX + offset, radiusY + offset);
    printEllipseFancy(c0, "RESIZED ELLIPSE");

    ACS_ListMeasurementEllipse_free(ellipses);
}

void printAreaDimensions(const ACS_MeasurementArea* area)
{
    ACS_AreaDimensions ads = ACS_MeasurementArea_getAreaDimensions(area);
    printf("AreaDimensions[%g, %g, %g, %g, %g,%g, %s]\n",ads.area, ads.height, ads.width, ads.length, ads.radiusX, ads.radiusY, ads.valid ? "valid" : "invalid");
}

void printEllipse(const ACS_MeasurementEllipse* ellipse)
{
    ACS_Point pos = ACS_MeasurementEllipse_getPosition(ellipse);
    int radiusX = ACS_MeasurementEllipse_getRadiusX(ellipse);
    int radiusY = ACS_MeasurementEllipse_getRadiusY(ellipse);
    printf("Position: ( %d, %d ), radiusX: %d, radiusY: %d", pos.x, pos.y, radiusX, radiusY);
}

void printEllipseFancy(ACS_MeasurementEllipse* ellipse, const char* message)
{
    printf("\n===== %s =====\n", message);
    printAreaDimensions(ACS_MeasurementEllipse_asMeasurementArea(ellipse));
    printEllipse(ellipse);
    printf("--------------------------\n"); 
}

void printThermalValue(ACS_ThermalValue tv)
{
    ACS_String* acsstr = ACS_ThermalValue_format(tv);
    const char* str = ACS_String_get(acsstr);
    printf("%s\n", str);
    ACS_String_free(acsstr);
}

void printQuantificationInput(const ACS_GasQuantificationInput* qi)
{
    printf("#### Gas quantification input parameters ####\n");
    printf("Gas: %s\n", qi->gas);
    printLeakType(qi->leakType);
    printWindspeed(qi->windSpeed);
    printf("Ambient temperature: ");
    printThermalValue(qi->ambientTemperature);
    printf("Distance: %d m\n", qi->distance);
    printf("Threshold delta temperature: ");
    printThermalValue(qi->thresholdDeltaTemperature);
    printf("Emissive: %s\n", qi->emissive ? "true" : "false");
    printf("--------------------------\n");
}

void printQuantificationResult(const ACS_GasQuantificationResult* qr)
{
    printf("#### Gas quantification result ####\n");
    printf("Flow: %g\n", qr->flow);
    printf("Concentration: %g\n", qr->concentration);
    printf("--------------------------\n");
}

void printWindspeed(int ws)
{
    printf("Windspeed: ");
    if (ws == ACS_WindSpeed_calm)
        printf("calm\n");
    if (ws == ACS_WindSpeed_normal)
        printf("normal\n");
    if (ws == ACS_WindSpeed_high)
        printf("high\n");
}

void printLeakType(int lt)
{
    printf("Leak type: ");
    if (lt == ACS_GasLeakType_point)
        printf("point\n");
    if (lt == ACS_GasLeakType_diffused)
        printf("diffused\n");
}