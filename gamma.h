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
// gamma is the slope at the white end
// black is the slope at the black end
void gamma_gen_lut_cubic(uint8_t *lut, uint8_t bit_depth, double gamma, double black);

// apply a filmic base curve before gamma encoding
// gamma controls how midtones are boosted and highlights compressed
// black is the slope at the black end
void gamma_gen_lut_filmic(uint8_t *lut, uint8_t bit_depth, double gamma, double black);

// apply the base curve x^gamma * (1 - k^x)/(1 - k) before gamma encoding
// shadow slope is approximately 0.03^gamma * ln(1/k) / (1 - k)
// allows extreme shadow boosting while preserving highlights
void gamma_gen_lut_hdr(uint8_t *lut, uint8_t bit_depth, double gamma, double shadow);

// compressed cubic curve with Reinhard tail for shadow boosting while preserving highlights
// gamma is slope at white end
// shadow is how much cubic is compressed to boost shadows
// black is slope at black end before compression of cubic
void gamma_gen_lut_hdr_cubic(uint8_t *lut, uint8_t bit_depth, double gamma, double shadow,
        double black);

// assumes length of lut >= highest value in img_in
void gamma_encode(const uint16_t *img_in, uint8_t *img_out, uint16_t width, uint16_t height,
        const uint8_t *lut);

#ifdef __cplusplus
}
#endif

#endif // GAMMA_H
