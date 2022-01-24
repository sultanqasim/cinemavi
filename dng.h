#ifndef DNG_H
#define DNG_H

#include <arv.h>
#include "colour_xfrm.h"

#ifdef __cplusplus
extern "C" {
#endif

int arv_buffer_to_dng(ArvBuffer *buffer, const char *dng_name, const char *camera_model);

#ifdef __cplusplus
}
#endif

#endif // DNG_H
