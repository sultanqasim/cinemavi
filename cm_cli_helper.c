#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "cm_cli_helper.h"
#include "dng.h"
#include "pipeline.h"

// Matrix to convert from camera RGB to sRGB in D65 daylight illumination
static const ColourMatrix default_calib = {.m={
     1.75883, -0.68132,  0.01113,
    -0.58876,  1.49340, -0.55559,
     0.04679, -0.59206,  2.02246
}};

static const ImagePipelineParams default_pipeline_params = {
    .exposure = 0.0,
    .warmth = 0.0,
    .tint = 0.0,
    .hue = 0.0,
    .sat = 1.0,
    .nr_lum = 150.0,
    .nr_chrom = 600.0,
    .gamma = 0.2,
    .shadow = 1,
    .lut_mode = CMLUT_HDR_CUBIC
};

void cinemavi_generate_dng(const void *raw, const CMRawHeader *cmrh,
        const char *fname)
{
    int dng_stat = bayer_rg12p_to_dng(raw, cmrh->cinfo.width, cmrh->cinfo.height, fname,
            cmrh->camera_model, &default_calib);
    if (dng_stat != 0) printf("Error %d writing DNG.\n", dng_stat);
    else printf("DNG written to: %s\n", fname);
}

void cinemavi_generate_tiff(const void *raw, const CMRawHeader *cmrh,
        const char *fname)
{
    uint8_t *rgb8 = (uint8_t *)malloc(cmrh->cinfo.width * cmrh->cinfo.height * 3);
    if (rgb8 != NULL) {
        printf("Processing image...\n");
        pipeline_process_image(raw, rgb8, &cmrh->cinfo, &default_pipeline_params, &default_calib);
        printf("Image processed.\n");

        int tiff_stat = rgb8_to_tiff(rgb8, cmrh->cinfo.width, cmrh->cinfo.height, fname);
        if (tiff_stat != 0) printf("Error %d writing TIFF.\n", tiff_stat);
        else printf("TIFF written to: %s\n", fname);
    }
    free(rgb8);
}

void cinemavi_generate_cmr(const void *raw, const CMRawHeader *cmrh,
        const char *fname)
{
    int cmr_stat = cmraw_save(raw, cmrh, fname);
    if (cmr_stat != 0)
        printf("Error %d writing CMR.\n", cmr_stat);
    else
        printf("CMR written to: %s\n", fname);
}

bool endswith(const char *s, const char *suffix)
{
    size_t s_len = strlen(s);
    size_t e_len = strlen(suffix);

    if (e_len > s_len) return false;

    return memcmp(s + s_len - e_len, suffix, e_len) ? false : true;
}
