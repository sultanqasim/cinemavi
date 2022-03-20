#ifndef YCBCR_H
#define YCBCR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "colour_xfrm.h"

// ITU-R BT.709
static const ColourMatrix CM_sRGB2YCbCr = {.m={
     0.2126,  0.7152,  0.0722,
    -0.1146, -0.3854,  0.5000,
     0.5000, -0.4542, -0.0458
}};

static const ColourMatrix_f CMf_sRGB2YCbCr = {.m={
     0.2126,  0.7152,  0.0722,
    -0.1146, -0.3854,  0.5000,
     0.5000, -0.4542, -0.0458
}};

static const ColourMatrix CM_YCbCr2sRGB = {.m={
    1,  0.0000,  1.5748,
    1, -0.1873, -0.4681,
    1,  1.8556,  0.0000
}};

static const ColourMatrix_f CMf_YCbCr2sRGB = {.m={
    1,  0.0000,  1.5748,
    1, -0.1873, -0.4681,
    1,  1.8556,  0.0000
}};

#ifdef __cplusplus
}
#endif

#endif // YCBCR_H
