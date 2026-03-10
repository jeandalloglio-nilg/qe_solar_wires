#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "acs/acs.h"
#include "acs/thermal_image.h"
#include "acs/thermal_value.h"
#include "acs/palette.h"
#include "acs/utility.h"
#include "acs/import.h"
#include "acs/enum.h"

static int map_palette(const char* name) {
    if (!name) return ACS_PalettePreset_iron;
    char s[64]; size_t n = strlen(name); if (n > 63) n = 63;
    for (size_t i = 0; i < n; ++i) s[i] = (char)tolower((unsigned char)name[i]);
    s[n] = 0;
    if (strstr(s, "ironbow") || strstr(s, "iron")) return ACS_PalettePreset_iron;
    if (strstr(s, "rainbow")) return ACS_PalettePreset_rainbow;
    if (strstr(s, "bw"))      return ACS_PalettePreset_bw;
    if (strstr(s, "whitehot"))return ACS_PalettePreset_whitehot;
    if (strstr(s, "blackhot"))return ACS_PalettePreset_blackhot;
    return ACS_PalettePreset_iron;
}

static void usage(const char* prog) {
    fprintf(stderr,
        "Uso: %s <imagem> <tminC> <tmaxC> [palette=iron] [out=overwrite]\n"
        "Ex.: %s img.jpg 30 45 iron overwrite\n", prog, prog);
}

int main(int argc, char** argv) {
    if (argc < 4) { usage(argv[0]); return 2; }

    const char* in_path = argv[1];
    const double tmin_c = atof(argv[2]);
    const double tmax_c = atof(argv[3]);
    const char* palette  = (argc >= 5) ? argv[4] : "iron";
    const char* out_arg  = (argc >= 6) ? argv[5] : "overwrite"; // "overwrite" ou caminho

    if (tmax_c <= tmin_c) {
        fprintf(stderr, "Erro: tmaxC (%.3f) deve ser > tminC (%.3f)\n", tmax_c, tmin_c);
        return 2;
    }

    ACS_ThermalImage* img = ACS_ThermalImage_alloc();
    if (!img) { fprintf(stderr, "Falha alloc ThermalImage\n"); return 1; }

    ACS_ThermalImage_openFromFile(img, in_path);
    const char* opened = ACS_ThermalImage_getFilePath(img);
    if (!opened || opened[0] == '\0') {
        fprintf(stderr, "Falha ao abrir arquivo: %s\n", in_path);
        ACS_ThermalImage_free(img);
        return 1;
    }

    ACS_ThermalImage_setTemperatureUnit(img, ACS_TemperatureUnit_celsius);

    // (Opcional) Se a sua enum tiver estes símbolos e o viewer respeitar o modo:
    // ACS_ThermalImage_setColorDistributionMode(img, ACS_ColorDistributionMode_TemperatureLinear);
    // ou: ACS_ThermalImage_setColorDistributionMode(img, ACS_ColorDistributionMode_SignalLinear);

    ACS_Scale* scale = ACS_ThermalImage_getScale(img);
    if (!scale) {
        fprintf(stderr, "Falha ao obter ACS_Scale\n");
        ACS_ThermalImage_free(img);
        return 1;
    }
    ACS_ThermalValue vmin = ACS_ThermalValue_fromCelsius(tmin_c);
    ACS_ThermalValue vmax = ACS_ThermalValue_fromCelsius(tmax_c);
    ACS_Scale_setScale(scale, vmin, vmax);

    // log (stderr) para conferência
    {
        ACS_ThermalValue gotMin = ACS_Scale_getScaleMin(scale);
        ACS_ThermalValue gotMax = ACS_Scale_getScaleMax(scale);
        ACS_String* sMin = ACS_ThermalValue_format(gotMin);
        ACS_String* sMax = ACS_ThermalValue_format(gotMax);
        fprintf(stderr, "[atlas_scale_set] scale: min=%s  max=%s\n",
                ACS_String_get(sMin), ACS_String_get(sMax));
        ACS_String_free(sMin);
        ACS_String_free(sMax);
    }

    int pal_preset = map_palette(palette);
    ACS_ThermalImage_setPalettePreset(img, pal_preset);
    // Alternativa (forçar via objeto):
    // const ACS_Palette* pconst = ACS_Palette_getPalettePreset(pal_preset);
    // ACS_ThermalImage_setPalette(img, (ACS_Palette*)pconst);

    const char* out_path = (strcmp(out_arg, "overwrite") == 0) ? in_path : out_arg;

    // 0 = manter formato original. Se sua enum tiver RJPEG, você pode preferir:
    // ACS_ThermalImage_saveAs(img, out_path, ACS_ImageFileFormat_Rjpeg);
    ACS_ThermalImage_saveAs(img, out_path, 0);

    ACS_ThermalImage_free(img);
    puts("OK");
    return 0;
}
