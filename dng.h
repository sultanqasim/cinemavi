#ifndef DNG_H
#define DNG_H

#include <arv.h>
#include "colour_xfrm.h"

#ifdef __cplusplus
extern "C" {
#endif

int arv_buffer_to_dng(ArvBuffer *buffer, const char *dng_name, const char *camera_model);

int rgb8_to_tiff(const uint8_t *img, uint32_t width, uint32_t height, const char *tiff_name);

#ifdef __cplusplus
}
#endif

#endif // DNG_H
