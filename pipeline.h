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
} ImagePipelineParams;

int pipeline_process_image(const void *bayer12p, uint8_t *rgb8, uint16_t width,
        uint16_t height, const ImagePipelineParams *params);

#ifdef __cplusplus
}
#endif

#endif // PIPELINE_H
