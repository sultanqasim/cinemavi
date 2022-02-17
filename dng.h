#ifndef DNG_H
#define DNG_H

#include "colour_xfrm.h"

#ifdef __cplusplus
extern "C" {
#endif

int bayer_rg12p_to_dng(const void *raw, uint16_t width, uint16_t height,
        const char *dng_name, const char *camera_model, const ColourMatrix *calib);

int rgb8_to_tiff(const uint8_t *img, uint16_t width, uint16_t height,
        const char *tiff_name);

#ifdef __cplusplus
}
#endif

#endif // DNG_H
