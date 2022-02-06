#include "colour_xfrm.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

void print_mat(const ColourMatrix *cmat)
{
    printf("%.3f %.3f %.3f\n", cmat->m[0], cmat->m[1], cmat->m[2]);
    printf("%.3f %.3f %.3f\n", cmat->m[3], cmat->m[4], cmat->m[5]);
    printf("%.3f %.3f %.3f\n", cmat->m[6], cmat->m[7], cmat->m[8]);
}

void cmat_d2f(const ColourMatrix *cmat, ColourMatrix_f *cmat_f)
{
    for (int i = 0; i < 9; i++)
        cmat_f->m[i] = cmat->m[i];
}

/* generate a colour correction matrix
 *
 * exposure:RGB components are multiplied by this value
 *          1.0 means no change to exposure
 * warmth:  valid range -1.0 to 1.0 (cool to warm)
 * tint:    valid range -1.0 to 1.0 (green to magenta)
 * hue:     hue adjustment in radians
 * sat:     saturation is multiplied by this value
 *          1.0 means no change to saturation
 */
void colour_matrix(ColourMatrix *cmat, double exposure, double warmth, double tint,
        double hue, double sat)
{
    ColourMatrix warmth_mat, tint_mat, sat_mat, hue_mat, work1_mat, work2_mat;

    if (exposure < 0.0) exposure = 0.0;
    if (warmth > 1.0) warmth = 1.0;
    else if (warmth < -1.0) warmth = -1.0;

    memset(&warmth_mat, 0, sizeof(ColourMatrix));
    warmth_mat.m[0] = (1 + warmth) * exposure;
    warmth_mat.m[4] = exposure;
    warmth_mat.m[8] = (1 - warmth) * exposure;

    if (tint > 1.0) tint = 1.0;
    if (tint < -1.0) tint = -1.0;

    memset(&tint_mat, 0, sizeof(ColourMatrix));
    tint_mat.m[0] = 1 + 0.5*tint;
    tint_mat.m[4] = 1 - tint;
    tint_mat.m[8] = 1 + 0.5*tint;

    const double desat = 1 - sat;
    sat_mat.m[0] = 1;
    sat_mat.m[1] = desat;
    sat_mat.m[2] = desat;
    sat_mat.m[3] = desat;
    sat_mat.m[4] = 1;
    sat_mat.m[5] = desat;
    sat_mat.m[6] = desat;
    sat_mat.m[7] = desat;
    sat_mat.m[8] = 1;

    // hue shift is rotating colour vector around 255,255,255 axis in RGB space
    // https://stackoverflow.com/questions/8507885/shift-hue-of-an-rgb-color
    const double cosA = cos(hue);
    const double p = (1.0 - cosA) / 3.0;
    const double q = sqrt(1.0 / 3) * sin(hue);
    const double a = cosA + p;
    const double b = p - q;
    const double c = p + q;
    hue_mat.m[0] = a;
    hue_mat.m[1] = b;
    hue_mat.m[2] = c;
    hue_mat.m[3] = c;
    hue_mat.m[4] = a;
    hue_mat.m[5] = b;
    hue_mat.m[6] = b;
    hue_mat.m[7] = c;
    hue_mat.m[8] = a;

    // first adjust white balance in camera RGB space, then rotate hue, then saturation
    colour_matmult33(&work1_mat, &warmth_mat, &tint_mat);
    colour_matmult33(&work2_mat, &work1_mat, &hue_mat);
    colour_matmult33(cmat, &work2_mat, &sat_mat);
}

// convert 16-bit integer to floating point image
void colour_i2f(const uint16_t *img_in, float *img_out, uint16_t width, uint16_t height)
{
    for (uint32_t i = 0; i < width * height * 3; i++)
        img_out[i] = img_in[i];
}

// convert floating point image to 16-bit integer
// if bound != 0, also ensure 0 <= pixel_value <= bound
void colour_f2i(const float *img_in, uint16_t *img_out, uint16_t width, uint16_t height, uint16_t bound)
{
    if (bound == 0) {
        for (uint32_t i = 0; i < width * height * 3; i++)
            img_out[i] = img_in[i];
    } else {
        for (uint32_t i = 0; i < width * height * 3; i++) {
            float v = img_in[i];
            if (v < 0)
                img_out[i] = 0;
            else if (v > bound)
                img_out[i] = bound;
            else
                img_out[i] = v;
        }
    }
}

// apply a colour matrix transformation to every pixel in the image
void colour_xfrm(const float *img_in, float *img_out, uint16_t width, uint16_t height,
        const ColourMatrix_f *cmat)
{
    const ColourPixel *imgp_in = (const ColourPixel *)img_in;
    ColourPixel *imgp_out = (ColourPixel *)img_out;

    for (uint32_t i = 0; i < width * height; i++)
        pixel_xfrm(imgp_in + i, imgp_out + i, cmat);
}

// C = A * B
void colour_matmult33(ColourMatrix *C, const ColourMatrix *A, const ColourMatrix *B)
{
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            C->m[row*3 + col] = A->m[row*3] * B->m[col] +
                                A->m[row*3 + 1] * B->m[col + 3] +
                                A->m[row*3 + 2] * B->m[col + 6];
        }
    }
}

void colour_matinv33(ColourMatrix *inv, const ColourMatrix *mat)
{
    double det = mat->m[0] * (mat->m[4] * mat->m[8] - mat->m[7] * mat->m[5]) -
                 mat->m[1] * (mat->m[3] * mat->m[8] - mat->m[5] * mat->m[6]) +
                 mat->m[2] * (mat->m[3] * mat->m[7] - mat->m[4] * mat->m[6]);

    double invdet = 1 / det;

    inv->m[0] = (mat->m[4] * mat->m[8] - mat->m[7] * mat->m[5]) * invdet;
    inv->m[1] = (mat->m[2] * mat->m[7] - mat->m[1] * mat->m[8]) * invdet;
    inv->m[2] = (mat->m[1] * mat->m[5] - mat->m[2] * mat->m[4]) * invdet;
    inv->m[3] = (mat->m[5] * mat->m[6] - mat->m[3] * mat->m[8]) * invdet;
    inv->m[4] = (mat->m[0] * mat->m[8] - mat->m[2] * mat->m[6]) * invdet;
    inv->m[5] = (mat->m[3] * mat->m[2] - mat->m[0] * mat->m[5]) * invdet;
    inv->m[6] = (mat->m[3] * mat->m[7] - mat->m[6] * mat->m[4]) * invdet;
    inv->m[7] = (mat->m[6] * mat->m[1] - mat->m[0] * mat->m[7]) * invdet;
    inv->m[8] = (mat->m[0] * mat->m[4] - mat->m[3] * mat->m[1]) * invdet;
}
