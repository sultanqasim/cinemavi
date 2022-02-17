#ifndef CAMERA_PARAMS_H
#define CAMERA_PARAMS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pipeline.h"

// Used to generate matrix to convert from camera RGB to sRGB
// in D65 daylight illumination
static const CMCameraCalibration default_calib = {
    .warmth = -0.5,
    .tint = 0.4,
    .hue = 0.0,
    .sat = 1.03
};

static const ImagePipelineParams default_pipeline_params = {
    .exposure = 0.0,
    .warmth = 0.0,
    .tint = 0.0,
    .hue = 0.0,
    .sat = 1.0,
    .nr_lum = 150.0,
    .nr_chrom = 600.0,
    .gamma = 0.05,
    .shadow = 0.4,
    .lut_mode = CMLUT_HDR_AUTO
};

#ifdef __cplusplus
}
#endif

#endif // CAMERA_PARAMS_H
