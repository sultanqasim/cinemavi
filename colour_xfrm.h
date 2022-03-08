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
    double p[3];
} ColourPixel;

typedef struct {
    float p[3];
} ColourPixel_f;

static inline void pixel_xfrm(const ColourPixel *pix_in, ColourPixel *pix_out,
        const ColourMatrix *cmat)
{
    for (int i = 0; i < 3; i++) {
        pix_out->p[i] =
            pix_in->p[0] * cmat->m[i*3 + 0] +
            pix_in->p[1] * cmat->m[i*3 + 1] +
            pix_in->p[2] * cmat->m[i*3 + 2];
    }
}

static inline void pixel_xfrm_f(const ColourPixel_f *pix_in, ColourPixel_f *pix_out,
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
 * red:     ratio to multiply red channel by
 * blue:    ratio to multiply blue channel by
 * hue:     hue adjustment in radians
 * sat:     saturation is multiplied by this value
 *          1.0 means no change to saturation
 */
void colour_matrix(ColourMatrix *cmat, double red, double blue, double hue, double sat);

// convert 16-bit integer to floating point image
void colour_i2f(const uint16_t *img_in, float *img_out, uint16_t width, uint16_t height);

// convert floating point image to 16-bit integer
// if bound != 0, also ensure 0 <= pixel_value <= bound
void colour_f2i(const float *img_in, uint16_t *img_out, uint16_t width, uint16_t height,
        uint16_t bound);

// apply a colour matrix transformation to every pixel in the image
void colour_xfrm(const float *img_in, float *img_out, uint16_t width, uint16_t height,
        const ColourMatrix_f *cmat);

// C = A * B
void colour_matmult33(ColourMatrix *C, const ColourMatrix *A, const ColourMatrix *B);

// 3x3 matrix inversion
void colour_matinv33(ColourMatrix *inv, const ColourMatrix *mat);

// Daylight illuminant temperature to CIE 1931 xy chromaticity
void colour_temp_tint_to_xy(double temp_K, double tint, double *x, double *y);

// CIE 1931 xy chromaticity to CCT
void colour_xy_to_temp_tint(double x, double y, double *temp_K, double *tint);

// Outputs ratios to multiply source red and blue channels by to correct from specified
// illuminant (x,y)
void colour_illum_xy_to_rb_ratio(const ColourMatrix *src_to_xyz,
        double x, double y, double *ratio_R, double *ratio_B);

// inverse of colour_illum_xy_to_rb_ratio
void colour_rb_ratio_to_illum_xy(const ColourMatrix *src_to_xyz,
        double ratio_R, double ratio_B, double *x, double *y);

// Outputs ratios to multiply source red and blue channels by to correct from specified
// correlated colour temperature (in Kelvin) and tint
void colour_temp_tint_to_rb_ratio(const ColourMatrix *src_to_xyz,
        double temp_K, double tint, double *ratio_R, double *ratio_B);

// inverse of colour_temp_tint_to_rb_ratio
void colour_rb_ratio_to_temp_tint(const ColourMatrix *src_to_xyz,
        double ratio_R, double ratio_B, double *temp_K, double *tint);

// determine camera space values to hit (1.0, 1.0, 1.0) in target space
void colour_white_in_cam(const ColourMatrix *target_to_cam, ColourPixel *cam_white);

// scale matrix (in place) to ensure white in target space is achievable in camera space
// exposure is a boost or reduction in stops, 0 is no change
void colour_matrix_white_scale(ColourMatrix *cam_to_target, double exposure);

// pre-clip integer camera RGB values (in place) to ensure transformed clipped whites stay white
void colour_pre_clip(uint16_t *img, uint16_t width, uint16_t height, uint16_t max_val,
        const ColourMatrix *cam_to_target);

#ifdef __cplusplus
}
#endif

#endif // COLOUR_XFRM_H
