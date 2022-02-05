#ifndef GAMMA_H
#define GAMMA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// generate lut for gamma encoding a linear space image
// bit depth should be between 8 and 16
// length of lut should be 1 << bit_depth
void gamma_gen_lut(uint8_t *lut, uint8_t bit_depth);

// apply cubic base curve before gamma encoding
void gamma_gen_lut_cubic(uint8_t *lut, uint8_t bit_depth,
        double a, double b, double c);

// apply a filmic base curve before gamma encoding
// shadow is the slope at the black end
// gamma controls how midtones are boosted and highlights compressed
void gamma_gen_lut_filmic(uint8_t *lut, uint8_t bit_depth,
        double gamma, double shadow);

// assumes length of lut >= highest value in img_in
void gamma_encode(const uint16_t *img_in, uint8_t *img_out, uint16_t width, uint16_t height,
        const uint8_t *lut);

#ifdef __cplusplus
}
#endif

#endif // GAMMA_H
