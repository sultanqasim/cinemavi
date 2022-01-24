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
void colour_matrix(ColourMatrix *cmat, double warmth, double tint, double hue, double sat)
{
    ColourMatrix warmth_mat, tint_mat, sat_mat, hue_mat, work1_mat, work2_mat;

    if (warmth > 1.0) warmth = 1.0;
    else if (warmth < -1.0) warmth = -1.0;

    memset(&warmth_mat, 0, sizeof(ColourMatrix));
    warmth_mat.m[0] = warmth < 0 ? 1 + warmth : 1;
    warmth_mat.m[4] = 1;
    warmth_mat.m[8] = warmth > 0 ? 1 - warmth : 1;


    if (tint > 1.0) tint = 1.0;
    if (tint < -1.0) tint = -1.0;

    memset(&tint_mat, 0, sizeof(ColourMatrix));
    tint_mat.m[0] = tint < 0 ? 1 + tint : 1;
    tint_mat.m[4] = tint > 0 ? 1 - tint : 1;
    tint_mat.m[8] = tint < 0 ? 1 + tint : 1;

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

// converts rgb 16-bit image to YCbCr floating point image and fixes colour,
// through a single matrix multiplication per pixel
void colour_xfrm(const uint16_t *rgb, float *ycbcr, uint16_t width, uint16_t height, const ColourMatrix *cmat);
// TODO
// To convert to YCbCr space, project RGB vector onto YCbCr primaries in RGB space
// ie. dot product RGB vector with each YCbCr primary, divided by magnitude of primary

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
