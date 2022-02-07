#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "cmraw.h"
#include "pipeline.h"
#include "dng.h"

static void cinemavi_generate_tiff(const void *raw, const CMRawHeader *cmrh,
        const char *fname)
{
    uint8_t *rgb8 = (uint8_t *)malloc(cmrh->cinfo.width * cmrh->cinfo.height * 3);
    if (rgb8 != NULL) {
        ImagePipelineParams params = {
            .exposure = 1.0,
            .warmth = -0.2,
            .tint = 0.3,
            .hue = 0.0,
            .sat = 1.0,
            .nr_lum = 150.0,
            .nr_chrom = 600.0,
            .gamma = 0.25,
            .shadow = 1.0
        };
        printf("Processing image...\n");
        pipeline_process_image(raw, rgb8, &cmrh->cinfo, &params);
        printf("Image processed.\n");

        int tiff_stat = rgb8_to_tiff(rgb8, cmrh->cinfo.width, cmrh->cinfo.height, fname);
        if (tiff_stat != 0) printf("Error %d writing TIFF.\n", tiff_stat);
        else printf("TIFF written to: %s\n", fname);
    }
    free(rgb8);
}

static bool endswith(const char *s, const char *suffix)
{
    size_t s_len = strlen(s);
    size_t e_len = strlen(suffix);

    if (e_len > s_len) return false;

    return memcmp(s + s_len - e_len, suffix, e_len) ? false : true;
}

int main (int argc, char **argv)
{
    if (argc != 3) {
        printf("Usage: %s [cmr_name] [tiff_name]\n", argv[0]);
        return -1;
    }

    if (!endswith(argv[1], ".cmr")) {
        printf("Invalid input extension: %s\n", argv[1]);
        return -1;
    }

    if (!endswith(argv[2], ".tiff")) {
        printf("Invalid output extension: %s\n", argv[2]);
        return -1;
    }

    CMRawHeader cmrh;
    void *raw = NULL;

    int status = cmraw_load(&raw, &cmrh, argv[1]);
    if (status != 0) {
        printf("Error %d loading RAW file.\n", status);
    } else {
        cinemavi_generate_tiff(raw, &cmrh, argv[2]);
    }

    free(raw);

    return status;
}
