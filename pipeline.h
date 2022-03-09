#ifndef PIPELINE_H
#define PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "cmraw.h"
#include "colour_xfrm.h"

typedef enum {
    CMLUT_LINEAR,
    CMLUT_FILMIC,
    CMLUT_CUBIC,
    CMLUT_HDR,
    CMLUT_HDR_AUTO,
    CMLUT_HDR_CUBIC,
    CMLUT_HDR_CUBIC_AUTO
} CMLUTMode;

typedef struct {
    double exposure;
    double temp_K;
    double tint;
    double hue;
    double sat;
    double nr_lum;
    double nr_chrom;
    double gamma;
    double shadow;
    double black;
    CMLUTMode lut_mode;
} ImagePipelineParams;

int pipeline_process_image(const void *raw, uint8_t *rgb8, const CMCaptureInfo *cinfo,
        const ImagePipelineParams *params);

// use fast 2x2 binned debayering and skip noise reduction
// output image is half height and half width
int pipeline_process_image_bin22(const void *raw, uint8_t *rgb8, const CMCaptureInfo *cinfo,
        const ImagePipelineParams *params);

typedef enum {
    CMWHITE_BRIGHTS,
    CMWHITE_ROBUST,
    CMWHITE_SPOT
} CMAutoWhiteMode;

typedef struct {
    CMAutoWhiteMode awb_mode;

    // for spot mode
    uint16_t pos_x;
    uint16_t pos_y;
} CMAutoWhiteParams;

int pipeline_auto_white_balance(const void *raw, const CMCaptureInfo *cinfo,
        const CMAutoWhiteParams *params, double *temp_K, double *tint);

#ifdef __cplusplus
}
#endif

#endif // PIPELINE_H
