#ifndef COLOUR_XFRM_H
#define COLOUR_XFRM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// 3x3 in row major order, to be multiplied by RGB column vector
typedef struct {
    double m[9];
} ColourMatrix;

// float version for faster calculations, but generate matrix as double for precision
typedef struct {
    float m[9];
} ColourMatrix_f;

void print_mat(const ColourMatrix *cmat);

void cmat_d2f(const ColourMatrix *cmat, ColourMatrix_f *cmat_f);

typedef struct {
    float p[3];
} ColourPixel;

static inline void pixel_xfrm(const ColourPixel *pix_in, ColourPixel *pix_out,
        const ColourMatrix_f *cmat)
{
    for (int i = 0; i < 3; i++) {
        pix_out->p[i] =
            pix_in->p[0] * cmat->m[i*3 + 0] +
            pix_in->p[1] * cmat->m[i*3 + 1] +
            pix_in->p[2] * cmat->m[i*3 + 2];
    }
}

/* generate a colour correction matrix
 *
 * warmth:  valid range -1.0 to 1.0 (cool to warm)
 * tint:    valid range -1.0 to 1.0 (green to magenta)
 * hue:     hue adjustment in radians
 * sat:     saturation is multiplied by this value
 *          1.0 means no change to saturation
 */
void colour_matrix(ColourMatrix *cmat, double warmth, double tint, double hue, double sat);

// convert 16-bit integer to floating point image
void colour_i2f(const uint16_t *img_in, float *img_out, uint16_t width, uint16_t height);

// convert floating point image to 16-bit integer
// if bound != 0, also ensure 0 <= pixel_value <= bound
void colour_f2i(const float *img_in, uint16_t *img_out, uint16_t width, uint16_t height, uint16_t bound);

// apply a colour matrix transformation to every pixel in the image
void colour_xfrm(const float *img_in, float *img_out, uint16_t width, uint16_t height,
        const ColourMatrix_f *cmat);

// C = A * B
void colour_matmult33(ColourMatrix *C, const ColourMatrix *A, const ColourMatrix *B);

// 3x3 matrix inversion
void colour_matinv33(ColourMatrix *inv, const ColourMatrix *mat);

#ifdef __cplusplus
}
#endif

#endif // COLOUR_XFRM_H
