#include <stdio.h>

#include "acs/thermal_image.h"
#include "acs/thermal_value.h"
#include "acs/palette.h"
#include "acs/utility.h"

int main(int argc, char** argv){
    if(argc < 2){
        fprintf(stderr,"Uso: %s <imagem>\n", argv[0]);
        return 2;
    }
    const char* path = argv[1];

    ACS_ThermalImage* img = ACS_ThermalImage_alloc();
    ACS_ThermalImage_openFromFile(img, path);

    ACS_Scale* scale = ACS_ThermalImage_getScale(img);
    ACS_ThermalValue vmin = ACS_Scale_getScaleMin(scale);
    ACS_ThermalValue vmax = ACS_Scale_getScaleMax(scale);

    ACS_String* sMin = ACS_ThermalValue_format(vmin);
    ACS_String* sMax = ACS_ThermalValue_format(vmax);

    ACS_Palette* p = ACS_ThermalImage_getPalette(img);
    const char* palName = p ? ACS_Palette_getName(p) : "(desconhecida)";

    printf("ScaleMin: %s\n", ACS_String_get(sMin));
    printf("ScaleMax: %s\n", ACS_String_get(sMax));
    printf("Palette : %s\n", palName);

    ACS_String_free(sMin);
    ACS_String_free(sMax);
    ACS_ThermalImage_free(img);
    return 0;
}
