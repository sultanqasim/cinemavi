#ifndef PIPELINE_H
#define PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct {
    double warmth;
    double tint;
    double hue;
    double sat;
    double nr_lum;
    double nr_chrom;
    double gamma;
    double shadow;
} ImagePipelineParams;

int pipeline_process_image(const void *bayer12p, uint8_t *rgb8, uint16_t width,
        uint16_t height, const ImagePipelineParams *params);

// use fast 2x2 binned debayering and skip noise reduction
// output image is half height and half width
int pipeline_process_image_bin22(const void *bayer12p, uint8_t *rgb8, uint16_t width,
        uint16_t height, const ImagePipelineParams *params);

#ifdef __cplusplus
}
#endif

#endif // PIPELINE_H
