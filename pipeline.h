#ifndef PIPELINE_H
#define PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

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
} ImagePipelineParams;

int pipeline_process_image(const void *raw, uint8_t *rgb8, const CMCaptureInfo *cinfo,
        const ImagePipelineParams *params);

// use fast 2x2 binned debayering and skip noise reduction
// output image is half height and half width
int pipeline_process_image_bin22(const void *raw, uint8_t *rgb8, const CMCaptureInfo *cinfo,
        const ImagePipelineParams *params);

#ifdef __cplusplus
}
#endif

#endif // PIPELINE_H
