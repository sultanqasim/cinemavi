#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "cm_cli_helper.h"
#include "dng.h"
#include "pipeline.h"
#include "cm_calibrations.h"
#include "colour_xfrm.h"

static const ImagePipelineParams default_pipeline_params = {
    .exposure = 0.0,
    .temp_K = 5000.0,
    .tint = 0.2,
    .hue = 0.0,
    .sat = 1.0,
    .nr_lum = 150.0,
    .nr_chrom = 600.0,
    .gamma = 0.3,
    .shadow = 1,
    .black = 0.3,
    .lut_mode = CMLUT_HDR_CUBIC
};

void cinemavi_generate_dng(const void *raw, const CMRawHeader *cmrh,
        const char *fname)
{
    uint16_t calib_index = cmrh->cinfo.calib_id;
    if (calib_index >= CMCAL_NUM_CALIBRATIONS) calib_index = 0;

    int dng_stat = bayer_rg12p_to_dng(raw, cmrh->cinfo.width, cmrh->cinfo.height, fname,
            cmrh->camera_model, &CM_cam_calibs[calib_index]);
    if (dng_stat != 0) printf("Error %d writing DNG.\n", dng_stat);
    else printf("DNG written to: %s\n", fname);
}

void cinemavi_generate_tiff(const void *raw, const CMRawHeader *cmrh,
        const char *fname)
{
    uint8_t *rgb8 = (uint8_t *)malloc(cmrh->cinfo.width * cmrh->cinfo.height * 3);
    if (rgb8 != NULL) {
        // use as-shot white balance if specified
        ImagePipelineParams pipeline_params = default_pipeline_params;
        if (cmrh->cinfo.white_x > 0 || cmrh->cinfo.white_y > 0)
            colour_xy_to_temp_tint(cmrh->cinfo.white_x, cmrh->cinfo.white_y,
                    &pipeline_params.temp_K, &pipeline_params.tint);

        printf("Processing image...\n");
        pipeline_process_image(raw, rgb8, &cmrh->cinfo, &pipeline_params);
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
