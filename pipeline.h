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
        const ImagePipelineParams *params, const ColourMatrix *calib);

// use fast 2x2 binned debayering and skip noise reduction
// output image is half height and half width
int pipeline_process_image_bin22(const void *raw, uint8_t *rgb8, const CMCaptureInfo *cinfo,
        const ImagePipelineParams *params, const ColourMatrix *calib);

#ifdef __cplusplus
}
#endif

#endif // PIPELINE_H
