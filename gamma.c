#include <math.h>
#include "gamma.h"

// generate lut for gamma encoding a linear space image
// bit depth should be between 8 and 16
// length of lut should be 1 << bit_depth
void gamma_gen_lut(uint8_t *lut, uint8_t bit_depth)
{
    double G = 8.0 / bit_depth;
    double i_scale = 1.0 / (1 << bit_depth);

    for (int i = 0; i < 1 << bit_depth; i++)
        lut[i] = 256 * pow(i * i_scale, G);
}

// assumes length of lut >= highest value in img_in
void gamma_encode(const uint16_t *img_in, uint8_t *img_out, uint16_t width, uint16_t height,
        const uint8_t *lut)
{
    for (uint32_t i = 0; i < width * height * 3; i++)
        img_out[i] = lut[img_in[i]];
}
