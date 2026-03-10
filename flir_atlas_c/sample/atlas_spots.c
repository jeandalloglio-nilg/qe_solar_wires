// atlas_spots.c
// Lib C para gravar Measurement Spots (Sp1..SpN) em RJPEG da FLIR via Atlas C SDK

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// === headers do seu Atlas C SDK (você mandou estes):
#include "measurements.h"          // ACS_Measurements_addSpot, etc.
#include "measurement_spot.h"      // ACS_MeasurementSpot_setSpot, ...
#include "measurement_marker.h"    // (opcional)
#include "measurement_rectangle.h" // (opcional)

// ============================================================
// IMPORTANTE: ligue estas 4 funções aos headers corretos do SDK
// (nomes variam entre versões: acs_image.h, rjpeg.h, flir_image_c.h, etc.)
// ============================================================
typedef struct ACS_Image ACS_Image; // handle opaco; definido no SDK

// Retornam 0 em sucesso, !=0 em erro.
extern int  ACS_Image_open (const char* path, ACS_Image** outImg);    // <- SUBSTITUA pelo header real
extern ACS_Measurements* ACS_Image_getMeasurements(ACS_Image* img);   // <- SUBSTITUA
extern int  ACS_Image_save (ACS_Image* img, const char* path);        // <- SUBSTITUA
extern void ACS_Image_close(ACS_Image* img);                          // <- SUBSTITUA
// ============================================================

// API exportada pela lib para o Python (ctypes)
// xs/ys: arrays de inteiros com coordenadas de pixel; n: quantidade
// clear_existing: 0/1 — se true, tenta remover medições existentes (quando o SDK suportar)
// Se a API de "nome" não estiver disponível no seu build, os spots ainda aparecem no Thermal Studio.
#ifdef _WIN32
  #define API_EXPORT __declspec(dllexport)
#else
  #define API_EXPORT __attribute__((visibility("default")))
#endif

// helper opcional — alguns SDKs dão método para limpar; aqui não chamamos nada específico.
// Você pode implementar varredura & remoção por id se o seu SDK expuser.
static void maybe_clear_existing(ACS_Measurements* ms, int clear_existing) {
    (void)ms;
    (void)clear_existing;
    // Placeholder: se o seu SDK tiver "ACS_Measurements_clear(ms)" ou equivalente, chame aqui.
    // Ex.: if (clear_existing) ACS_Measurements_clear(ms);
}

API_EXPORT
int atlas_write_spots(const char* in_rjpeg_path,
                      const int* xs,
                      const int* ys,
                      int n,
                      const char* out_rjpeg_path,
                      int clear_existing)
{
    if (!in_rjpeg_path || !out_rjpeg_path || !xs || !ys || n <= 0) {
        return -1; // argumentos inválidos
    }

    ACS_Image* img = NULL;
    int rc = ACS_Image_open(in_rjpeg_path, &img);
    if (rc != 0 || !img) {
        fprintf(stderr, "ACS_Image_open falhou rc=%d (%s)\n", rc, in_rjpeg_path);
        return -2;
    }

    ACS_Measurements* ms = ACS_Image_getMeasurements(img);
    if (!ms) {
        fprintf(stderr, "ACS_Image_getMeasurements retornou NULL\n");
        ACS_Image_close(img);
        return -3;
    }

    // opcional: limpar medições existentes
    maybe_clear_existing(ms, clear_existing);

    // adiciona Sp1..SpN
    for (int i = 0; i < n; ++i) {
        int x = xs[i];
        int y = ys[i];

        ACS_MeasurementSpot* spot = ACS_Measurements_addSpot(ms, x, y);
        if (!spot) {
            fprintf(stderr, "ACS_Measurements_addSpot(%d,%d) falhou (i=%d)\n", x, y, i+1);
            continue;
        }

        // se quiser reposicionar explicitamente (normalmente não precisa, já nasce em x,y):
        // ACS_MeasurementSpot_setSpot(spot, x, y);

        // ====== NOME "SpN" ======
        // Alguns builds expõem API de "shape aspect" para setar o rótulo:
        //   const ACS_MeasurementShape* shape = ACS_MeasurementSpot_asMeasurementShape(spot);
        //   ACS_MeasurementShape_setName((ACS_MeasurementShape*)shape, label);
        // Como os headers de "shape" não estão aqui, só tentamos nomear se você tiver:
        // (Deixe assim; os spots aparecem no Thermal Studio mesmo sem setar Name explicitamente.)
        (void)spot;
    }

    rc = ACS_Image_save(img, out_rjpeg_path);
    if (rc != 0) {
        fprintf(stderr, "ACS_Image_save falhou rc=%d (%s)\n", rc, out_rjpeg_path);
        ACS_Image_close(img);
        return -4;
    }

    ACS_Image_close(img);
    return 0;
}
