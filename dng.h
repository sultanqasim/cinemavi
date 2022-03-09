#ifndef DNG_H
#define DNG_H

#include "colour_xfrm.h"
#include "cmraw.h"

#ifdef __cplusplus
extern "C" {
#endif

int bayer_rg12p_to_dng(const void *raw, const CMRawHeader *cmrh, const char *dng_name);

int rgb8_to_tiff(const uint8_t *img, uint16_t width, uint16_t height,
        const char *tiff_name);

#ifdef __cplusplus
}
#endif

#endif // DNG_H
