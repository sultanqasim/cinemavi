#ifndef GAMMA_H
#define GAMMA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// generate lut for gamma encoding a linear space image
// bit depth should be between 10 and 16
// length of lut should be 1 << bit_depth
void gamma_gen_lut(uint8_t *lut, uint8_t bit_depth, double gamma);

// assumes length of lut >= highest value in img_in
void gamma_encode(const uint16_t *img_in, uint8_t *img_out, uint16_t width, uint16_t height,
        const uint8_t *lut);

#ifdef __cplusplus
}
#endif

#endif // GAMMA_H
