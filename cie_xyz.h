#ifndef CIE_XYZ_H
#define CIE_XYZ_H

#ifdef __cplusplus
extern "C" {
#endif

#include "colour_xfrm.h"

// http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html

static const ColourMatrix CM_sRGB2XYZ = {.m={
    0.4124564, 0.3575761, 0.1804375,
    0.2126729, 0.7151522, 0.0721750,
    0.0193339, 0.1191920, 0.9503041
}};

static const ColourMatrix_f CMf_sRGB2XYZ = {.m={
    0.4124564, 0.3575761, 0.1804375,
    0.2126729, 0.7151522, 0.0721750,
    0.0193339, 0.1191920, 0.9503041
}};

static const ColourMatrix CM_XYZ2sRGB = {.m={
     3.2404542, -1.5371385, -0.4985314,
    -0.9692660,  1.8760108,  0.0415560,
     0.0556434, -0.2040259,  1.0572252
}};

static const ColourMatrix_f CMf_XYZ2sRGB = {.m={
     3.2404542, -1.5371385, -0.4985314,
    -0.9692660,  1.8760108,  0.0415560,
     0.0556434, -0.2040259,  1.0572252
}};

#ifdef __cplusplus
}
#endif

#endif // CIE_XYZ_H
