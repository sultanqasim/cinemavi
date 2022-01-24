#ifndef NOISE_REDUCTION_H
#define NOISE_REDUCTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// TKTK:
// inputs: YCbCr image, width, height
// outputs: filtered YCbCr image (must be different, not in-place)
// options: gaussian radius, kernel size (3x3, 5x5, 7x7, 9x9), Y value above which NR would not be applied on each colour plane (Y and CbCr)

#ifdef __cplusplus
}
#endif

#endif // NOISE_REDUCTION_H
