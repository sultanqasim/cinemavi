#ifndef YCRCG_H
#define YCRCG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "colour_xfrm.h"

/* YCrCg is a colour space that's a cartesian version of HSL.
 *
 * Y is luminance (non-perceptual, weighing R, G, and B equally)
 * Cr is red
 * Cg is yellow-green
 *
 * Since YCrCg and RGB share the same origin, you can transform from RGB to YCrCg
 * by projecting onto the basis vectors in RGB space, which are orthogonal. The
 * RGB to YCrCg transformation matrix is:
 *
 * sqrt(1/3)  sqrt(1/3)  sqrt(1/3)
 * sqrt(2/3) -sqrt(1/6) -sqrt(1/6)
 * 0          sqrt(1/2) -sqrt(1/2)
 *
 * Each row of the above matrix represents the basis vectors of YCrCg in RGB space.
 *
 * The inverse of this matrix (for YCrCg to RGB conversion) is:
 *
 * sqrt(1/3)  sqrt(2/3)  0
 * sqrt(1/3) -sqrt(1/6)  sqrt(1/2)
 * sqrt(1/3) -sqrt(1/6) -sqrt(1/2)
 */

static const ColourMatrix CM_RGB2YCrCg = {.m={
    0.57735027,	 0.57735027,  0.57735027,
    0.81649658, -0.40824829, -0.40824829,
    0.        ,  0.70710678, -0.70710678
}};

static const ColourMatrix CM_YCrCg2RGB = {.m={
    0.57735027,  0.81649658,  0.        ,
    0.57735027, -0.40824829,  0.70710678,
    0.57735027, -0.40824829, -0.70710678
}};

#ifdef __cplusplus
}
#endif

#endif // YCRCG_H
