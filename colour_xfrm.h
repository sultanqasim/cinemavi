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

void print_mat(const ColourMatrix *cmat);

/* generate a colour correction matrix
 *
 * warmth:
 *      valid range -1.0 to 1.0
 *      negative values cause red channel to be multiplied by (1 + warmth)
 *      positive values cause blue channel to be multiplied by (1 - warmth)
 * tint:
 *      valid range -1.0 to 1.0
 *      negative values cause red and blue to be multiplied by (1 + tint)
 *      positive values cause green to be multiplied by (1 - tiny)
 *
 * hue:
 *      hue adjustment in radians
 *
 * sat:
 *      saturation is multiplied by this value
 *      1.0 means no change to saturation
 */
void colour_matrix(ColourMatrix *cmat, double warmth, double tint, double hue, double sat);

/* converts rgb 16-bit image to YCbCr floating point image, and adusts white balance,
 * through a single matrix multiplication per pixel
 */
void colour_xfrm(const uint16_t *rgb, float *ycbcr, uint16_t width, uint16_t height, const ColourMatrix *cmat);

// C = A * B
void colour_matmult33(ColourMatrix *C, const ColourMatrix *A, const ColourMatrix *B);

// 3x3 matrix inversion
void colour_matinv33(ColourMatrix *inv, const ColourMatrix *mat);

#ifdef __cplusplus
}
#endif

#endif // COLOUR_XFRM_H
