#ifndef PIPELINE_H
#define PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum {
    CMLUT_LINEAR,
    CMLUT_FILMIC,
    CMLUT_CUBIC,
    CMLUT_HDR,
    CMLUT_HDR_AUTO,
    CMLUT_HDR_CUBIC
} CMLUTMode;

// Crudely maps from Camera RGB to sRGB in D65 light
typedef struct {
    double warmth;
    double tint;
    double hue;
    double sat;
} CMCameraCalibration;

typedef struct {
    double exposure;
    double warmth;
    double tint;
    double hue;
    double sat;
    double nr_lum;
    double nr_chrom;
    double gamma;
    double shadow;
    CMLUTMode lut_mode;
} ImagePipelineParams;

int pipeline_process_image(const void *raw, uint8_t *rgb8, const CMCaptureInfo *cinfo,
        const ImagePipelineParams *params, const CMCameraCalibration *calib);

// use fast 2x2 binned debayering and skip noise reduction
// output image is half height and half width
int pipeline_process_image_bin22(const void *raw, uint8_t *rgb8, const CMCaptureInfo *cinfo,
        const ImagePipelineParams *params, const CMCameraCalibration *calib);

#ifdef __cplusplus
}
#endif

#endif // PIPELINE_H
